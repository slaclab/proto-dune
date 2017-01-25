----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interface to the WIB top level
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;


package WIB_IO is

  
  type WIB_Monitor_t is record
    GLB_i_Reset : std_logic;
    REG_RESET   : std_logic;
    UDP_RESET   : std_logic;
    ALG_RESET   : std_logic;
    sys_locked  : std_logic;
    FEMB_locked : std_logic;
  end record WIB_Monitor_t;

  
  type WIB_Control_t is record
    GLB_i_Reset : std_logic;
    REG_RESET   : std_logic;
    UDP_RESET   : std_logic;
    ALG_RESET   : std_logic;
  end record WIB_Control_t;

  --constant used for default settings of the FEControl record
  constant DEFAULT_WIB_control : WIB_Control_t := (GLB_i_Reset => '0',
                                                   REG_RESET   => '0',
                                                   UDP_RESET   => '0',
                                                   ALG_RESET   => '0');   
end WIB_IO;
