library ieee; use ieee.std_logic_1164.all;
-- Every change of a produces a one-delta-wide pulse: a zero-time glitch that
-- exists in the core's deltas but not in its settled values.
entity glitch is port (
  a     : in  std_logic;
  pulse : out std_logic);
end;
architecture rtl of glitch is
  signal a_d : std_logic;
begin
  a_d   <= a;
  pulse <= a xor a_d;
end;
