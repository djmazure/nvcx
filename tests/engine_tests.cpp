#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "nvcx/simulation.hpp"
#include "nvcx/vivado.hpp"

using namespace nvcx;
namespace fs = std::filesystem;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond))                                                           \
            throw std::runtime_error(                                          \
                std::format("{}:{}: {}", __FILE__, __LINE__, #cond));          \
    } while (0)

namespace {

std::string g_kernel;
fs::path g_designs;
std::string snap(const std::string& s) {
    return (g_designs / "xsim.dir" / s / "xsimk.so").string();
}

const Simulation::Port& port(const Simulation& d, const char* name) {
    const Simulation::Port* p = d.port(name);
    if (!p)
        throw std::runtime_error(std::string("no port ") + name);
    return *p;
}
// A port's value as a logic string, MSB first.
std::string logic(const Simulation& d, const char* name) {
    const Simulation::Port& p = port(d, name);
    std::vector<uint8_t> b(p.raw_size());
    d.get(p, b.data());
    if (!p.vhdl)
        return vlog_to_logic(b.data(), p.size);
    std::string s;
    for (uint8_t c : b)
        s += c < 9 ? LOGIC_CHARS[c] : '?';
    return s;
}
void set_logic(Simulation& d, const char* name, const std::string& s) {
    const Simulation::Port& p = port(d, name);
    std::vector<uint8_t> b(p.raw_size());
    if (!p.vhdl)
        logic_to_vlog(s, b.data(), p.size);
    else {
        CHECK(s.size() == b.size());
        for (size_t i = 0; i < s.size(); i++) {
            const char* q = strchr(LOGIC_CHARS, s[i]);
            CHECK(q);
            b[i] = (uint8_t)(q - LOGIC_CHARS);
        }
    }
    d.put(p, b.data());
}
std::string bits(unsigned v, int n) {
    std::string s(n, '0');
    for (int i = 0; i < n; i++)
        if (v >> (n - 1 - i) & 1)
            s[i] = '1';
    return s;
}

void test_ports() {
    // XSI's port table: names, directions, and sizes in bytes for VHDL
    Simulation d(snap("hier"), g_kernel);
    CHECK(d.precision() == -12);
    CHECK(d.ports().size() == 6);
    CHECK(port(d, "clk").dir == Direction::In && port(d, "clk").size == 1);
    CHECK(port(d, "din").dir == Direction::In && port(d, "din").size == 4);
    CHECK(port(d, "dout").dir == Direction::Out && port(d, "dout").size == 4);
    CHECK(port(d, "ival").size == 4 && port(d, "rval").size == 8 &&
          port(d, "bval").size == 1);
    for (const Simulation::Port& p : d.ports())
        CHECK(p.vhdl);
    CHECK(d.run(0).status == Simulation::RunStatus::Completed);
    int32_t ival;
    std::vector<uint8_t> b(4);
    d.get(port(d, "ival"), b.data());
    memcpy(&ival, b.data(), 4);
    CHECK(ival == 5);
}

void test_clocked_run() {
    Simulation d(snap("dut"), g_kernel);
    for (unsigned v : {0u, 1u, 0xBEEFu, 0xFFFFu}) {
        set_logic(d, "u_in", bits(v, 16));
        set_logic(d, "clk", "1");
        CHECK(d.run(5000).status == Simulation::RunStatus::Completed);
        set_logic(d, "clk", "0");
        d.run(5000);
        CHECK(logic(d, "u_out") == bits(v, 16));
    }
    CHECK(d.time() == 40000);
}

void test_sequential_designs() {
    for (const char* s : {"hier", "dut", "hier"}) {
        Simulation d(snap(s), g_kernel);
        CHECK(d.time() == 0);
        CHECK(d.run(1000).status == Simulation::RunStatus::Completed);
    }
}

void test_next_event_time() {
    {
        Simulation d(snap("exp"), g_kernel);
        uint64_t t;
        CHECK(d.next_event_time(t) && t == 5000);
        d.run(7000);
        CHECK(d.next_event_time(t) && t == 10000);
    }
    {
        Simulation d(snap("dut"), g_kernel);
        uint64_t t;
        CHECK(!d.next_event_time(t));
        set_logic(d, "clk", "1");
        d.run(100);
        CHECK(!d.next_event_time(t) && d.time() == 100);
    }
}

void test_verilog_values() {
    Simulation d(snap("dut_v"), g_kernel);
    for (const Simulation::Port& p : d.ports())
        CHECK(!p.vhdl); // sizes in bits
    CHECK(port(d, "u_in").size == 16 && port(d, "clk").size == 1);
    CHECK(logic(d, "u_in") == std::string(16, 'Z') &&
          logic(d, "u_out") == std::string(16, 'X'));
    set_logic(d, "doom", "0");
    set_logic(d, "u_in", "0000000000001011");
    CHECK(logic(d, "u_in") == "0000000000001011");
    set_logic(d, "clk", "1");
    d.run(5000);
    set_logic(d, "clk", "0");
    d.run(5000);
    CHECK(logic(d, "u_out") == "0000000000001011");
    set_logic(d, "u_in", "ZZZZ0101XXXX1HL0");
    CHECK(logic(d, "u_in") == "ZZZZ0101XXXX1100");
}

void test_run_zero_applies_deposits() {
    // The bridge deposits inputs and then runs zero ticks: the kernel must
    // process the resulting events (a clock edge among them) without time
    // advancing, for VHDL and Verilog alike.
    for (const char* which : {"dut", "dut_v"}) {
        Simulation d(snap(which), g_kernel);
        set_logic(d, "doom", "0");
        set_logic(d, "u_in", bits(0x1234, 16));
        set_logic(d, "clk", "1");
        CHECK(d.run(0).status == Simulation::RunStatus::Completed);
        if (logic(d, "u_out") != bits(0x1234, 16))
            throw std::runtime_error(std::format(
                "{}: u_out after run(0) = {}", which, logic(d, "u_out")));
        set_logic(d, "clk", "0");
        d.run(0);
        set_logic(d, "u_in", bits(0x4321, 16));
        set_logic(d, "clk", "1");
        d.run(0);
        if (logic(d, "u_out") != bits(0x4321, 16))
            throw std::runtime_error(std::format(
                "{}: second edge: u_out = {}", which, logic(d, "u_out")));
        CHECK(d.time() == 0);
    }
}

void test_verilog_on_instance_copy() {
    // A Verilog design on a per-instance kernel copy (what the bridge always
    // uses).
    fs::path work = fs::temp_directory_path() /
                    ("nvcx-tests-vlog-" + std::to_string(getpid()));
    Kernel::get(g_kernel);
    InstancePaths p = make_instance(work, instance_tag(0), snap("dut_v"));
    {
        Simulation d(p.design.string(), p.kernel.string());
        // the copies are deleted as soon as the design is open; it keeps
        // working
        discard_instance(p);
        CHECK(!fs::exists(p.root));
        CHECK(logic(d, "u_out") == std::string(16, 'X'));
        set_logic(d, "doom", "0");
        set_logic(d, "u_in", bits(0x0007, 16));
        d.run(1000);
        set_logic(d, "clk", "1");
        d.run(0);
        if (logic(d, "u_out") != bits(0x0007, 16) ||
            logic(d, "u_inc") != bits(0x0008, 16)) {
            throw std::runtime_error(std::format(
                "u_out={} u_inc={}", logic(d, "u_out"), logic(d, "u_inc")));
        }
    }
}

void test_version_mismatch_refused() {
    // a snapshot's xsim.version must match the kernel's release; the kernel
    // itself does not check (and fails obscurely)
    const std::string kernel_version = Kernel::get(g_kernel).version;
    if (kernel_version.empty()) {
        fprintf(stderr, "  (kernel has no version string; nothing to check)\n");
        return;
    }
    fs::path work = fs::temp_directory_path() /
                    ("nvcx-tests-ver-" + std::to_string(getpid()));
    fs::path dir = work / "xsim.dir" / "exp";
    fs::create_directories(dir);
    for (auto& e : fs::directory_iterator(fs::path(snap("exp")).parent_path()))
        if (e.is_regular_file())
            fs::copy_file(e.path(),
                          dir / e.path().filename(),
                          fs::copy_options::overwrite_existing);
    {
        std::ofstream(dir / "xsim.version") << "1999.1\n";
    }
    try {
        Simulation d((dir / "xsimk.so").string(), g_kernel);
        throw std::runtime_error("a snapshot from another release opened");
    } catch (const std::runtime_error& e) {
        std::string what = e.what();
        CHECK(what.find("1999.1") != std::string::npos);
        CHECK(what.find(kernel_version) != std::string::npos);
    }
    fs::remove_all(work);
}

void test_second_live_design_refused() {
    // one live design per kernel library: the kernel holds global state
    Simulation a(snap("dut"), g_kernel);
    try {
        Simulation b(snap("hier"), g_kernel);
        throw std::runtime_error("a second design opened on the same kernel");
    } catch (const std::runtime_error& e) {
        CHECK(std::string(e.what()).find("already open") != std::string::npos);
    }
}

const std::map<std::string, std::function<void()>> TESTS = {
    {"ports", test_ports},
    {"clocked_run", test_clocked_run},
    {"sequential_designs", test_sequential_designs},
    {"version_mismatch_refused", test_version_mismatch_refused},
    {"next_event_time", test_next_event_time},
    {"verilog_values", test_verilog_values},
    {"run_zero_applies_deposits", test_run_zero_applies_deposits},
    {"verilog_on_instance_copy", test_verilog_on_instance_copy},
    {"second_live_design_refused", test_second_live_design_refused},
};

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--list") {
        for (auto& [name, fn] : TESTS)
            printf("%s\n", name.c_str());
        return 0;
    }
    if (argc < 3) {
        fprintf(stderr, "usage: nvcx-tests <kernel.so> <designs dir> [name]\n");
        return 2;
    }
    g_kernel = argv[1];
    g_designs = argv[2];
    int failed = 0;
    for (auto& [name, fn] : TESTS) {
        if (argc > 3 && name != argv[3])
            continue;
        try {
            fn();
            printf("PASS %s\n", name.c_str());
        } catch (const std::exception& e) {
            printf("FAIL %s: %s\n", name.c_str(), e.what());
            failed++;
        }
    }
    return failed ? 1 : 0;
}
