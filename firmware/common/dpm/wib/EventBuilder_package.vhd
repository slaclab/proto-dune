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
use work.FEMB_GB_IO.all;

package EB_IO is

  
  type FEMB_stream_t is record
    valid : std_logic;
    capture_errors : std_logic_vector( 7 downto 0);
    CD_errors      : std_logic_vector( 7 downto 0);
    CD_timestamp   : std_logic_vector( 7 downto 0);    
    data_out       : std_logic_vector(15 downto 0);
  end record FEMB_stream_t;
  type FEMB_Stream_array_t is array (integer range <>) of FEMB_Stream_t;

  -------------------------------------------------------------------------------
  -- Control
  -------------------------------------------------------------------------------
  type FEMB_EB_Control_t is record
   COLDATA_en        : std_logic_vector(LINKS_PER_FEMB -1 downto 0);
   event_count_reset : std_logic;

   spy_buffer_wait_for_trigger : std_logic;
   spy_buffer_start  : std_logic;
   spy_buffer_read   : std_logic;

   debug             : std_logic;
   
   gearbox           : FEMB_GB_Control_t;
  end record FEMB_EB_Control_t;
  type FEMB_EB_Control_array_t is array (integer range <>) of FEMB_EB_Control_t;
  constant DEFAULT_FEMB_EB_CONTROL : FEMB_EB_CONTROL_t := (COLDATA_en => x"F",
                                                           event_count_reset => '0',
                                                           spy_buffer_wait_for_trigger => '0',
                                                           spy_buffer_start => '0',
                                                           spy_buffer_read => '0',
                                                           debug => '0',
                                                           gearbox => DEFAULT_FEMB_GB_CONTROL
                                                           );  
  type EB_Control_t is record
    FEMB_EB              : FEMB_EB_Control_array_t(FEMB_COUNT downto 1);
    reset                : std_logic;
    reconf_reset         : std_logic;
    rx_event_count_reset : std_logic;
    P_POD_reset          : std_logic;
  end record EB_Control_t;
  constant DEFAULT_EB_CONTROL : EB_Control_t := (FEMB_EB => (others => DEFAULT_FEMB_EB_CONTROL) ,
                                                 reset => '0',
                                                 reconf_reset => '0',
                                                 rx_event_count_reset => '0',
                                                 P_POD_reset => '1');



  -------------------------------------------------------------------------------
  -- Monitoring
  -------------------------------------------------------------------------------
  
  type FEMB_EB_Monitor_t is record
    COLDATA_en   : std_logic_vector(LINKS_PER_FEMB -1 downto 0);
    fiber_number : std_logic_vector(1 downto 0);
    event_count  : unsigned(31 downto 0);
    
    spy_buffer_data    : std_logic_vector(35 downto 0);
    spy_buffer_empty   : std_logic;
    spy_buffer_running : std_logic;
    spy_buffer_wait_for_trigger : std_logic;

    debug             : std_logic;
    
    gearbox            : FEMB_GB_Monitor_t;
  end record FEMB_EB_Monitor_t;
  type FEMB_EB_Monitor_array_t is array (integer range <>) of FEMB_EB_Monitor_t;

  type EB_Monitor_t is record
    FEMB_EB            : FEMB_EB_Monitor_array_t(FEMB_Count downto 1);

    reset              : std_logic;
    reconf_reset       : std_logic;
    reconfig_busy      : std_logic;
    pll_powerdown      : std_logic_vector(FEMB_COUNT-1 downto 0);
    pll_locked         : std_logic_vector(FEMB_COUNT-1 downto 0);
    tx_analogreset     : std_logic_vector(FEMB_COUNT-1 downto 0);
    tx_digitalreset    : std_logic_vector(FEMB_COUNT-1 downto 0);
    tx_cal_busy        : std_logic_vector(FEMB_COUNT-1 downto 0);
    tx_ready           : std_logic_vector(FEMB_COUNT-1 downto 0);

    rx_ready           : std_logic;
    rx_analogreset     : std_logic;
    rx_digitalreset    : std_logic;
    rx_is_lockedtoref  : std_logic;
    rx_is_lockedtodata : std_logic;
    rx_cal_busy        : std_logic;
    rx_errdetect       : std_logic_vector(3 downto 0);
    rx_runningdisp     : std_logic_vector(3 downto 0);
    rx_patterndetect   : std_logic_vector(3 downto 0);
    rx_syncstatus      : std_logic_vector(3 downto 0);
    
    rx_event_count     : unsigned(31 downto 0);
    P_POD_reset        : std_logic;

  end record EB_Monitor_t;
  

  
end EB_IO;
