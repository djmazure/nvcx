-- A report at each severity, all at the first clock edge (tests/test_nvc.py)
library ieee;
use ieee.std_logic_1164.all;

entity sev is
port (clk : in std_logic;
      q   : out std_logic := '0');
end sev;

architecture behav of sev is
begin
    process(clk) begin
        if rising_edge(clk) then
            report "sev note" severity note;
            report "sev warning" severity warning;
            report "sev error" severity error;
            report "sev failure" severity failure;
            q <= '1';
        end if;
    end process;
end behav;
