-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmCore.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2018-03-08
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

use work.pdts_defs.all;

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
      -- Reference 250 Clock
      refClk250P      : in    sl;
      refClk250N      : in    sl;
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

   signal recClk   : sl;
   signal recData  : sl;
   signal recLol   : sl;
   signal qsfpRst  : sl;
   signal dpmBusy  : slv(7 downto 0);
   signal cdrClk   : sl;
   signal cdrRst   : sl;
   signal busyOut  : sl;
   signal sfpTxDis : sl;
   signal sfpTxDat : sl;

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
         -- sfpTxDis    => sfpTxDis,
         -- sfpTx       => sfpTxDat,
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
         dpmBusy    => dpmBusy,
         -- CDR Interface
         recClk     => recClk,
         recData    => recData,
         cdrLocked  => status.cdrLocked,
         -- Reference 250 Clock
         refClk250P => refClk250P,
         refClk250N => refClk250N,
         -- DPM Ports
         dpmClkP    => dpmClkP,
         dpmClkN    => dpmClkN,
         dpmFbP     => dpmFbP,
         dpmFbN     => dpmFbN);

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
         SCLK_FREQ => 125.0,            -- 125 MHz
         EN_TX     => false)
      port map (
         sclk       => axilClk,         -- Free-running system clock
         srst       => axilRst,         -- System reset (sclk domain)
         addr       => x"00",  -- Endpoint address (async, sampled in clk domain)
         tgrp       => "00",   -- Timing group (async, sampled in clk domain)
         stat       => status.timing.stat,     -- Status output (sclk domain)
         rec_clk    => recClk,          -- CDR recovered clock from timing link
         rec_d      => recData,  -- CDR recovered data from timing link (rec_clk domain)
         txd        => sfpTxDat,  -- Output data to timing link (rec_clk domain)
         sfp_los    => '0',    -- SFP LOS line (async, sampled in sclk domain)
         cdr_los    => '0',    -- CDR LOS line (async, sampled in sclk domain)
         cdr_lol    => recLol,  -- CDR LOL line (async, sampled in sclk domain)
         sfp_tx_dis => sfpTxDis,        -- SFP tx disable line (clk domain)
         clk        => cdrClk,          -- 50MHz clock output
         rst        => cdrRst,          -- 50MHz domain reset
         rdy        => status.timing.rdy,      -- Timestamp valid flag
         sync       => status.timing.syncCmd,  -- Sync command output (clk domain)
         sync_stb   => open,            -- Sync command strobe (clk domain)
         sync_valid => status.timing.syncValid,  -- Sync command valid flag (clk domain)
         tstamp     => status.timing.timestamp,  -- Timestamp out
         tsync_in   => CMD_W_NULL,      -- Tx sync command input
         tsync_out  => open);           -- Tx sync command handshake         

   status.timing.eventCnt <= (others => '0');

end architecture rtl;
