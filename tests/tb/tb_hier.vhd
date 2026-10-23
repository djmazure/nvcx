library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_hier is end;
architecture sim of tb_hier is
    signal clk : std_logic := '0';
    signal din, dout : std_logic_vector(3 downto 0) := x"a";
    signal ival : integer; signal rval : real; signal bval : boolean;
    component hier is
        generic (N : integer := 2; W : natural := 4);
        port (clk : in std_logic := '0'; din : in std_logic_vector(W-1 downto 0) := (others => '0');
              dout : out std_logic_vector(W-1 downto 0); ival : out integer; rval : out real; bval : out boolean);
    end component;
begin
    dut : hier port map (clk, din, dout, ival, rval, bval);
    process
    begin
        wait for 1 ns;
        assert ival = 5 report "integer port: " & integer'image(ival) severity failure;
        assert rval = 1.5 report "real port: " & real'image(rval) severity failure;
        assert bval report "boolean port" severity failure;
        report "PASS" severity note;
        wait;
    end process;
end;
