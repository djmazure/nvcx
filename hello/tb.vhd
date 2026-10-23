library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.finish;
library xpm;
use xpm.vcomponents.all;

entity tb is
end entity;

architecture sim of tb is
    signal clk : std_logic := '0';
    signal aresetn : std_logic := '0';
    signal tvalid : std_logic;
    signal tdata, doutb, sample : std_logic_vector(15 downto 0);

    constant DEPTH : natural := 256;
    type samples_t is array (0 to DEPTH-1) of std_logic_vector(15 downto 0);
    signal expected : samples_t;

    signal wea : std_logic_vector(0 downto 0) := "0";
    signal addra, addrb : unsigned(7 downto 0) := (others => '0');

    -- the DDS as its instantiation template declares it; NVC cannot bind it, nvcx provides it
    component hello_dds is
        port (aclk : in std_logic; aresetn : in std_logic;
              m_axis_data_tvalid : out std_logic; m_axis_data_tdata : out std_logic_vector(15 downto 0));
    end component;
begin

    clk <= not clk after 5 ns;

    -- NVC can't simulate encrypted IP on its own
    dds : hello_dds
    port map (
        aclk => clk,
        aresetn => aresetn,
        m_axis_data_tvalid => tvalid,
        m_axis_data_tdata => tdata);

    -- ...or SystemVerilog with generics (yet)
    ram : xpm_memory_sdpram
    generic map (
        MEMORY_SIZE        => DEPTH * sample'length,
        CLOCKING_MODE      => "common_clock",
        WRITE_DATA_WIDTH_A => 16,
        BYTE_WRITE_WIDTH_A => 16,
        ADDR_WIDTH_A       => 8,
        READ_DATA_WIDTH_B  => 16,
        ADDR_WIDTH_B       => 8,
        READ_LATENCY_B     => 1)
    port map (
        sleep => '0',

        clka => clk,
        ena => '1',
        wea => wea,
        addra => std_logic_vector(addra),
        dina => sample,
        injectsbiterra => '0',
        injectdbiterra => '0',

        clkb => clk,
        rstb => '0',
        enb => '1',
        regceb => '1',
        addrb => std_logic_vector(addrb),
        doutb => doutb);

    check : process
        variable cosine, sine, magnitude2 : integer;
    begin
        wait for 100 ns;
        aresetn <= '1';
        wait until tvalid = '1';

        -- record DEPTH samples into the RAM and the array
        for i in 0 to DEPTH - 1 loop
            wait until rising_edge(clk) and tvalid = '1';
            cosine := to_integer(signed(tdata(7 downto 0)));
            sine   := to_integer(signed(tdata(15 downto 8)));

            if i < 4 then
                report "sample "
                    & integer'image(i)
                    & ": cos="
                    & integer'image(cosine)
                    & " sin="
                    & integer'image(sine);
            end if;

            magnitude2 := cosine * cosine + sine * sine;
            assert magnitude2 > 110 * 110 and magnitude2 < 135 * 135
                report "sample "
                    & integer'image(i)
                    & " off the unit circle"
                severity failure;

            -- the RAM takes this sample at the next edge, from the registered copy
            sample <= tdata;
            expected(i) <= tdata;
            addra <= to_unsigned(i, 8);
            wea <= "1";
        end loop;

        wait until rising_edge(clk);
        wea <= "0";

        -- read every entry back through port B and compare
        for i in 0 to DEPTH - 1 loop
            addrb <= to_unsigned(i, 8);
            wait until rising_edge(clk); -- address taken
            wait until rising_edge(clk); -- READ_LATENCY_B = 1
            assert doutb = expected(i)
                report "RAM entry "
                    & integer'image(i)
                    & " read back "
                    & to_hstring(doutb)
                    & ", expected "
                    & to_hstring(expected(i))
                severity failure;
        end loop;

        report "PASS: " & integer'image(DEPTH);

        finish;
    end process;
end architecture;
