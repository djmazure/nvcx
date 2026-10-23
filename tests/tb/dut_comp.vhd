-- Component declaration for dut.vhd, as a Xilinx *_VCOMP package would provide it: the generic
-- shell test generates a shell from this and instantiates it with two different WIDTHs.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package dut_comp is
    component dut
        generic (WIDTH : natural := 16);
        port (
            clk     : in  std_logic;
            doom    : in  std_logic;
            u_in    : in  unsigned(WIDTH-1 downto 0);
            u_out   : out unsigned(WIDTH-1 downto 0);
            u_inc   : out unsigned(WIDTH-1 downto 0);
            s_in    : in  signed(WIDTH-1 downto 0);
            s_out   : out signed(WIDTH-1 downto 0);
            slv_in  : in  std_logic_vector(WIDTH-1 downto 0);
            slv_out : out std_logic_vector(WIDTH-1 downto 0)
        );
    end component;
end package;
