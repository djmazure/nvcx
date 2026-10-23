library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_regcomb is end;
architecture sim of tb_regcomb is
    signal clk : std_logic := '0';
    signal d, q, nq : std_logic_vector(7 downto 0) := (others => '0');
    signal nq_at_q  : std_logic_vector(7 downto 0);
    component regcomb is
        port (clk : in std_logic; d : in std_logic_vector(7 downto 0); q, nq : out std_logic_vector(7 downto 0));
    end component;
begin
    dut : regcomb port map (clk, d, q, nq);
    -- what nq shows in the delta in which q changes: nq derives from q one
    -- delta later, so it is still the old value then
    process(q) begin nq_at_q <= nq; end process;
    process
        variable prev : integer := 0;
    begin
        for i in 1 to 10 loop
            d <= std_logic_vector(to_unsigned(i, 8));
            wait for 1 ns;
            clk <= '1'; wait for 5 ns; clk <= '0'; wait for 4 ns;
            assert q = std_logic_vector(to_unsigned(i, 8)) report "q at cycle " & integer'image(i) severity failure;
            assert nq = not std_logic_vector(to_unsigned(i, 8)) report "nq at cycle " & integer'image(i) severity failure;
            assert nq_at_q = not std_logic_vector(to_unsigned(prev, 8))
                report "cycle " & integer'image(i) & ": nq changed in the same delta as q (" & to_hstring(nq_at_q) & ", expected " & to_hstring(not std_logic_vector(to_unsigned(prev, 8))) & ")"
                severity failure;
            prev := i;
        end loop;
        report "PASS" severity note;
        wait;
    end process;
end;
