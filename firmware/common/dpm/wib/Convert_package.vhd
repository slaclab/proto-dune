----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for the convert signals and meta-data from the DCC
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;

package Convert_IO is
  constant COLDATA_CONVERT_PERIOD_128MHZ : integer := 62;  --640; --
  
  type convert_t is record
    trigger       : std_logic;
    reset_count   : unsigned(23 downto 0);
    convert_count : unsigned(15 downto 0);
    time_stamp    : unsigned(31 downto 0);
    slot_id       : std_logic_vector(2 downto 0);
    crate_id      : std_logic_vector(4 downto 0);
  end record convert_t;

  type convert_simple_t is record
    trigger       : std_logic;
    reset_count   : unsigned(23 downto 0);
    convert_count : unsigned(15 downto 0);  
    slot_id       : std_logic_vector(2 downto 0);
    crate_id      : std_logic_vector(4 downto 0);
  end record convert_simple_t;
  
end Convert_IO;
