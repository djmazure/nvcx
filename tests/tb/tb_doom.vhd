library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity tb_doom is end;
architecture sim of tb_doom is
    signal clk : std_logic := '0';
    signal u_out, u_inc : unsigned(15 downto 0);
    signal s_out : signed(15 downto 0);
    signal slv_out : std_logic_vector(15 downto 0);
    component dut is
        generic (WIDTH : natural := 16);
        port (clk, doom : in std_logic := '0';
              u_in : in unsigned(WIDTH-1 downto 0) := (others => '0');
              u_out, u_inc : out unsigned(WIDTH-1 downto 0);
              s_in : in signed(WIDTH-1 downto 0) := (others => '0');
              s_out : out signed(WIDTH-1 downto 0);
              slv_in : in std_logic_vector(WIDTH-1 downto 0) := (others => '0');
              slv_out : out std_logic_vector(WIDTH-1 downto 0));
    end component;
begin
    -- the core asserts doom = '0' at every clock edge, severity failure: the kernel's report must
    -- surface as an NVC assertion and end the simulation
    d : dut port map (clk => clk, doom => '1', u_in => x"0001", u_out => u_out, u_inc => u_inc,
                                  s_in => (others => '0'), s_out => s_out, slv_in => x"1234", slv_out => slv_out);
    process
    begin
        wait for 1 ns;
        clk <= '1';
        wait for 5 ns;
        report "FAIL: the core's assertion did not stop the simulation" severity failure;
    end process;
end;
