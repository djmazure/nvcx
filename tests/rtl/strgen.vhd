-- A string generic's bytes on a port, right-justified as Verilog would (tests/test_regress.py)
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity strgen is
generic (STR : string := "none");
port (bytes : out std_logic_vector(31 downto 0));
end strgen;

architecture behav of strgen is
    function pack(s : string) return std_logic_vector is
        variable r : std_logic_vector(31 downto 0) := (others => '0');
    begin
        for i in s'range loop
            r := r(23 downto 0) & std_logic_vector(to_unsigned(character'pos(s(i)), 8));
        end loop;
        return r;
    end function;
begin
    bytes <= pack(STR);
end behav;
