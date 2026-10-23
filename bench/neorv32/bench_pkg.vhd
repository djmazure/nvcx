-- Shared by the wrapper and the testbench (both simulators compile it).
package bench_pkg is
    -- The processor's clock. hello_world runs its UART at 19200 baud from
    -- this frequency, so a low one keeps the character count in cycles small.
    constant clock_hz : natural := 2_000_000;
    constant baud     : natural := 19200;
end package;
