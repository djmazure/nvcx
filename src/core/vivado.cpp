#include "nvcx/vivado.hpp"

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
        const char* string(uint64_t off) const {
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

} // namespace

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

} // namespace nvcx
