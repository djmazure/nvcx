library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_clkgen is end;
architecture sim of tb_clkgen is
    signal clk_out  : std_logic;
    signal cnt_out  : unsigned(7 downto 0);
    signal captured : unsigned(7 downto 0) := (others => '0');
    component clkgen is
        port (clk_out : out std_logic; cnt_out : out unsigned(7 downto 0));
    end component;
begin
    dut : clkgen port map (clk_out, cnt_out);
    -- A register outside the core, clocked by the core's clock, sampling the
    -- core's counter. The counter is registered on that same edge inside the
    -- core, so the sample must be the value from before the edge.
    process(clk_out) begin
        if rising_edge(clk_out) then captured <= cnt_out; end if;
    end process;
    process
    begin
        for i in 1 to 10 loop
            wait until rising_edge(clk_out);
            wait for 1 ns;
            assert cnt_out = i report "core counter at edge " & integer'image(i) & ": " & integer'image(to_integer(cnt_out)) severity failure;
            assert captured = cnt_out - 1
                report "edge " & integer'image(i) & ": the register clocked by the core's clock sampled its counter after the update (captured "
                       & integer'image(to_integer(captured)) & ", counter " & integer'image(to_integer(cnt_out)) & ")"
                severity failure;
        end loop;
        report "PASS" severity note;
        wait;
    end process;
end;
