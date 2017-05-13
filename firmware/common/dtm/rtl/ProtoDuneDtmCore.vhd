-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmCore.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2017-05-12
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
use work.AxiLitePkg.all;
use work.AxiStreamPkg.all;
use work.ProtoDuneDtmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDtmCore is
   generic (
      TPD_G            : time             := 1 ns;
      AXI_CLK_FREQ_G   : real             := 125.0E+6;  -- units of Hz
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");
   port (
      -- RTM Low Speed
      dtmToRtmLsP     : inout slv(5 downto 0);
      dtmToRtmLsN     : inout slv(5 downto 0);
      -- DPM Signals
      dpmClkP         : out   slv(2 downto 0);
      dpmClkN         : out   slv(2 downto 0);
      dpmFbP          : in    slv(7 downto 0);
      dpmFbN          : in    slv(7 downto 0);
      -- Reference 200 MHz clock
      refClk200       : in    sl;
      refRst200       : in    sl;
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in    sl;
      axilRst         : in    sl;
      axilReadMaster  : in    AxiLiteReadMasterType;
      axilReadSlave   : out   AxiLiteReadSlaveType;
      axilWriteMaster : in    AxiLiteWriteMasterType;
      axilWriteSlave  : out   AxiLiteWriteSlaveType);
end ProtoDuneDtmCore;

architecture rtl of ProtoDuneDtmCore is

   constant NUM_AXIL_MASTERS_C : natural := 1;

   constant CORE_INDEX_C : natural := 0;

   constant XBAR_CONFIG_C : AxiLiteCrossbarMasterConfigArray(NUM_AXIL_MASTERS_C-1 downto 0) := genAxiLiteConfig(NUM_AXIL_MASTERS_C, AXI_BASE_ADDR_G, 28, 24);

   signal axilWriteMasters : AxiLiteWriteMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);

   signal status : ProtoDuneDtmStatusType;
   signal config : ProtoDuneDtmConfigType;

   signal recClk  : sl;
   signal recData : sl;
   signal recLol  : sl;
   signal qsfpRst : sl;
   signal dpmBusy : slv(7 downto 0);
   signal cdrClk  : sl;
   signal cdrRst  : sl;
   signal busyOut : sl;

begin

   qsfpRst <= axilRst or config.hardRst;
   recLol  <= not(status.cdrLocked);
   busyOut <= status.busyOut or not(status.timing.rdy);

   ----------------
   -- RTM Interface
   ----------------
   U_RTM_INTF : entity work.ProtoDuneDtmRtmIntf
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Control Interface
         cdrEdgeSel  => config.cdrEdgeSel,
         cdrDataInv  => config.cdrDataInv,
         qsfpRst     => qsfpRst,
         busyOut     => busyOut,
         sfpTxDis    => '0',
         sfpTx       => recData,
         -- CDR Interface
         recClk      => recClk,
         recData     => recData,
         -- RTM Low Speed Ports
         dtmToRtmLsP => dtmToRtmLsP,
         dtmToRtmLsN => dtmToRtmLsN);

   ----------------
   -- DPM Interface
   ----------------
   U_DPM_INTF : entity work.ProtoDuneDtmDpmIntf
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Busy Interface
         dpmBusy => dpmBusy,
         -- CDR Interface
         recClk  => recClk,
         recData => recData,
         -- DPM Ports
         dpmClkP => dpmClkP,
         dpmClkN => dpmClkN,
         dpmFbP  => dpmFbP,
         dpmFbN  => dpmFbN);

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
         freqOut => status.freqMeasured,
         locked  => status.cdrLocked,
         -- Clocks
         clkIn   => recClk,
         locClk  => axilClk,
         refClk  => refClk200);

   --------------------
   -- AXI-Lite Crossbar
   --------------------
   U_XBAR : entity work.AxiLiteCrossbar
      generic map (
         TPD_G              => TPD_G,
         NUM_SLAVE_SLOTS_G  => 1,
         NUM_MASTER_SLOTS_G => NUM_AXIL_MASTERS_C,
         DEC_ERROR_RESP_G   => AXI_ERROR_RESP_G,
         MASTERS_CONFIG_G   => XBAR_CONFIG_C)
      port map (
         axiClk              => axilClk,
         axiClkRst           => axilRst,
         sAxiWriteMasters(0) => axilWriteMaster,
         sAxiWriteSlaves(0)  => axilWriteSlave,
         sAxiReadMasters(0)  => axilReadMaster,
         sAxiReadSlaves(0)   => axilReadSlave,
         mAxiWriteMasters    => axilWriteMasters,
         mAxiWriteSlaves     => axilWriteSlaves,
         mAxiReadMasters     => axilReadMasters,
         mAxiReadSlaves      => axilReadSlaves);

   ------------------------
   -- AXI-Lite: Core Module
   ------------------------
   U_Reg : entity work.ProtoDuneDtmReg
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Status/Configuration Interface
         cdrClk          => cdrClk,
         cdrRst          => cdrRst,
         status          => status,
         config          => config,
         -- AXI-Lite Interface 
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(CORE_INDEX_C),
         axilReadSlave   => axilReadSlaves(CORE_INDEX_C),
         axilWriteMaster => axilWriteMasters(CORE_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(CORE_INDEX_C));

   --------------
   -- Busy Module
   --------------
   U_Busy : entity work.ProtoDuneDtmBusy
      generic map (
         TPD_G => TPD_G)
      port map (
         axilClk => axilClk,
         axilRst => axilRst,
         config  => config,
         dpmBusy => dpmBusy,
         busyVec => status.busyVec,
         busyOut => status.busyOut);

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
         stat    => status.timing.stat,  -- The status signal (stat) that indicates the internal state of the endpoint
         rec_clk => recClk,
         rec_d   => recData,
         sfp_los => '0',
         cdr_los => '0',
         cdr_lol => recLol,
         clk     => cdrClk,
         rst     => cdrRst,
         rdy     => status.timing.rdy,
         sync    => status.timing.syncCmd,  -- Sync command output (clk domain)
         sync_v  => status.timing.syncValid,  -- Sync command valid flag (clk domain)
         tstamp  => status.timing.timestamp,  -- Timestamp out
         evtctr  => status.timing.eventCnt);  -- Event counter out

end architecture rtl;
