----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interface to the FEMB daq links
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;
use work.COLDATA_IO.all;

package DCC_IO is

  -------------------------------------------------------------------------------
  -- Monitor
  -------------------------------------------------------------------------------

  type DCC_FEMB_CMD_Monitor_t is record
    trigger_enable  : std_logic;
  end record DCC_FEMB_CMD_Monitor_t;

  type DCC_CLK_CFG_Monitor_t is record
    reset_SI5338    : std_logic;
    program_state   : std_logic_vector(7 downto 0);
    clk_switch      : std_logic;
    cmd_switch      : std_logic;
    reset_DUNE_PLL  : std_logic;
    reset_FEMB_PLL  : std_logic;
    locked_DUNE_PLL : std_logic;
    locked_FEMB_PLL : std_logic;
  end record DCC_CLK_CFG_Monitor_t;

  type DCC_Monitor_t is record
    reset_count    : unsigned(23 downto 0);
    convert_count  : unsigned(15 downto 0);
    time_stamp     : unsigned(31 downto 0);
    trigger_enable : std_logic;
    slot_id        : std_logic_vector(2 downto 0);
    crate_id       : std_logic_vector(4 downto 0);
    DUNE_clk_sel   : std_logic;
    CLK_CFG        : DCC_CLK_CFG_Monitor_t;
    FEMB_CMD       : DCC_FEMB_CMD_Monitor_t;
  end record DCC_Monitor_t;

  -------------------------------------------------------------------------------
  -- Control
  -------------------------------------------------------------------------------

  type DCC_FEMB_CMD_Control_t is record
    trigger_enable       : std_logic;
    inject_calibrate     : std_logic;
    inject_sync          : std_logic;
    inject_COLDATA_reset : std_logic;
  end record DCC_FEMB_CMD_Control_t;
  constant DEFAULT_DCC_FEMB_CMD_Control : DCC_FEMB_CMD_Control_t := (trigger_enable       => '0',
                                                                     inject_calibrate     => '0',
                                                                     inject_sync          => '0',
                                                                     inject_COLDATA_reset => '0');

  type DCC_CLK_CFG_Control_t is record
    reset_SI5338    : std_logic;
    clk_switch      : std_logic;
    cmd_switch      : std_logic;
    reset_DUNE_PLL  : std_logic;
    reset_FEMB_PLL  : std_logic;
    locked_DUNE_PLL : std_logic;
    locked_FEMB_PLL : std_logic;

  end record DCC_CLK_CFG_Control_t;
  constant DEFAULT_DCC_CLK_CFG_control : DCC_CLK_CFG_Control_t := (reset_SI5338    => '0',
                                                                   clk_switch      => '0',
                                                                   cmd_switch      => '0',
                                                                   reset_DUNE_PLL  => '0',
                                                                   reset_FEMB_PLL  => '0',
                                                                   locked_DUNE_PLL => '0',
                                                                   locked_FEMB_PLL => '0'
                                                                   );

  type DCC_Control_t is record
    slot_id      : std_logic_vector(2 downto 0);
    crate_id     : std_logic_vector(4 downto 0);
    DUNE_clk_sel : std_logic;
    CLK_CFG      : DCC_CLK_CFG_Control_t;
    FEMB_CMD     : DCC_FEMB_CMD_Control_t;
  end record DCC_Control_t;

  --constant used for default settings 
  constant DEFAULT_DCC_control : DCC_Control_t := (slot_id      => "000",
                                                   crate_id     => "00000",
                                                   DUNE_clk_sel => '0',
                                                   CLK_CFG      => DEFAULT_DCC_CLK_CFG_control,
                                                   FEMB_CMD     => DEFAULT_DCC_FEMB_CMD_control);

end DCC_IO;
