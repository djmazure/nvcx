library ieee; use ieee.std_logic_1164.all;
package kinds_pkg is
  signal pkg_sig : std_logic := '0';
  constant PKG_C : integer := 7;
  shared variable pkg_var : integer := 1;
end package;
library ieee; use ieee.std_logic_1164.all; use work.kinds_pkg.all;
entity kinds is port (clk : in std_logic := '0'; q : out std_logic := '0'); end;
architecture rtl of kinds is
  signal s : std_logic := '0';
  shared variable sv : integer := 3;
  constant C : integer := 5;
  alias a_s : std_logic is s;
begin
  blk: block
    signal bs : std_logic := '1';
  begin
    bs <= not bs after 7 ns;
  end block;
  ifg: if C > 1 generate
    signal gs : std_logic := '0';
  begin
    gs <= clk;
  end generate;
  p1: process(clk) variable v : integer := 0; begin if rising_edge(clk) then v := v + 1; s <= not s; end if; end process;
  q <= s and pkg_sig;
end;
