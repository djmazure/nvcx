-- The processor behind a fixed configuration and the four ports the bench
-- needs. Natively NVC binds this entity; in the mixed mode an nvcx shell of
-- the same name stands in for it and xsim simulates this as its snapshot.
library ieee;
use ieee.std_logic_1164.all;
library neorv32;
use work.bench_pkg.all;

entity soc is
    port (
        clk_i       : in  std_ulogic;
        rstn_i      : in  std_ulogic;
        uart0_txd_o : out std_ulogic;
        uart0_rxd_i : in  std_ulogic
    );
end entity;

architecture rtl of soc is
begin
    processor : entity neorv32.neorv32_top
        generic map (
            CLOCK_FREQUENCY  => clock_hz,
            BOOT_MODE_SELECT => 0, -- the bootloader, with its UART console
            RISCV_ISA_Zicntr => true,
            IMEM_EN          => true,
            IMEM_SIZE        => 16 * 1024,
            DMEM_EN          => true,
            DMEM_SIZE        => 8 * 1024,
            IO_CLINT_EN      => true,
            IO_UART0_EN      => true
        )
        port map (
            clk_i       => clk_i,
            rstn_i      => rstn_i,
            uart0_txd_o => uart0_txd_o,
            uart0_rxd_i => uart0_rxd_i
        );
end architecture;
