library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity exp is port (
  clk     : in  std_logic := '0';
  a, b    : in  std_logic_vector(7 downto 0) := (others => '0');
  y_comb  : out std_logic_vector(7 downto 0);
  q       : out std_logic_vector(7 downto 0);
  cnt_out : out unsigned(7 downto 0));
end;
architecture rtl of exp is
  signal clk_int : std_logic := '0';
  signal cnt     : unsigned(7 downto 0) := (others => '0');
  signal q_i     : std_logic_vector(7 downto 0) := (others => '0');
  type mem_t is array (0 to 3) of std_logic_vector(7 downto 0);
  signal mem     : mem_t := (others => (others => '0'));
begin
  clk_int <= not clk_int after 5 ns;
  y_comb  <= a xor b;
  process(clk) begin
    if rising_edge(clk) then q_i <= a; mem(0) <= a; end if;
  end process;
  q <= q_i;
  process(clk_int) begin
    if rising_edge(clk_int) then cnt <= cnt + 1; end if;
  end process;
  cnt_out <= cnt;
end;
