#include "nvcx/simulation.hpp"

#include <dlfcn.h>
#include <format>
#include <fstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace nvcx {

namespace {

    std::string dirname_of(const std::string& p) {
        auto s = p.rfind('/');
        return s == std::string::npos ? "." : p.substr(0, s);
    }

    // The release that elaborated the snapshot in *dir*, from its xsim.version
    // (xelab 2025.2 and later), else "".
    std::string snapshot_version(const std::string& dir) {
        std::ifstream f(dir + "/xsim.version");
        std::string v;
        f >> v;
        return v;
    }

    // The kernel keeps its state in process-wide globals
    std::unordered_map<Kernel*, Simulation*> g_live;
    std::mutex g_live_mu;

    // xsi_open is serialized across instances
    std::mutex g_open_mu;

    // Registered with atexit after each Simulation has loaded its libraries,
    // so it runs before any of their static destructors
    void close_live_at_exit() {
        std::vector<Simulation*> live;
        {
            std::lock_guard<std::mutex> lock(g_live_mu);
            for (auto& [k, sim] : g_live)
                live.push_back(sim);
        }
        for (Simulation* sim : live)
            sim->close();
    }

} // namespace

Simulation::Simulation(const std::string& design_so,
                       const std::string& kernel_so)
    : design_dir_(dirname_of(design_so)) {
    kernel_ = &Kernel::get(kernel_so);
    // A snapshot only loads on the release that elaborated it; the kernel
    // does not check, and a mismatch dies with a garbage allocation size.
    const std::string elaborated_by = snapshot_version(design_dir_);
    if (!elaborated_by.empty() && !kernel_->version.empty() &&
        elaborated_by != kernel_->version)
        throw std::runtime_error(
            std::format("snapshot {} was elaborated by Vivado {} but the "
                        "simulator kernel is {} ({}); re-elaborate it or set "
                        "XILINX_VIVADO to the {} installation",
                        design_dir_,
                        elaborated_by,
                        kernel_->version,
                        kernel_->path(),
                        elaborated_by));
    {
        // one live design per kernel library: check and claim atomically
        std::lock_guard<std::mutex> lock(g_live_mu);
        if (g_live.count(kernel_))
            throw std::runtime_error(
                std::format("a design is already open on {} (the kernel holds "
                            "global state; close it first or use a "
                            "per-instance kernel copy)",
                            kernel_->path()));
        g_live[kernel_] = this;
    }
    std::lock_guard<std::mutex> open_lock(g_open_mu);
    try {
        dlerror();
        design_handle_ =
            dlopen(design_so.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
        if (!design_handle_)
            throw std::runtime_error(
                std::format("cannot load design {}: {}", design_so, dlerror()));
        auto xsi_open = reinterpret_cast<XsiHandle (*)(XsiSetupInfo*)>(
            dlsym(design_handle_, "xsi_open"));
        if (!xsi_open)
            throw std::runtime_error("design library has no xsi_open: " +
                                     design_so);
        setup_.logFileName = nullptr;
        setup_.wdbFileName = nullptr;
        // XSI resolves xsim.dir/<snapshot>/... relative to the working
        // directory unless told otherwise;
        xsim_dir_ = dirname_of(design_dir_);
        setup_.xsimDir = xsim_dir_.data();
        try {
            handle_ = xsi_open(&setup_);
        } catch (...) {
            throw std::runtime_error(std::format(
                "xsi_open threw (is {} the xsim.dir of this design?)",
                xsim_dir_));
        }
        if (!handle_)
            throw std::runtime_error("xsi_open failed for " + design_so);
        int n = kernel_->xsi_get_int(handle_, xsiNumTopPorts);
        for (int i = 0; i < n; i++) {
            Port p;
            p.index = i;
            const char* nm =
                kernel_->xsi_get_str_port(handle_, i, xsiNameTopPort);
            p.name = nm ? nm : "";
            switch (
                kernel_->xsi_get_int_port(handle_, i, xsiDirectionTopPort)) {
            case xsiInputPort:
                p.dir = Direction::In;
                break;
            case xsiOutputPort:
                p.dir = Direction::Out;
                break;
            case xsiInoutPort:
                p.dir = Direction::Inout;
                break;
            }
            p.size = (unsigned)kernel_->xsi_get_int_port(
                handle_, i, xsiHDLValueSize);
            p.vhdl = kernel_->host_isPortVHDL(handle_, i);
            ports_.push_back(std::move(p));
        }
        atexit(close_live_at_exit);
    } catch (...) {
        {
            std::lock_guard<std::mutex> lock(g_live_mu);
            g_live.erase(kernel_);
        }
        if (handle_)
            kernel_->xsi_close(handle_);
        if (design_handle_)
            dlclose(design_handle_);
        throw;
    }
}

void Simulation::close() {
    {
        std::lock_guard<std::mutex> lock(g_live_mu);
        g_live.erase(kernel_);
    }
    if (handle_)
        kernel_->xsi_close(handle_);
    handle_ = nullptr;
}

Simulation::~Simulation() {
    close();
    if (design_handle_)
        dlclose(design_handle_);
}

const Simulation::Port* Simulation::port(const std::string& name) const {
    for (const Port& p : ports_)
        if (p.name == name)
            return &p;
    return nullptr;
}

void Simulation::get(const Port& p, uint8_t* buf) const {
    // XSI writes only the significant bits of each Verilog word
    std::memset(buf, 0, p.raw_size());
    kernel_->xsi_get_value(handle_, p.index, buf);
}

void Simulation::put(const Port& p, const uint8_t* buf) {
    kernel_->xsi_put_value(handle_, p.index, const_cast<uint8_t*>(buf));
}

// xsi_get_error_info() is the kernel's message buffer for the last run, which
// also carries plain `report`/`assert` messages (as '@"Note: ...\nTime:
// ...\n"'); the status tells errors apart.
std::string Simulation::last_error() {
    const char* e = kernel_->xsi_get_error_info(handle_);
    return e ? e : "";
}

void Simulation::emit_report(const std::string& raw) {
    std::string msg = raw;
    if (msg.rfind("@\"", 0) == 0)
        msg.erase(0, 2);
    while (!msg.empty() && (msg.back() == '"' || msg.back() == '\n'))
        msg.pop_back();
    if (report_sink_) {
        report_sink_(msg);
        return;
    }
    fprintf(stdout, "%s\n", msg.c_str());
    fflush(stdout);
}

int Simulation::report_severity(const std::string& msg) {
    if (msg.starts_with("Failure:"))
        return 3;
    if (msg.starts_with("Error:"))
        return 2;
    if (msg.starts_with("Warning:"))
        return 1;
    return 0;
}

Simulation::RunResult Simulation::run(uint64_t ticks) {
    if (finished_)
        return {RunStatus::Finished, "simulation finished"};
    const uint64_t target = time() + ticks;
    uint64_t now = time();
    kernel_->xsi_run(handle_, target > now ? target - now : 0);
    std::string err = last_error();
    int st = kernel_->xsi_get_status(handle_);
    if (st == xsiError || st == xsiFatalError)
        return {RunStatus::Error,
                err.empty() ? "kernel status " + std::to_string(st) : err};
    if (!err.empty())
        emit_report(
            err); // a report; what it means for the run is the sink's call
    int ks = kernel_->status_kind();
    if (ks == abi::KS_FINISHED || ks == abi::KS_VHDL_FINISH ||
        ks == abi::KS_CLOSED) {
        finished_ = true;
        return {RunStatus::Finished, "simulation finished"};
    }
    return {RunStatus::Completed, ""};
}

std::string vlog_to_logic(const uint8_t* raw, unsigned nbits) {
    std::string out(nbits, 'X');
    for (unsigned i = 0; i < nbits; i++) {
        VlogLogicVal w;
        std::memcpy(&w, raw + (i / 32) * sizeof(VlogLogicVal), sizeof w);
        bool a = (w.aVal >> (i % 32)) & 1, b = (w.bVal >> (i % 32)) & 1;
        out[nbits - 1 - i] = b ? (a ? 'X' : 'Z') : (a ? '1' : '0');
    }
    return out;
}

void logic_to_vlog(const std::string& s, uint8_t* raw, unsigned nbits) {
    const size_t words = (nbits + 31) / 32;
    std::vector<VlogLogicVal> w(words, VlogLogicVal{0, 0});
    for (unsigned i = 0; i < nbits && i < s.size(); i++) {
        char c = toupper((unsigned char)s[s.size() - 1 - i]);
        bool a, b;
        if (c == '0' || c == 'L') {
            a = 0;
            b = 0;
        } else if (c == '1' || c == 'H') {
            a = 1;
            b = 0;
        } else if (c == 'Z') {
            a = 0;
            b = 1;
        } else {
            a = 1;
            b = 1;
        }
        if (a)
            w[i / 32].aVal |= 1u << (i % 32);
        if (b)
            w[i / 32].bVal |= 1u << (i % 32);
    }
    std::memcpy(raw, w.data(), words * sizeof(VlogLogicVal));
}

} // namespace nvcx
