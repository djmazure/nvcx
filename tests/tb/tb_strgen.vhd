library ieee; use ieee.std_logic_1164.all;
use work.strgen_comp.all;
entity tb_strgen is end;
architecture sim of tb_strgen is
    signal zero, twelve, ab : std_logic_vector(31 downto 0);
begin
    -- numeric-looking strings, which xelab took as numbers when nvcx passed them unquoted, and
    -- one that never looked numeric
    z : strgen generic map (STR => "0") port map (zero);
    t : strgen generic map (STR => "12") port map (twelve);
    a : strgen generic map (STR => "ab") port map (ab);
    process
    begin
        wait for 1 ns;
        assert zero = x"00000030" report "STR => ""0"": " & to_hstring(zero) severity failure;
        assert twelve = x"00003132" report "STR => ""12"": " & to_hstring(twelve) severity failure;
        assert ab = x"00006162" report "STR => ""ab"": " & to_hstring(ab) severity failure;
        report "PASS" severity note;
        wait;
    end process;
end;
