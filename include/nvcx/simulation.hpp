#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "nvcx/kernel.hpp"

namespace nvcx {

enum class Direction {
    None,
    In,
    Out,
    Inout,
};

class Simulation {
  public:
    // A top-level port. Values cross in XSI's layout: VHDL one byte per scalar
    // (its enumeration position; 4-byte integers, 8-byte reals), Verilog
    // {aVal, bVal} word pairs, one pair per 32 bits.
    struct Port {
        int index = 0; // XSI port number
        std::string name;
        Direction dir = Direction::None;
        bool vhdl = true;  // value layout
        unsigned size = 0; // bytes (VHDL) or bits (Verilog), as XSI reports
        size_t raw_size() const {
            return vhdl ? size : ((size_t)size + 31) / 32 * 8;
        }
    };
    enum class RunStatus {
        Completed,
        Finished,
        Error,
    };
    struct RunResult {
        RunStatus status;
        std::string message;
    };

    Simulation(const std::string& design_so, const std::string& kernel_so);
    ~Simulation();
    void close();
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    const std::vector<Port>& ports() const {
        return ports_;
    }
    const Port* port(const std::string& name) const;

    // Time in units of 10^precision() seconds.
    uint64_t time() const {
        return (uint64_t)kernel_->xsi_get_time(handle_);
    }
    int precision() const {
        return kernel_->xsi_get_int(handle_, xsiTimePrecisionKernel);
    }
    // Run *ticks* time units (zero: settle the current time's deltas).
    RunResult run(uint64_t ticks);
    bool finished() const {
        return finished_;
    }
    // Earliest kernel-scheduled event, or false
    bool next_event_time(uint64_t& t) const {
        return kernel_->next_event_time(t);
    }

    // Port values, Port::raw_size() bytes in XSI's layout.
    void get(const Port& p, uint8_t* buf) const;
    void put(const Port& p, const uint8_t* buf);

    // VHDL report/assert messages from the kernel, as it formats them
    // ("Failure: ...\nTime: ..."); the default sink prints them to stdout the
    // way a standalone simulator would.
    void set_report_sink(std::function<void(const std::string&)> sink) {
        report_sink_ = std::move(sink);
    }
    // Severity of such a message, as the sink receives it: 0 note, 1 warning,
    // 2 error, 3 failure.
    static int report_severity(const std::string& msg);

  private:
    std::string last_error();
    void emit_report(const std::string& raw);
    Kernel* kernel_ = nullptr;
    void* design_handle_ = nullptr;
    XsiHandle handle_ = nullptr;
    XsiSetupInfo setup_{};
    std::string design_dir_, xsim_dir_;
    std::vector<Port> ports_;
    bool finished_ = false;
    std::function<void(const std::string&)> report_sink_;
};

// Verilog values in XSI's layout, as logic strings (MSB first, as for VHDL).
std::string vlog_to_logic(const uint8_t* raw, unsigned nbits);
void logic_to_vlog(const std::string& s,
                   uint8_t* raw,
                   unsigned nbits); // 'L'->0, 'H'->1, Z->Z, else X
// std_ulogic by enumeration position, which is also XSI's VHDL byte encoding.
inline constexpr char LOGIC_CHARS[] = "UX01ZWLH-";

} // namespace nvcx
