-- Component declaration for strgen.vhd and strgen.v (tests/test_regress.py)
library ieee;
use ieee.std_logic_1164.all;

package strgen_comp is
    component strgen
        generic (STR : string := "none");
        port (bytes : out std_logic_vector(31 downto 0));
    end component;
end package;
