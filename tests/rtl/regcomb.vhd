library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
-- A registered output and a combinational output derived from it: nq follows
-- q one delta later.
entity regcomb is port (
  clk : in  std_logic;
  d   : in  std_logic_vector(7 downto 0);
  q   : out std_logic_vector(7 downto 0);
  nq  : out std_logic_vector(7 downto 0));
end;
architecture rtl of regcomb is
  signal q_i, q_copy : std_logic_vector(7 downto 0) := (others => '0');
begin
  process(clk) begin
    if rising_edge(clk) then q_i <= d; end if;
  end process;
  q      <= q_i;
  q_copy <= q_i;
  nq     <= not q_copy;
end;
