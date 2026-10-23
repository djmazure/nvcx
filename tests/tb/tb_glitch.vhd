library ieee; use ieee.std_logic_1164.all;
entity tb_glitch is end;
architecture sim of tb_glitch is
    signal a, pulse : std_logic := '0';
    signal pulses   : natural := 0;
    component glitch is
        port (a : in std_logic; pulse : out std_logic);
    end component;
begin
    dut : glitch port map (a, pulse);
    -- count the core's zero-time pulses
    process(pulse) begin
        if rising_edge(pulse) then pulses <= pulses + 1; end if;
    end process;
    process
    begin
        wait for 10 ns;
        for i in 1 to 5 loop
            a <= not a;
            wait for 10 ns;
            assert pulse = '0' report "pulse settled high" severity failure;
        end loop;
        assert pulses = 5
            report "zero-time pulses on the core's output: saw " & integer'image(pulses) & " of 5"
            severity failure;
        report "PASS" severity note;
        wait;
    end process;
end;
