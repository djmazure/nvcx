library ieee; use ieee.std_logic_1164.all;
entity tb_sev is end;
architecture sim of tb_sev is
    signal clk, q : std_logic := '0';
    component sev is
        port (clk : in std_logic; q : out std_logic);
    end component;
begin
    -- the core reports at each severity on the clock edge: each must arrive as an NVC assertion of
    -- that severity, and the first at or above --exit-severity must end the simulation
    s : sev port map (clk => clk, q => q);
    process
    begin
        wait for 1 ns;
        clk <= '1';
        wait for 5 ns;
        report "FAIL: the core's reports did not stop the simulation" severity failure;
    end process;
end;
