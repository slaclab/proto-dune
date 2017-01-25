----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interface to the FEMB daq links
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;
use work.WIB_Constants.all;
use work.COLDATA_IO.all;
use work.types.all;

package FEMB_IO is

  -------------------------------------------------------------------------------
  -- DQM
  -------------------------------------------------------------------------------    
  type FEMB_DQM_t is record
    COLDATA_stream : data_8b10b_t(LINK_COUNT downto 1);
  end record FEMB_DQM_t;

  -------------------------------------------------------------------------------
  -- Monitoring
  -------------------------------------------------------------------------------    

  ---------------------------------------
  -- COLDATA ASIC Stream monitor
  type CD_Stream_Monitor_t is record
    convert_delay                  : integer range 0 to 255;
    wait_window                    : std_logic_vector(7 downto 0);
    counter_BUFFER_FULL            : unsigned(31 downto 0);
    counter_CONVERT_IN_WAIT_WINDOW : unsigned(31 downto 0);
    counter_BAD_SOF                : unsigned(31 downto 0);
    counter_UNEXPECTED_EOF         : unsigned(31 downto 0);
    counter_MISSING_EOF            : unsigned(31 downto 0);
    counter_KCHAR_IN_DATA          : unsigned(31 downto 0);
    counter_BAD_CHSUM              : unsigned(31 downto 0);
    counter_packets                : unsigned(31 downto 0);
    data                           : std_logic_vector(8 downto 0);
  end record CD_Stream_Monitor_t;
  type CD_Stream_Monitor_array_t is array (LINKS_PER_FEMB downto 1) of CD_Stream_Monitor_t;

  ---------------------------------------
  -- Fake COLDATA Stream monitor
  type Fake_CD_Monitor_t is record
    counter_packets_A         : unsigned(31 downto 0);
    counter_packets_B         : unsigned(31 downto 0);
    data_A                    : std_logic_vector(8 downto 0);
    data_B                    : std_logic_vector(8 downto 0);

    inject_CD_errors          : std_logic_vector(15 downto 0);
    inject_BAD_checksum       : std_logic_vector(1 downto 0);
    inject_BAD_SOF            : std_logic_vector(1 downto 0);
    inject_LARGE_FRAME        : std_logic_vector(1 downto 0);
    inject_K_CHAR             : std_logic_vector(1 downto 0);
    inject_SHORT_FRAME        : std_logic_vector(1 downto 0);

    set_reserved              : std_logic_vector(15 downto 0);
    set_header                : std_logic_vector(31 downto 0);
    fake_data_type            : std_logic;
    fake_stream_type          : std_logic_vector(LINKS_PER_CDA downto 1);
  end record Fake_CD_Monitor_t;       
  type Fake_CD_Monitor_array_t is array (CDAS_PER_FEMB downto 1) of Fake_CD_Monitor_t;
  
  -----------------------------------------------------------
  -- FEMB monitor
  type FEMB_Monitor_t is record
    CD_Stream         : CD_Stream_Monitor_array_t;    
    Fake_CD           : Fake_CD_Monitor_array_t;
    fake_loopback_en  : std_logic_vector(LINKS_PER_FEMB downto 1);
    reconfig_busy      : std_logic_vector(LINKS_PER_FEMB downto 1);        
    pll_powerdown      : std_logic_vector(LINKS_PER_FEMB downto 1);
    pll_locked         : std_logic_vector(LINKS_PER_FEMB downto 1);
    tx_analogreset     : std_logic_vector(LINKS_PER_FEMB downto 1);      
    tx_digitalreset    : std_logic_vector(LINKS_PER_FEMB downto 1);     
    tx_cal_busy        : std_logic_vector(LINKS_PER_FEMB downto 1);
    tx_ready           : std_logic_vector(LINKS_PER_FEMB downto 1);
    rx_ready           : std_logic_vector(LINKS_PER_FEMB downto 1);         
    rx_analogreset     : std_logic_vector(LINKS_PER_FEMB downto 1);      
    rx_digitalreset    : std_logic_vector(LINKS_PER_FEMB downto 1);     
    rx_cal_busy        : std_logic_vector(LINKS_PER_FEMB downto 1);         
    rx_is_lockedtoref  : std_logic_vector(LINKS_PER_FEMB downto 1);  
    rx_is_lockedtodata : std_logic_vector(LINKS_PER_FEMB downto 1); 
    rx_errdetect       : std_logic_vector(LINKS_PER_FEMB downto 1);       
    rx_disperr         : std_logic_vector(LINKS_PER_FEMB downto 1);         
    rx_runningdisp     : std_logic_vector(LINKS_PER_FEMB downto 1);
    rx_patterndetect   : std_logic_vector(LINKS_PER_FEMB downto 1);
    rx_syncstatus      : std_logic_vector(LINKS_PER_FEMB downto 1);   

  end record FEMB_Monitor_t;
  type FEMB_Monitor_array_t is array (FEMB_COUNT downto 1) of FEMB_Monitor_t;
  
 
  -------------------------------------------------------------------------------
  -- FEMBs Monitor
  type FEMBs_Monitor_t is record
    FEMB                 : FEMB_Monitor_array_t;
    reset              : std_logic;
    reconf_reset       : std_logic;    
  end record FEMBs_Monitor_t;


  -------------------------------------------------------------------------------
  -- Control
  -------------------------------------------------------------------------------    

  ---------------------------------------
  -- COLDATA ASIC Stream control
  type CD_Stream_Control_t is record
    convert_delay                        : integer range 0 to 255;
    wait_window                          : std_logic_vector(7 downto 0);
    reset_counter_BUFFER_FULL            : std_logic;
    reset_counter_CONVERT_IN_WAIT_WINDOW : std_logic;
    reset_counter_BAD_SOF                : std_logic;
    reset_counter_UNEXPECTED_EOF         : std_logic;
    reset_counter_MISSING_EOF            : std_logic;
    reset_counter_KCHAR_IN_DATA          : std_logic;
    reset_counter_BAD_CHSUM              : std_logic;
    reset_counter_packets                : std_logic;  
  end record CD_Stream_Control_t;
  type CD_Stream_Control_array_t is array (LINKS_PER_FEMB downto 1) of CD_Stream_Control_t;
  constant DEFAULT_CD_Stream_Control : CD_Stream_Control_t := (convert_delay                        => 18,
                                                               wait_window                          => x"02",
                                                               reset_counter_BUFFER_FULL            => '0',
                                                               reset_counter_CONVERT_IN_WAIT_WINDOW => '0',
                                                               reset_counter_BAD_SOF                => '0',
                                                               reset_counter_UNEXPECTED_EOF         => '0',
                                                               reset_counter_MISSING_EOF            => '0',
                                                               reset_counter_KCHAR_IN_DATA          => '0',
                                                               reset_counter_BAD_CHSUM              => '0',
                                                               reset_counter_packets                => '0');
  ---------------------------------------
  -- Fake COLDATA Stream control
  type Fake_CD_Control_t is record
    reset_counter_packets_1_A : std_logic;
    reset_counter_packets_1_B : std_logic;

    inject_errors             : std_logic;
    inject_CD_errors          : std_logic_vector(15 downto 0);
    inject_BAD_checksum       : std_logic_vector(1 downto 0);
    inject_BAD_SOF            : std_logic_vector(1 downto 0);
    inject_LARGE_FRAME        : std_logic_vector(1 downto 0);
    inject_K_CHAR             : std_logic_vector(1 downto 0);
    inject_SHORT_FRAME        : std_logic_vector(1 downto 0);
    
    set_reserved              : std_logic_vector(15 downto 0);
    set_header                : std_logic_vector(31 downto 0);
    fake_data_type            : std_logic;
    fake_stream_type          : std_logic_vector(LINKS_PER_CDA downto 1);
  end record Fake_CD_Control_t;       
  type Fake_CD_Control_array_t is array (CDAS_PER_FEMB downto 1) of Fake_CD_Control_t;
  constant DEFAULT_Fake_CD_Control : Fake_CD_Control_t := (reset_counter_packets_1_A => '0',
                                                           reset_counter_packets_1_B => '0',
                                                           inject_errors => '0',
                                                           inject_CD_errors => x"0000",
                                                           inject_BAD_checksum => "00",
                                                           inject_BAD_SOF => "00",
                                                           inject_LARGE_FRAME => "00",
                                                           inject_K_CHAR   => "00",   
                                                           inject_SHORT_FRAME => "00",
                                                           set_reserved => x"0000",
                                                           set_header => x"00000000",
                                                           fake_data_type => '1',
                                                           fake_stream_type => "11");
 
  -----------------------------------------------------------
  -- FEMB control
  type FEMB_Control_t is record
    CD_Stream         : CD_Stream_Control_array_t;    
    Fake_CD           : Fake_CD_Control_array_t;
    fake_loopback_en  : std_logic_vector(LINKS_PER_FEMB downto 1);
  end record FEMB_Control_t;
  type FEMB_Control_array_t is array (FEMB_COUNT downto 1) of FEMB_Control_t;
  constant DEFAULT_FEMB_Control : FEMB_Control_t := (CD_Stream => (others => DEFAULT_CD_Stream_Control),
                                                     Fake_CD   => (others => DEFAULT_Fake_CD_Control),
                                                     fake_loopback_en => (others => '0'));


  -------------------------------------------------------------------------------
  -- FEMBs Control
  type FEMBs_Control_t is record
    FEMB              : FEMB_Control_array_t;
    reset             : std_logic;
    reconf_reset      : std_logic;
  end record FEMBs_Control_t;    
  constant DEFAULT_FEMBs_Control : FEMBs_Control_t := (FEMB => (others => DEFAULT_FEMB_Control),
                                                       reset => '0',
                                                       reconf_reset => '0');
  
end FEMB_IO;
