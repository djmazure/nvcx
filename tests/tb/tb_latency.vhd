library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_latency is end;
architecture sim of tb_latency is
    signal clk : std_logic := '0';
    signal d, q, nq : std_logic_vector(7 downto 0) := (others => '0');
    signal tick   : bit := '0';      -- toggles every delta while counting
    signal deltas : integer := -1;   -- deltas since the clock edge; -1: idle
    signal result : integer := 0;    -- deltas from the edge to q's change
    component regcomb is
        port (clk : in std_logic; d : in std_logic_vector(7 downto 0); q, nq : out std_logic_vector(7 downto 0));
    end component;
begin
    dut : regcomb port map (clk, d, q, nq);
    -- Count the deltas between the clock edge and q's change. Inside the core
    -- q_i updates one delta after the edge and q one delta after that: two.
    process(clk, tick, q)
    begin
        if rising_edge(clk) then
            deltas <= 0;
            tick <= not tick;
        elsif q'event and deltas >= 0 then
            result <= deltas + 1;
            deltas <= -1;
        elsif tick'event and deltas >= 0 and deltas < 100 then
            deltas <= deltas + 1;
            tick <= not tick;
        end if;
    end process;
    process
    begin
        for i in 1 to 5 loop
            d <= std_logic_vector(to_unsigned(i, 8));
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert q = std_logic_vector(to_unsigned(i, 8)) report "q at cycle " & integer'image(i) severity failure;
            assert result = 2
                report "cycle " & integer'image(i) & ": q changed " & integer'image(result) & " deltas after the clock edge (in one simulator: 2)"
                severity failure;
        end loop;
        report "PASS" severity note;
        wait;
    end process;
end;
