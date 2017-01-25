-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmPkg.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2016-11-01
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description:
-------------------------------------------------------------------------------
-- This file is part of 'DUNE Development Firmware'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'DUNE Development Firmware', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.StdRtlPkg.all;
use work.AxiStreamPkg.all;
use work.RceG3Pkg.all;

package ProtoDuneDpmPkg is

   constant RSSI_PORTS_C : PositiveArray(0 downto 0) := (0 => 8192);

   -- WIB 8B10B Commas
   constant K28_5_C : slv(7 downto 0) := "10111100";  -- K28.5, 0xBC
   constant K28_1_C : slv(7 downto 0) := "00111100";  -- K28.1, 0x3C
   constant K28_2_C : slv(7 downto 0) := "01011100";  -- K28.2, 0x5C

   constant WIB_SIZE_C : natural := 2;
   constant WIB_AXIS_CONFIG_C : AxiStreamConfigType := (
      TSTRB_EN_C    => RCEG3_AXIS_DMA_CONFIG_C.TSTRB_EN_C,
      TDATA_BYTES_C => 2,               -- 16-bit data bus
      TDEST_BITS_C  => RCEG3_AXIS_DMA_CONFIG_C.TDEST_BITS_C,
      TID_BITS_C    => RCEG3_AXIS_DMA_CONFIG_C.TID_BITS_C,
      TKEEP_MODE_C  => RCEG3_AXIS_DMA_CONFIG_C.TKEEP_MODE_C,
      TUSER_BITS_C  => RCEG3_AXIS_DMA_CONFIG_C.TUSER_BITS_C,
      TUSER_MODE_C  => RCEG3_AXIS_DMA_CONFIG_C.TUSER_MODE_C);   

   -- First AXIS word side-band indexes
   constant WIB_SOF_C    : integer := 1;  -- Driven every frame
   constant WIB_RUN_EN_C : integer := 3;  -- Timing or emulation driven

   -- Last AXIS word side-band indexes
   constant WIB_EOFE_C  : integer := 0;  -- Driven if frame length error or CRC error detected
   constant WIB_FLUSH_C : integer := 2;  -- Software driven     

   constant EMU_DELAY_C      : positive := 5;  -- 5 = SRLC32E per ADC delay module
   constant EMU_DELAY_TAPS_C : positive := 2**EMU_DELAY_C;
   type EmuDlyCfg is array (127 downto 0) of slv(EMU_DELAY_C-1 downto 0);

   type ProtoDuneDpmConfigType is record
      emuFebSelect : sl;
      emuClkSel    : sl;
      softRst      : sl;
      hardRst      : sl;
      debugStart   : sl;
      debugStop    : sl;
   end record;
   constant PROTO_DUNE_DPM_CONFIG_INIT_C : ProtoDuneDpmConfigType := (
      emuFebSelect => '0',
      emuClkSel    => '0',
      softRst      => '0',
      hardRst      => '0',
      debugStart   => '0',
      debugStop    => '0');      

   type ProtoDuneDpmStatusType is record
      streaming     : sl;
      extTrig       : sl;
      timeStamp     : slv(55 downto 0);
      trigTimeStamp : slv(55 downto 0);
   end record;
   constant PROTO_DUNE_DPM_STATUS_INIT_C : ProtoDuneDpmStatusType := (
      streaming     => '0',
      extTrig       => '0',
      timeStamp     => (others => '0'),
      trigTimeStamp => (others => '0'));

end ProtoDuneDpmPkg;
