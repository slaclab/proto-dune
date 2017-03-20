-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmCore.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2017-03-20
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
   signal dpmBusy : slv(7 downto 0);
   signal cdrClk  : sl;
   signal cdrRst  : sl;

begin

   ----------------
   -- RTM Interface
   ----------------
   U_RTM_INTF : entity work.ProtoDuneDtmRtmIntf
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Clocks and Resets
         axilClk      => axilClk,
         axilRst      => axilRst,
         refClk200    => refClk200,
         refRst200    => refRst200,
         -- RTM Interface
         hardRst      => config.hardRst,
         busyOut      => status.busyOut,
         sfpTxDis     => '0',
         sfpTx        => '0',
         -- Status (axilClk domain)
         cdrLocked    => status.cdrLocked,
         freqMeasured => status.freqMeasured,
         -- CDR Interface
         recClk       => recClk,
         recData      => recData
         recLol       => recLol);

   ----------------
   -- DPM Interface
   ----------------
   U_DPM_INTF : entity work.ProtoDuneDtmDpmIntf
      generic map (
         TPD_G => TPD_G)
      port map (
         -- DPM Interface
         dpmBusy => dpmBusy
         -- CDR Interface
         recClk  => recClk,
         recData => recData);

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
         rdy     => status.timing.linkUp,
         sync    => status.timing.syncCmd,  -- Sync command output (clk domain)
         sync_v  => status.timing.syncValid,  -- Sync command valid flag (clk domain)
         tstamp  => status.timing.timestamp,  -- Timestamp out
         evtctr  => status.timing.eventCnt);  -- Event counter out

end architecture rtl;
