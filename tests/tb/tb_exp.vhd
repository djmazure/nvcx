library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_exp is end;
architecture sim of tb_exp is
    signal clk : std_logic := '0';
    signal a, b, y_comb, q : std_logic_vector(7 downto 0) := (others => '0');
    signal cnt_out : unsigned(7 downto 0);
    component exp is
        port (clk : in std_logic := '0'; a, b : in std_logic_vector(7 downto 0) := (others => '0');
              y_comb, q : out std_logic_vector(7 downto 0); cnt_out : out unsigned(7 downto 0));
    end component;
begin
    dut : exp port map (clk, a, b, y_comb, q, cnt_out);
    process
    begin
        a <= x"0f"; b <= x"f0";
        wait for 1 ns;
        assert y_comb = x"ff" report "comb path: y_comb = " & to_hstring(y_comb) severity failure;
        assert q = x"00" report "q before any edge" severity failure;
        for i in 0 to 9 loop
            a <= std_logic_vector(to_unsigned(i, 8));
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert q = std_logic_vector(to_unsigned(i, 8)) report "registered path at cycle " & integer'image(i) severity failure;
            assert y_comb = (std_logic_vector(to_unsigned(i, 8)) xor x"f0") report "comb path at cycle " & integer'image(i) severity failure;
        end loop;
        -- the core's own 10 ns oscillator has run free the whole time: 101 ns -> 10 rising edges
        assert cnt_out = 10 report "internal oscillator: cnt_out = " & integer'image(to_integer(cnt_out)) severity failure;
        wait for 100 ns;
        assert cnt_out = 20 report "oscillator while idle: cnt_out = " & integer'image(to_integer(cnt_out)) severity failure;
        report "PASS" severity note;
        wait;
    end process;
end;
