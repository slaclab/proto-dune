-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmTiming.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2017-06-05
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
use work.AxiLitePkg.all;
use work.AxiStreamPkg.all;
use work.ProtoDuneDpmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDpmTiming is
   generic (
      TPD_G            : time             := 1 ns;
      CASCADE_SIZE_G   : positive         := 1;
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");
   port (
      -- Timing Interface (wibClk domain)
      wibClk          : in  sl;
      wibRst          : in  sl;
      emuEnable       : in  sl;
      runEnable       : out sl;
      swFlush         : out sl;
      timingClk       : out sl;
      timingRst       : out sl;
      timingTrig      : out sl;
      timingTs        : out slv(63 downto 0);
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- AXI Stream Interface (dmaClk domain)
      dmaClk          : in  sl;
      dmaRst          : in  sl;
      dmaIbMaster     : out AxiStreamMasterType;
      dmaIbSlave      : in  AxiStreamSlaveType;
      -- Reference 200 MHz clock
      refClk200       : in  sl;
      refRst200       : in  sl;
      -- DTM Interface
      dtmClkP         : in  slv(1 downto 0);
      dtmClkN         : in  slv(1 downto 0);
      dtmFbP          : out sl;
      dtmFbN          : out sl);
end ProtoDuneDpmTiming;

architecture mapping of ProtoDuneDpmTiming is

   signal runEn : sl;
   signal busy  : sl;

   signal clock           : sl;
   signal recClk          : sl;
   signal data            : sl;
   signal Q1              : sl;
   signal Q2              : sl;
   signal recData         : sl;
   signal freqMeasured    : slv(31 downto 0);
   signal cdrLocked       : sl;
   signal recLol          : sl;
   signal cdrClk          : sl;
   signal cdrRst          : sl;
   signal cdrDataInv      : sl;
   signal cdrEdgeSel      : sl;
   signal timingBus       : ProtoDuneDpmTimingType;
   signal timingMsgDrop   : sl;
   signal timingRunEnable : sl;
   signal triggerDet      : sl;

   attribute dont_touch              : string;
   attribute dont_touch of timingBus : signal is "TRUE";

begin


   U_BUSY : OBUFDS
      port map (
         I  => busy,
         O  => dtmFbP,
         OB => dtmFbN);

   U_CDR_CLK : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map (
         I  => dtmClkP(0),
         IB => dtmClkN(0),
         O  => clock);

   U_BUFG : BUFG
      port map (
         I => clock,
         O => recClk);

   U_CDR_DATA : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map (
         I  => dtmClkP(1),
         IB => dtmClkN(1),
         O  => data);

   U_IDDR : IDDR
      generic map (
         DDR_CLK_EDGE => "SAME_EDGE_PIPELINED",  -- "OPPOSITE_EDGE", "SAME_EDGE", or "SAME_EDGE_PIPELINED"
         INIT_Q1      => '0',           -- Initial value of Q1: '0' or '1'
         INIT_Q2      => '0',           -- Initial value of Q2: '0' or '1'
         SRTYPE       => "SYNC")        -- Set/Reset type: "SYNC" or "ASYNC" 
      port map (
         D  => data,                    -- 1-bit DDR data input
         C  => recClk,                  -- 1-bit clock input
         CE => '1',                     -- 1-bit clock enable input
         R  => '0',                     -- 1-bit reset
         S  => '0',                     -- 1-bit set
         Q1 => Q1,   -- 1-bit output for positive edge of clock 
         Q2 => Q2);  -- 1-bit output for negative edge of clock          

   process(recClk)
   begin
      if rising_edge(recClk) then
         if (cdrEdgeSel = '0') then
            recData <= Q1 xor cdrDataInv after TPD_G;
         else
            recData <= Q2 xor cdrDataInv after TPD_G;
         end if;
      end if;
   end process;

   ---------------------------------------------------------
   -- Measure the CDR clock frequency and determine 
   -- if (CLK_LOWER_LIMIT_G < CDR Clock < CLK_UPPER_LIMIT_G)
   ---------------------------------------------------------
   U_ClockFreq : entity work.SyncClockFreq
      generic map (
         TPD_G             => TPD_G,
         REF_CLK_FREQ_G    => 200.0E+6,
         REFRESH_RATE_G    => 1.0E+3,
         CLK_UPPER_LIMIT_G => 251.0E+6,
         CLK_LOWER_LIMIT_G => 249.0E+6,
         CNT_WIDTH_G       => 32)
      port map (
         -- Frequency Measurement and Monitoring Outputs (locClk domain)
         freqOut => freqMeasured,
         locked  => cdrLocked,
         -- Clocks
         clkIn   => recClk,
         locClk  => axilClk,
         refClk  => refClk200);

   recLol <= not(cdrLocked);

   ---------------------         
   -- CERN Timing Module
   ---------------------         
   U_Timing : entity work.pdts_endpoint
      generic map (
         SCLK_FREQ => 125.0)
      port map (
         sclk    => axilClk,            -- Free-running system clock
         srst    => axilRst,            -- System reset (sclk domain)
         addr    => x"00",  -- "Any address except 0x01 is acceptable for this test" from Dave Newbold (20MARCH2017)
         tgrp    => "00",  -- "Any tgrp is acceptable - 0x0 will do" from Dave Newbold (20MARCH2017)
         stat    => timingBus.stat,  -- The status signal (stat) that indicates the internal state of the endpoint
         rec_clk => recClk,
         rec_d   => recData,
         sfp_los => '0',
         cdr_los => '0',
         cdr_lol => recLol,
         clk     => cdrClk,
         rst     => cdrRst,
         rdy     => timingBus.rdy,
         sync    => timingBus.syncCmd,  -- Sync command output (clk domain)
         sync_v  => timingBus.syncValid,  -- Sync command valid flag (clk domain)
         tstamp  => timingBus.timestamp,  -- Timestamp out
         evtctr  => timingBus.eventCnt);  -- Event counter out

   --------------------------
   -- Timing Register Control
   --------------------------
   U_TimingReg : entity work.ProtoDuneDpmTimingReg
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- WIB Interface (wibClk domain)
         wibClk          => wibClk,
         wibRst          => wibRst,
         runEnable       => runEn,
         swFlush         => swFlush,
         -- Timing RX Interface (cdrClk domain)
         cdrClk          => cdrClk,
         cdrRst          => cdrRst,
         cdrEdgeSel      => cdrEdgeSel,
         cdrDataInv      => cdrDataInv,
         cdrLocked       => cdrLocked,
         freqMeasured    => freqMeasured,
         timingBus       => timingBus,
         timingMsgDrop   => timingMsgDrop,
         timingRunEnable => timingRunEnable,
         triggerDet      => triggerDet,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave);

   process(wibClk)
   begin
      if rising_edge(wibClk) then
         runEnable <= runEn                        after TPD_G;
         runEn     <= timingRunEnable or emuEnable after TPD_G;
      end if;
   end process;

   U_Msg : entity work.ProtoDuneDpmTimingMsg
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Timing Interface (cdrClk domain)
         cdrClk          => cdrClk,
         cdrRst          => cdrRst,
         timingBus       => timingBus,
         timingMsgDrop   => timingMsgDrop,
         timingRunEnable => timingRunEnable,
         triggerDet      => triggerDet,
         -- AXI Stream Interface (dmaClk domain)
         dmaClk          => dmaClk,
         dmaRst          => dmaRst,
         dmaIbMaster     => dmaIbMaster,
         dmaIbSlave      => dmaIbSlave);

   timingClk <= recClk;
   U_Rst : entity work.RstSync
      generic map (
         TPD_G => TPD_G)
      port map (
         clk      => recClk,
         asyncRst => cdrRst,
         syncRst  => timingRst);

   U_timingTs : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 64)
      port map (
         rst    => cdrRst,
         wr_clk => cdrClk,
         din    => timingBus.timestamp,
         rd_clk => recClk,
         dout   => timingTs);

   U_timingTrig : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 1)
      port map (
         rst    => cdrRst,
         wr_clk => cdrClk,
         din(0) => '0',
         wr_en  => triggerDet,
         rd_clk => recClk,
         valid  => timingTrig,
         dout   => open);

   -------------------------------
   -- Place holder for future code
   -------------------------------
   busy <= '0';

end mapping;
