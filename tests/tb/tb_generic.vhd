library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
use work.dut_comp.all;
entity tb_generic is end;
architecture sim of tb_generic is
    signal clk : std_logic := '0';
    signal u8_in, u8_out, u8_inc : unsigned(7 downto 0) := (others => '0');
    signal u12_in, u12_out, u12_inc : unsigned(11 downto 0) := (others => '0');
    signal s8 : signed(7 downto 0); signal s12 : signed(11 downto 0);
    signal v8 : std_logic_vector(7 downto 0); signal v12 : std_logic_vector(11 downto 0);
begin
    -- the same component, two generic sets: the plugin elaborates one snapshot per set
    narrow : dut generic map (WIDTH => 8)
        port map (clk, '0', u8_in, u8_out, u8_inc, (others => '0'), s8, x"a5", v8);
    wide : dut generic map (WIDTH => 12)
        port map (clk, '0', u12_in, u12_out, u12_inc, (others => '1'), s12, x"abc", v12);
    process
    begin
        for i in 1 to 4 loop
            u8_in <= to_unsigned(i, 8); u12_in <= to_unsigned(1000 + i, 12);
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert u8_out = i and u8_inc = i + 1 report "narrow at " & integer'image(i) severity failure;
            assert u12_out = 1000 + i and u12_inc = 1001 + i report "wide at " & integer'image(i) severity failure;
            assert v8 = x"a5" and v12 = x"abc" and s12 = -1 report "constant inputs" severity failure;
        end loop;
        report "PASS" severity note;
        wait;
    end process;
end;
