#pragma once

#include <filesystem>
#include <string>

namespace nvcx {

struct Closure {
    std::filesystem::path kernel; // Vivado's simulator kernel (not loaded here)
    std::filesystem::path
        bridge; // the bridge library in use: the stub, or Vivado's own
};

// Load the kernel's dependency closure (every library but the kernel itself)
// from $XILINX_VIVADO into the process, once, and describe it. The kernel is
// not loaded: Kernel::get does that.
const Closure& load_closure();

} // namespace nvcx
