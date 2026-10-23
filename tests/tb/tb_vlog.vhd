library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_vlog is end;
architecture sim of tb_vlog is
    signal clk, doom : std_logic := '0';
    signal u_in, u_out, u_inc, s_in, s_out, slv_in, slv_out : std_logic_vector(15 downto 0) := (others => '0');
    -- the Verilog core as VHDL sees it; nvcx::write_shell builds the shell from this
    component dut_v is
        port (clk, doom : in std_logic;
              u_in, s_in, slv_in : in std_logic_vector(15 downto 0);
              u_out, u_inc, s_out, slv_out : out std_logic_vector(15 downto 0));
    end component;
begin
    -- named association: xsim orders a non-ANSI Verilog port list differently between releases
    dut : dut_v port map (clk => clk, doom => doom, u_in => u_in, u_out => u_out, u_inc => u_inc,
                          s_in => s_in, s_out => s_out, slv_in => slv_in, slv_out => slv_out);
    process
    begin
        for i in 1 to 4 loop
            u_in <= std_logic_vector(to_unsigned(i, 16)); slv_in <= (others => 'Z'); s_in <= x"FFFF";
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert unsigned(u_out) = i and unsigned(u_inc) = i + 1 report "verilog core at " & integer'image(i) severity failure;
            assert slv_out = (15 downto 0 => 'Z') report "Z did not round-trip: " & to_hstring(slv_out) severity failure;
            assert s_out = x"FFFF" report "s_out" severity failure;
        end loop;
        report "PASS" severity note;
        wait;
    end process;
end;
