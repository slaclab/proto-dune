----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interface to the I2C links
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;


package I2C_IO is
  
  type I2C_Monitor_t is record
    wr_strb   : std_logic;
    rd_strb   : std_logic;
    num_bytes : std_logic_vector(3 downto 0);
    address   : std_logic_vector(15 downto 0);
    address16 : std_logic;
    data_in   : std_logic_vector(31 downto 0);
    data_out  : std_logic_vector(31 downto 0);    
    busy      : std_logic;
  end record I2C_Monitor_t;

  
  type I2C_Control_t is record
    wr_strb   : std_logic;
    rd_strb   : std_logic;
    num_bytes : std_logic_vector(3 downto 0);
    address   : std_logic_vector(15 downto 0);
    address16 : std_logic;
    data_in   : std_logic_vector(31 downto 0);
  end record I2C_Control_t;

  --constant used for default settings of the FEControl record
  constant DEFAULT_I2C_control : I2C_Control_t := (wr_strb   => '0',
                                                   rd_strb   => '0',
                                                   num_bytes => (others => '0'),
                                                   address   => (others => '0'),
                                                   address16 => '0',
                                                   data_in   => (others => '0'));
  
end I2C_IO;
