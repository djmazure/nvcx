#include "nvcx/vivado.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <elf.h>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

namespace nvcx {

namespace {

    const char* const SHARED_WITH_HOST[] = {"libstdc++.so",
                                            "libgcc_s.so",
                                            "libc.so",
                                            "libm.so",
                                            "libdl.so",
                                            "libpthread.so",
                                            "librt.so",
                                            "libz.so",
                                            "libutil.so"};
    const char* const BRIDGE_SONAME = "libxv_simbridge_kernel.so";

    bool host_provided(const std::string& n) {
        for (const char* p : SHARED_WITH_HOST)
            if (n.rfind(p, 0) == 0)
                return true;
        return false;
    }

    std::vector<char> slurp(const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        if (!f)
            throw std::runtime_error("cannot read " + p.string());
        return std::vector<char>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
    }

    // $XILINX_VIVADO, or an error.
    fs::path vivado_dir() {
        const char* v = getenv("XILINX_VIVADO");
        if (!v || !*v)
            throw std::runtime_error(
                "XILINX_VIVADO is not set (source a Vivado settings64.sh)");
        return v;
    }

    // vivado_dir()/lib/lnx64.o, where the simulator's libraries are.
    fs::path vivado_libdir() {
        return vivado_dir() / "lib" / "lnx64.o";
    }

    // Vivado's simulator kernel, libxv_simulator_kernel.so.
    fs::path vivado_kernel() {
        fs::path libdir = vivado_libdir();
        std::string best;
        if (fs::is_directory(libdir))
            for (auto& e : fs::directory_iterator(libdir)) {
                std::string n = e.path().filename().string();
                if (n.find("_simulator_kernel.so") != std::string::npos &&
                    (best.empty() || n < best))
                    best = n;
            }
        if (best.empty())
            throw std::runtime_error("no simulator kernel in " +
                                     libdir.string());
        return libdir / best;
    }

    // Directory holding this library (the stub is installed beside it).
    fs::path self_dir() {
        Dl_info info{};
        if (dladdr(reinterpret_cast<void*>(&self_dir), &info) && info.dli_fname)
            return fs::path(info.dli_fname).parent_path();
        return fs::read_symlink("/proc/self/exe").parent_path();
    }

    // nvcx's stub for the bridge library, beside this library or executable
    // (the build tree) or in lib/nvcx of the install.
    fs::path stub_path() {
        for (const fs::path& cand :
             {self_dir() / BRIDGE_SONAME,
              self_dir() / ".." / "lib" / "nvcx" / BRIDGE_SONAME})
            if (fs::is_regular_file(cand))
                return cand;
        throw std::runtime_error(
            std::format("bridge stub {} not found next to {}",
                        BRIDGE_SONAME,
                        self_dir().string()));
    }

    // The dynamic section of a shared library, with its string table.
    struct ElfDynamic {
        std::vector<char> data;
        const Elf64_Dyn* dyn = nullptr;
        size_t count = 0;
        uint64_t strtab = 0; // file offset
        std::vector<Elf64_Dyn> entries() const {
            std::vector<Elf64_Dyn> out;
            for (size_t i = 0; i < count && dyn[i].d_tag != DT_NULL; i++)
                out.push_back(dyn[i]);
            return out;
        }
        char* string(uint64_t off) {
            return data.data() + strtab + off;
        }
    };

    ElfDynamic read_dynamic(const fs::path& so) {
        ElfDynamic e;
        e.data = slurp(so);
        std::vector<char>& d = e.data;
        if (d.size() < sizeof(Elf64_Ehdr) ||
            std::memcmp(d.data(), ELFMAG, SELFMAG) != 0 ||
            d[EI_CLASS] != ELFCLASS64)
            throw std::runtime_error(so.string() + ": not an ELF64 file");
        auto* eh = reinterpret_cast<const Elf64_Ehdr*>(d.data());
        auto* ph = reinterpret_cast<const Elf64_Phdr*>(d.data() + eh->e_phoff);
        const Elf64_Phdr* dyn = nullptr;
        for (unsigned i = 0; i < eh->e_phnum; i++)
            if (ph[i].p_type == PT_DYNAMIC)
                dyn = &ph[i];
        if (!dyn)
            throw std::runtime_error(so.string() + ": no PT_DYNAMIC");
        auto vaddr_to_off = [&](uint64_t va) -> uint64_t {
            for (unsigned i = 0; i < eh->e_phnum; i++)
                if (ph[i].p_type == PT_LOAD && va >= ph[i].p_vaddr &&
                    va < ph[i].p_vaddr + ph[i].p_filesz)
                    return ph[i].p_offset + (va - ph[i].p_vaddr);
            throw std::runtime_error(so.string() + ": bad vaddr");
        };
        e.dyn = reinterpret_cast<const Elf64_Dyn*>(d.data() + dyn->p_offset);
        e.count = dyn->p_filesz / sizeof(Elf64_Dyn);
        for (const Elf64_Dyn& x : e.entries())
            if (x.d_tag == DT_STRTAB)
                e.strtab = vaddr_to_off(x.d_un.d_ptr);
        return e;
    }

    struct ElfInfo {
        std::vector<std::string> needed;
        std::string soname;
    };

    ElfInfo elf_info(const fs::path& so) {
        ElfDynamic elf = read_dynamic(so);
        ElfInfo info;
        for (const Elf64_Dyn& d : elf.entries()) {
            if (d.d_tag == DT_NEEDED)
                info.needed.push_back(elf.string(d.d_un.d_val));
            else if (d.d_tag == DT_SONAME)
                info.soname = elf.string(d.d_un.d_val);
        }
        return info;
    }

    // Rewrite DT_SONAME / DT_NEEDED strings in place with same-length
    // replacements (name -> name).
    void rename_in_place(const fs::path& so,
                         const std::map<std::string, std::string>& mapping) {
        ElfDynamic elf = read_dynamic(so);
        bool changed = false;
        for (const Elf64_Dyn& d : elf.entries()) {
            if (d.d_tag != DT_NEEDED && d.d_tag != DT_SONAME)
                continue;
            char* s = elf.string(d.d_un.d_val);
            auto it = mapping.find(s);
            if (it == mapping.end())
                continue;
            if (it->second.size() != it->first.size())
                throw std::runtime_error("rename must keep the length: " +
                                         it->first);
            std::memcpy(s, it->second.data(), it->second.size());
            changed = true;
        }
        if (changed) {
            std::ofstream f(so, std::ios::binary | std::ios::trunc);
            f.write(elf.data.data(), (std::streamsize)elf.data.size());
        }
    }

    // "libxv_simulator_kernel.so" with tag "00002a" ->
    // "libxv_simulator_00002a.so": the rename is done in place in the
    // string table, so the length is fixed.
    std::string instance_name(const std::string& name, const std::string& tag) {
        auto i = name.find(".so");
        if (i == std::string::npos || i <= tag.size())
            throw std::runtime_error(
                "library name too short for an instance tag: " + name);
        std::string r = name;
        r.replace(i - tag.size(), tag.size(), tag);
        return r;
    }

} // namespace

fs::path default_workdir() {
    const char* e = getenv("NVCX_WORK");
    return e && *e ? fs::path(e) : fs::path("nvcx.work");
}

const Closure& load_closure() {
    static std::mutex mu;
    static std::optional<Closure> loaded;
    std::lock_guard<std::mutex> lock(mu);
    if (loaded)
        return *loaded;
    Closure c;
    c.kernel = vivado_kernel();
    // Transitive closure of the kernel's NEEDED entries within Vivado's
    // library directories, minus what the host provides.
    const fs::path search[] = {vivado_libdir(), vivado_libdir() / "Ubuntu"};
    std::map<std::string, fs::path> found;
    std::vector<fs::path> todo{c.kernel};
    while (!todo.empty()) {
        fs::path lib = todo.back();
        todo.pop_back();
        for (const std::string& n : elf_info(lib).needed) {
            if (found.count(n) || host_provided(n))
                continue;
            for (const fs::path& d : search)
                if (fs::is_regular_file(d / n)) {
                    found[n] = d / n;
                    todo.push_back(d / n);
                    break;
                }
        }
    }
    // The bridge library: Vivado's own only when the closure imports from it.
    auto bridge = found.find(BRIDGE_SONAME);
    if (bridge != found.end()) {
        c.bridge = bridge->second;
        found.erase(bridge);
    } else {
        c.bridge = stub_path();
        if (elf_info(c.bridge).soname != BRIDGE_SONAME)
            throw std::runtime_error(
                std::format("{} does not carry the soname {}",
                            c.bridge.string(),
                            BRIDGE_SONAME));
    }
    std::vector<fs::path> pending{c.bridge};
    for (auto& [name, path] : found)
        pending.push_back(path);
    // Each library needs its own dependencies loaded first (there is no
    // rpath): passes until no progress. (Vivado's libgomp uses static TLS,
    // which dlopen serves from glibc's surplus; one such library fits with
    // room to spare, and dlopen says so clearly if that ever changes.)
    std::string last_error;
    while (!pending.empty()) {
        std::vector<fs::path> next;
        for (const fs::path& p : pending) {
            if (dlopen(p.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND))
                continue;
            last_error = dlerror();
            next.push_back(p);
        }
        if (next.size() == pending.size())
            throw std::runtime_error(
                std::format("cannot load the kernel's library closure from "
                            "{}: {}",
                            vivado_libdir().string(),
                            last_error));
        pending.swap(next);
    }
    loaded = c;
    return *loaded;
}

std::string instance_tag(uint64_t n) {
    // base 62, fixed width: 62^6 instances before the width would have to
    // grow
    static const char digits[] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string tag(6, '0');
    for (size_t i = tag.size(); i-- > 0 && n > 0; n /= 62)
        tag[i] = digits[n % 62];
    if (n > 0)
        throw std::runtime_error("too many xsim instances in one process");
    return tag;
}

InstancePaths make_instance(const fs::path& outdir_root,
                            const std::string& tag,
                            const fs::path& design_so) {
    const Closure& closure = load_closure();
    // the snapshot's name is its directory's, after any symbolic links: the
    // kernel expects xsim.dir/<name> to be the name it was elaborated under
    const std::string snap =
        fs::canonical(design_so).parent_path().filename().string();
    // A private directory per instance and process, so concurrent processes
    // sharing a work directory never touch each other's copies.
    fs::create_directories(outdir_root.parent_path());
    std::string root = outdir_root.string() + ".XXXXXX";
    if (!mkdtemp(root.data()))
        throw std::runtime_error(
            std::format("cannot create {}: {}", root, strerror(errno)));
    // xsi_open insists on the elaboration layout: the snapshot directory
    // must be a child of a directory literally named xsim.dir.
    fs::path outdir = fs::path(root) / "xsim.dir" / snap;
    fs::create_directories(outdir);
    // the two libraries a design NEEDs by soname, under this instance's names
    const std::string kernel = closure.kernel.filename().string();
    const std::string bridge = "libxv_simbridge_kernel.so";
    std::map<std::string, std::string> ren = {
        {kernel, instance_name(kernel, tag)},
        {bridge, instance_name(bridge, tag)}};
    for (auto& [name, src] : {std::pair{kernel, closure.kernel},
                              std::pair{bridge, closure.bridge}}) {
        fs::copy_file(
            src, outdir / ren[name], fs::copy_options::overwrite_existing);
        rename_in_place(outdir / ren[name], ren);
    }
    InstancePaths p;
    p.root = root;
    p.kernel = outdir / ren[kernel];
    p.design = outdir / (snap + "_xsimk." + tag + ".so");
    fs::copy_file(design_so, p.design, fs::copy_options::overwrite_existing);
    rename_in_place(p.design, ren);
    for (const char* f : {"xsim.dbg",
                          "xsim.rtti",
                          "xsim.type",
                          "xsim.mem",
                          "xsim.reloc",
                          "xsim.svtype",
                          "xsim.version",
                          "xsim.xdbg"})
        if (fs::is_regular_file(design_so.parent_path() / f))
            fs::copy_file(design_so.parent_path() / f,
                          outdir / f,
                          fs::copy_options::overwrite_existing);
    // The design copy NEEDs the renamed stub by soname; the loader matches
    // already-loaded sonames.
    std::string stub = (outdir / ren.at("libxv_simbridge_kernel.so")).string();
    if (!dlopen(stub.c_str(), RTLD_LAZY | RTLD_LOCAL))
        throw std::runtime_error(
            std::format("cannot preload {}: {}", stub, dlerror()));
    return p;
}

void discard_instance(const InstancePaths& p) {
    std::error_code ec;
    fs::remove_all(p.root, ec); // best effort: a leftover is only clutter
}

} // namespace nvcx
