-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmTimingMsg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-05-11
-- Last update: 2017-05-11
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
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

use work.StdRtlPkg.all;
use work.AxiStreamPkg.all;
use work.SsiPkg.all;
use work.RceG3Pkg.all;
use work.ProtoDuneDpmPkg.all;

entity ProtoDuneDpmTimingMsg is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Timing Interface (cdrClk domain)
      cdrClk          : in  sl;
      cdrRst          : in  sl;
      timingBus       : in  ProtoDuneDpmTimingType;
      timingMsgDrop   : out sl;
      timingRunEnable : out sl;
      triggerDet      : out sl;
      -- AXI Stream Interface (dmaClk domain)
      dmaClk          : in  sl;
      dmaRst          : in  sl;
      dmaIbMaster     : out AxiStreamMasterType;
      dmaIbSlave      : in  AxiStreamSlaveType);
end ProtoDuneDpmTimingMsg;

architecture mapping of ProtoDuneDpmTimingMsg is

   constant AXIS_CONFIG_C : AxiStreamConfigType := (
      TSTRB_EN_C    => RCEG3_AXIS_DMA_CONFIG_C.TSTRB_EN_C,
      TDATA_BYTES_C => 16,              -- 128-bit data bus
      TDEST_BITS_C  => RCEG3_AXIS_DMA_CONFIG_C.TDEST_BITS_C,
      TID_BITS_C    => RCEG3_AXIS_DMA_CONFIG_C.TID_BITS_C,
      TKEEP_MODE_C  => RCEG3_AXIS_DMA_CONFIG_C.TKEEP_MODE_C,
      TUSER_BITS_C  => RCEG3_AXIS_DMA_CONFIG_C.TUSER_BITS_C,
      TUSER_MODE_C  => RCEG3_AXIS_DMA_CONFIG_C.TUSER_MODE_C);


   type RegType is record
      timingMsgDrop   : sl;
      timingRunEnable : sl;
      triggerDet      : sl;
      txMaster        : AxiStreamMasterType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      timingMsgDrop   => '0',
      timingRunEnable => '0',
      triggerDet      => '0',
      txMaster        => AXI_STREAM_MASTER_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal txMaster : AxiStreamMasterType;
   signal txSlave  : AxiStreamSlaveType;

   -- attribute dont_touch             : string;
   -- attribute dont_touch of r        : signal is "TRUE";

begin

   comb : process (cdrRst, r, timingBus, txSlave) is
      variable v : RegType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the signals
      v.timingMsgDrop := '0';
      v.triggerDet    := '0';
      if (txSlave.tReady = '1') then
         v.txMaster.tValid := '0';
      end if;

      -- Check if time stamp is ready
      if (timingBus.rdy = '1') then
         -- Check for valid 
         if (timingBus.syncValid = '1') then
            -- Check if ready to move data
            if (v.txMaster.tValid = '0') then
               -- Forward the timing message
               v.txMaster.tData(63 downto 0)    := timingBus.timestamp;
               v.txMaster.tData(95 downto 64)   := timingBus.eventCnt;
               v.txMaster.tData(99 downto 96)   := timingBus.syncCmd;
               v.txMaster.tData(103 downto 100) := timingBus.stat;
               v.txMaster.tLast                 := '1';
               v.txMaster.tKeep                 := (others => '1');
               ssiSetUserSof(AXIS_CONFIG_C, v.txMaster, '1');
            else
               -- Set the flag
               v.timingMsgDrop := '1';
            end if;
            -- Check for spill_start command
            if (timingBus.syncCmd = 0) then
               -- Set the flag
               v.timingRunEnable := '1';
            end if;
            -- Check for spill_end command
            if (timingBus.syncCmd = 1) then
               -- Set the flag
               v.timingRunEnable := '0';
            end if;
            -- Check for trigger
            if (timingBus.syncCmd = 3) then
               -- Set the flag
               v.triggerDet := '1';
            end if;
         end if;
      end if;

      -- Reset
      if (cdrRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      timingMsgDrop   <= r.timingMsgDrop;
      timingRunEnable <= r.timingRunEnable;
      triggerDet      <= r.triggerDet;

   end process comb;

   seq : process (cdrClk) is
   begin
      if rising_edge(cdrClk) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   U_Fifo : entity work.SsiFifo
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         PIPE_STAGES_G       => 1,
         EN_FRAME_FILTER_G   => false,
         VALID_THOLD_G       => 1,
         -- FIFO configurations
         BRAM_EN_G           => true,
         XIL_DEVICE_G        => "7SERIES",
         USE_BUILT_IN_G      => false,
         GEN_SYNC_FIFO_G     => false,
         CASCADE_SIZE_G      => 1,
         FIFO_ADDR_WIDTH_G   => 9,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => AXIS_CONFIG_C,
         MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
      port map (
         -- Slave Port
         sAxisClk    => cdrClk,
         sAxisRst    => cdrRst,
         sAxisMaster => r.txMaster,
         sAxisSlave  => txSlave,
         -- Master Port
         mAxisClk    => dmaClk,
         mAxisRst    => dmaRst,
         mAxisMaster => dmaIbMaster,
         mAxisSlave  => dmaIbSlave);

end mapping;
