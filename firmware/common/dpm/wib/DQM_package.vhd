----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interfaces with the WIB Event builder
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;
use work.WIB_Constants.all;

package DQM_IO is


  -------------------------------------------------------------------------------
  -- Control
  -------------------------------------------------------------------------------
  type DQM_CD_SS_t is record
    stream_number : std_logic;
    CD_number     : std_logic;
    FEMB_number   : std_logic_vector(1 downto 0);
  end record DQM_CD_SS_t;
  constant DEFAULT_DQM_CD_SS : DQM_CD_SS_t := (stream_number => '0',
                                               CD_number => '0',
                                               FEMB_number => "00");
  
  type DQM_Control_t is record
    enable_DQM : std_logic;
    DQM_type : std_logic_vector(3 downto 0);
    CD_SS : DQM_CD_SS_t;
  end record DQM_Control_t;
  constant DEFAULT_DQM_CONTROL : DQM_Control_t := (enable_DQM => '0',
                                                   DQM_type => x"0",
                                                   CD_SS => DEFAULT_DQM_CD_SS);



  -------------------------------------------------------------------------------
  -- Monitoring
  -------------------------------------------------------------------------------
  type DQM_Monitor_t is record
    enable_DQM : std_logic;
    DQM_type : std_logic_vector(3 downto 0);
    CD_SS : DQM_CD_SS_t;
  end record DQM_Monitor_t;
  

  
end DQM_IO;
