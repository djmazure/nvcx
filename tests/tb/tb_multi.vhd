library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_multi is end;
architecture sim of tb_multi is
    signal clk : std_logic := '0';
    signal u1_in, u2_in, u1_out, u2_out, u1_inc, u2_inc : unsigned(15 downto 0) := (others => '0');
    signal s_out1, s_out2 : signed(15 downto 0);
    signal slv_out1, slv_out2 : std_logic_vector(15 downto 0);
    signal a, y_comb, q : std_logic_vector(7 downto 0) := (others => '0');
    signal cnt_out : unsigned(7 downto 0);
    component dut is
        generic (WIDTH : natural := 16);
        port (clk, doom : in std_logic := '0';
              u_in : in unsigned(WIDTH-1 downto 0) := (others => '0');
              u_out, u_inc : out unsigned(WIDTH-1 downto 0);
              s_in : in signed(WIDTH-1 downto 0) := (others => '0');
              s_out : out signed(WIDTH-1 downto 0);
              slv_in : in std_logic_vector(WIDTH-1 downto 0) := (others => '0');
              slv_out : out std_logic_vector(WIDTH-1 downto 0));
    end component;
    component exp is
        port (clk : in std_logic := '0'; a, b : in std_logic_vector(7 downto 0) := (others => '0');
              y_comb, q : out std_logic_vector(7 downto 0); cnt_out : out unsigned(7 downto 0));
    end component;
begin
    -- two instances of one snapshot and one of another: three private xsim kernels in one process
    d1 : dut port map (clk => clk, doom => '0', u_in => u1_in, u_out => u1_out, u_inc => u1_inc,
                                   s_in => (others => '0'), s_out => s_out1, slv_in => x"1234", slv_out => slv_out1);
    d2 : dut port map (clk => clk, doom => '0', u_in => u2_in, u_out => u2_out, u_inc => u2_inc,
                                   s_in => (others => '1'), s_out => s_out2, slv_in => x"abcd", slv_out => slv_out2);
    e  : exp port map (clk, a, x"ff", y_comb, q, cnt_out);
    process
    begin
        for i in 1 to 5 loop
            u1_in <= to_unsigned(i, 16); u2_in <= to_unsigned(100 * i, 16); a <= std_logic_vector(to_unsigned(i, 8));
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert u1_out = i and u1_inc = i + 1 report "d1 at " & integer'image(i) severity failure;
            assert u2_out = 100 * i and u2_inc = 100 * i + 1 report "d2 at " & integer'image(i) severity failure;
            assert slv_out1 = x"1234" and slv_out2 = x"abcd" and s_out2 = -1 report "constant inputs" severity failure;
            assert q = std_logic_vector(to_unsigned(i, 8)) and y_comb = (std_logic_vector(to_unsigned(i, 8)) xor x"ff")
                report "exp at " & integer'image(i) severity failure;
        end loop;
        assert cnt_out = 5 report "oscillator: " & integer'image(to_integer(cnt_out)) severity failure;
        report "PASS" severity note;
        wait;
    end process;
end;
