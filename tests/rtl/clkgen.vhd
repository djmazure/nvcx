library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
-- A core that generates its own clock and registers a counter on it. Inside
-- the core the clock edge and the counter's update are one delta apart.
entity clkgen is port (
  clk_out : out std_logic;
  cnt_out : out unsigned(7 downto 0));
end;
architecture rtl of clkgen is
  signal clk : std_logic := '0';
  signal cnt : unsigned(7 downto 0) := (others => '0');
begin
  clk <= not clk after 5 ns;
  process(clk) begin
    if rising_edge(clk) then cnt <= cnt + 1; end if;
  end process;
  clk_out <= clk;
  cnt_out <= cnt;
end;
