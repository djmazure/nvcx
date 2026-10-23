-- Clock and reset for the processor, and a UART terminal: it prints what the
-- bootloader says and types back. It aborts the auto-boot, asks for the system
-- info ('i') and the help text ('h'), and ends the run on the last help line.
-- The lines and the cycle count go to uart0.txt, which must come out the same
-- in every mode.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use std.env.finish;
use work.bench_pkg.all;

entity tb is
end entity;

architecture sim of tb is
    component soc is
        port (
            clk_i       : in  std_ulogic;
            rstn_i      : in  std_ulogic;
            uart0_txd_o : out std_ulogic;
            uart0_rxd_i : in  std_ulogic
        );
    end component;

    constant period   : time := 1 sec / clock_hz;
    constant bit_time : time := 1 sec / baud;

    signal clk    : std_ulogic := '0';
    signal rstn   : std_ulogic := '0';
    signal txd    : std_ulogic;
    signal rxd    : std_ulogic := '1';
    signal cycles : natural := 0;
    -- the terminal's keystrokes: the receiver asks, the sender types
    signal key    : character := NUL;
    signal keys   : natural := 0;
begin
    dut : soc port map (clk, rstn, txd, rxd);

    clk  <= not clk after period / 2;
    rstn <= '1' after 10 * period;

    count : process (clk)
    begin
        if rising_edge(clk) then
            cycles <= cycles + 1;
        end if;
    end process;

    -- types a key whenever the receiver asks for one
    sender : process
        variable b : std_ulogic_vector(7 downto 0);
    begin
        wait on keys;
        b := std_ulogic_vector(to_unsigned(character'pos(key), 8));
        rxd <= '0'; -- start bit
        wait for bit_time;
        for i in 0 to 7 loop
            rxd <= b(i);
            wait for bit_time;
        end loop;
        rxd <= '1'; -- stop bit
        wait for bit_time;
    end process;

    receiver : process
        file log : text open write_mode is "uart0.txt";
        variable l    : line;
        variable byte : std_ulogic_vector(7 downto 0);
        variable c    : character;
        variable s    : string(1 to 256);
        variable n    : natural := 0;

        -- what the bootloader has to say before the terminal types
        function prompts(line : string) return character is
        begin
            if line = "Auto-boot in 8s. Press any key to abort." then
                return 'x';
            elsif line = "Type 'h' for help." then
                return 'i';
            elsif line'length > 5 and line(1 to 5) = "MISC:" then
                return 'h';
            end if;
            return NUL;
        end function;
    begin
        wait until txd = '0' and txd'event; -- start bit
        wait for bit_time * 1.5;
        for i in 0 to 7 loop
            byte(i) := txd;
            wait for bit_time;
        end loop;
        c := character'val(to_integer(unsigned(byte)));
        if c = LF then
            report "uart0: " & s(1 to n);
            write(l, s(1 to n));
            writeline(log, l);
            if s(1 to n) = "e: Start executable" then
                write(l, string'("cycles: ") & integer'image(cycles));
                writeline(log, l);
                report "done after " & integer'image(cycles) & " cycles";
                finish;
            end if;
            c := prompts(s(1 to n));
            n := 0;
            if c /= NUL then
                key <= c;
                keys <= keys + 1;
            end if;
        elsif c /= CR and n < s'length then
            n := n + 1;
            s(n) := c;
        end if;
    end process;
end architecture;
