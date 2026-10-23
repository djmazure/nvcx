#pragma once

#include <filesystem>
#include <string>

namespace nvcx {

// $NVCX_WORK, default nvcx.work: where the per-instance kernel copies go
// (and where tcl/nvcx.tcl keeps its snapshot cache).
std::filesystem::path default_workdir();

struct Closure {
    std::filesystem::path kernel; // Vivado's simulator kernel (not loaded here)
    std::filesystem::path
        bridge; // the bridge library in use: the stub, or Vivado's own
};

// Load the kernel's dependency closure (every library but the kernel itself)
// from $XILINX_VIVADO into the process, once, and describe it. The kernel is
// not loaded: Kernel::get does that, for Vivado's kernel or a per-instance
// copy.
const Closure& load_closure();

struct InstancePaths {
    std::filesystem::path root;   // this instance's private directory
    std::filesystem::path kernel; // renamed kernel copy (load with Kernel::get)
    std::filesystem::path design; // renamed design copy (open with Design)
};

// The tag of the n-th instance in a process: a fixed-width string that is
// unique per process, which is all a soname needs to be.
std::string instance_tag(uint64_t n);

// Copy Vivado's kernel, the bridge library in use (see Closure) and
// *design_so* (xsim.dir/<snapshot>/xsimk.so) into a fresh private directory
// *outdir*.XXXXXX (mkdtemp), as xsim.dir/<snapshot>/ inside it, under
// per-instance sonames (the characters before ".so" become *tag*, which must be
// shorter than the library's stem). The closure itself stays shared and is
// loaded here if it is not yet.
InstancePaths make_instance(const std::filesystem::path& outdir,
                            const std::string& tag,
                            const std::filesystem::path& design_so);

// Delete the instance's directory once its design is open. The loaded
// libraries stay mapped without a name, nothing is left on disk, and no
// other process can see or overwrite this instance's copies.
void discard_instance(const InstancePaths& p);

} // namespace nvcx
