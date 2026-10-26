// Stand-in for libxv_simbridge_kernel.so, Vivado's bridge to its waveform
// database, to contain the transitive shared-library blast radius (libpython,
// X11, solvers).
#include <cstdint>

extern "C" __attribute__((visibility("default"))) void*
isimBridge_getStandAloneWdbWriter(void) {
    return nullptr;
}

namespace SimBridge {
class __attribute__((visibility("default"))) IEventTarget {
  public:
    virtual ~IEventTarget();
};
IEventTarget::~IEventTarget() = default;
} // namespace SimBridge
