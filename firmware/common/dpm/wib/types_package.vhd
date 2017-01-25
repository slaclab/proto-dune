----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;

package types is
  type uint16_array_t is array (integer range <>) of std_logic_vector(15 downto 0);
  type uint32_array_t is array (integer range <>) of std_logic_vector(31 downto 0);
  type uint36_array_t is array (integer range <>) of std_logic_vector(35 downto 0);
  type uint48_array_t is array (integer range <>) of std_logic_vector(47 downto 0);
  type uint64_array_t is array (integer range <>) of std_logic_vector(63 downto 0);
  
  --passing of 8b10b data
  type data_8b10b_t is array (integer range <> ) of std_logic_vector(8 downto 0);

end types;
