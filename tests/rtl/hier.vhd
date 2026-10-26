library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity leaf is generic (G : integer := 1); port (clk : in std_logic; d : in std_logic_vector(3 downto 0); q : out std_logic_vector(3 downto 0)); end;
architecture rtl of leaf is
  signal r : std_logic_vector(3 downto 0) := (others => '0');
  constant K : integer := G * 2;
begin
  process(clk) begin if rising_edge(clk) then r <= d; end if; end process;
  q <= r;
end;
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity hier is generic (N : integer := 2; W : natural := 4);
  port (clk : in std_logic := '0'; din : in std_logic_vector(W-1 downto 0) := (others => '0');
        dout : out std_logic_vector(W-1 downto 0); ival : out integer; rval : out real; bval : out boolean);
end;
architecture rtl of hier is
  type arr_t is array (0 to N) of std_logic_vector(W-1 downto 0);
  signal chain : arr_t;
  type rec_t is record a : std_logic; b : unsigned(2 downto 0); end record;
  signal rec : rec_t := ('0', "000");
  signal i_s : integer := 5; signal r_s : real := 1.5; signal b_s : boolean := true;
  signal ev_cnt : natural := 0;
begin
  g: for i in 0 to N-1 generate
    u: entity work.leaf generic map (G => i) port map (clk => clk, d => chain(i), q => chain(i+1));
  end generate;
  chain(0) <= din;
  dout <= chain(N); ival <= i_s; rval <= r_s; bval <= b_s;
  process(clk) begin if rising_edge(clk) then ev_cnt <= ev_cnt + 1; rec.b <= rec.b + 1; end if; end process;
end;
