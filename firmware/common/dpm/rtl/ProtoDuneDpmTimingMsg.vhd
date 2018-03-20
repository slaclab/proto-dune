-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmTimingMsg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-05-11
-- Last update: 2018-03-20
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

use work.pdts_defs.all;

entity ProtoDuneDpmTimingMsg is
   generic (
      TPD_G : time := 1 ns);
   port (
      softRst         : in  sl;
      -- Timing Interface (cdrClk domain)
      cdrClk          : in  sl;
      cdrRst          : in  sl;
      timingBus       : in  ProtoDuneDpmTimingType;
      timingMsgDrop   : out sl;
      timingRunEnable : out sl;
      triggerDet      : out sl;
      eventCnt        : out slv(31 downto 0);
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
      eventCnt        : slv(31 downto 0);
      timingMsgDrop   : sl;
      timingRunEnable : sl;
      triggerDet      : sl;
      txMaster        : AxiStreamMasterType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      eventCnt        => (others => '0'),
      timingMsgDrop   => '0',
      timingRunEnable => '0',
      triggerDet      => '0',
      txMaster        => AXI_STREAM_MASTER_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal reset    : sl;
   signal cdrReset : sl;

   signal txMaster : AxiStreamMasterType;
   signal txSlave  : AxiStreamSlaveType;

   -- attribute dont_touch             : string;
   -- attribute dont_touch of r        : signal is "TRUE";

begin

   reset <= cdrRst or softRst;

   U_softRst : entity work.RstSync
      generic map (
         TPD_G => TPD_G)
      port map (
         clk      => cdrClk,
         asyncRst => reset,
         syncRst  => cdrReset);

   comb : process (cdrReset, r, timingBus, txSlave) is
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

            -- Check for run_start command
            if (timingBus.syncCmd = SCMD_RUN_START) then
               -- Set the flag
               v.timingRunEnable := '1';
            end if;

            -- Check for run_end command
            if (timingBus.syncCmd = SCMD_RUN_STOP) then
               -- Set the flag
               v.timingRunEnable := '0';
            end if;

            -- Check for trigger: op-code = [0x8:0xF]
            if (timingBus.syncCmd(3) = '1') then
               -- Set the flag
               v.triggerDet := '1';
            end if;

            -- Check if ready to move data
            if (v.txMaster.tValid = '0') then
               -- Forward the timing message
               v.txMaster.tData(63 downto 0)    := timingBus.timestamp;
               v.txMaster.tData(95 downto 64)   := r.eventCnt;
               v.txMaster.tData(99 downto 96)   := timingBus.syncCmd;
               v.txMaster.tData(103 downto 100) := timingBus.stat;
               v.txMaster.tValid                := '1';
               v.txMaster.tLast                 := '1';
               v.txMaster.tKeep                 := (others => '1');
               v.txMaster.tDest                 := x"FF";
               ssiSetUserSof(AXIS_CONFIG_C, v.txMaster, '1');
            else
               -- Set the flag
               v.timingMsgDrop := '1';
            end if;

            -- Increment the counter
            v.eventCnt := r.eventCnt + 1;

         end if;
      end if;

      -- Reset
      if (cdrReset = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      timingMsgDrop   <= r.timingMsgDrop;
      timingRunEnable <= r.timingRunEnable;
      triggerDet      <= r.triggerDet;
      eventCnt        <= r.eventCnt;

   end process comb;

   seq : process (cdrClk) is
   begin
      if rising_edge(cdrClk) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   U_Fifo : entity work.AxiStreamFifoV2
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         INT_PIPE_STAGES_G   => 1,
         PIPE_STAGES_G       => 1,
         SLAVE_READY_EN_G    => true,
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
         sAxisRst    => cdrReset,
         sAxisMaster => r.txMaster,
         sAxisSlave  => txSlave,
         -- Master Port
         mAxisClk    => dmaClk,
         mAxisRst    => dmaRst,
         mAxisMaster => dmaIbMaster,
         mAxisSlave  => dmaIbSlave);

end mapping;
