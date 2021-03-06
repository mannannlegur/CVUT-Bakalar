-- knihovna typ� pro pr�ci
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

-- definice vstup� a v�stup� - vstupy a, b, c, v�stupy f
entity algebra is
    Port ( en : in std_logic;
			  a2 : in std_logic;
           a1 : in std_logic;
           a0 : in std_logic;
           q2 : out std_logic;
			  q1 : out std_logic;
			  q0 : out std_logic);
end algebra;

architecture Behavioral of algebra is
begin
  q2 <= ((not a0) and a2) or (a2 and (not en)) or (en and a0 and a1);
  q1 <= (a1 and (not en)) or (a1 and (not a0)) or (a0 and en and (not a1) and (not a0));
  q0 <= ((not en) and a0) or (en and (not a0));
  
end Behavioral;
