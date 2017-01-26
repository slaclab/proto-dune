-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmCore.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2016-12-02
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
use work.AxiLitePkg.all;
use work.AxiStreamPkg.all;
use work.ProtoDuneDpmPkg.all;

entity ProtoDuneDpmCore is
   generic (
      TPD_G            : time             := 1 ns;
      CASCADE_SIZE_G   : positive         := 1;
      AXI_CLK_FREQ_G   : real             := 125.0E+6;  -- units of Hz
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");      
   port (
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
      dmaObMaster     : in  AxiStreamMasterType;
      dmaObSlave      : out AxiStreamSlaveType;
      -- RTM Interface
      ref250ClkP      : in  sl;
      ref250ClkN      : in  sl;
      dpmToRtmHsP     : out slv(2 downto 0);
      dpmToRtmHsN     : out slv(2 downto 0);
      rtmToDpmHsP     : in  slv(2 downto 0);
      rtmToDpmHsN     : in  slv(2 downto 0);
      -- DTM Interface
      dtmRefClkP      : in  sl;
      dtmRefClkN      : in  sl;
      dtmClkP         : in  slv(1 downto 0);
      dtmClkN         : in  slv(1 downto 0);
      dtmFbP          : in  sl;
      dtmFbN          : in  sl;
      -- User ETH interface (userEthClk domain)
      ethClk          : in  sl;
      ethRst          : in  sl;
      localIp         : in  slv(31 downto 0);
      localMac        : in  slv(47 downto 0);
      ibMacMaster     : out AxiStreamMasterType;
      ibMacSlave      : in  AxiStreamSlaveType;
      obMacMaster     : in  AxiStreamMasterType;
      obMacSlave      : out AxiStreamSlaveType);
end ProtoDuneDpmCore;

architecture mapping of ProtoDuneDpmCore is

   constant NUM_AXIL_MASTERS_C : natural := 5;

   constant HLS_INDEX_C    : natural := 0;
   constant WIB_INDEX_C    : natural := 1;
   constant EMU_INDEX_C    : natural := 2;
   constant TIMING_INDEX_C : natural := 3;
   constant RSSI_INDEX_C   : natural := 4;

   constant XBAR_CONFIG_C : AxiLiteCrossbarMasterConfigArray(NUM_AXIL_MASTERS_C-1 downto 0) := genAxiLiteConfig(NUM_AXIL_MASTERS_C, AXI_BASE_ADDR_G, 28, 24);

   signal axilWriteMasters : AxiLiteWriteMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);

   signal clk       : sl;
   signal rst       : sl;
   signal runEnable : sl;
   signal swFlush   : sl;

   signal emuLoopback : sl;
   signal emuEnable   : sl;
   signal emuData     : slv(15 downto 0);
   signal emuDataK    : slv(1 downto 0);

   signal txPreCursor  : slv(4 downto 0);
   signal txPostCursor : slv(4 downto 0);
   signal txDiffCtrl   : slv(3 downto 0);

   signal wibMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal wibSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal obServerMaster : AxiStreamMasterType;
   signal obServerSlave  : AxiStreamSlaveType;
   signal ibServerMaster : AxiStreamMasterType;
   signal ibServerSlave  : AxiStreamSlaveType;

   signal rssiMaster : AxiStreamMasterType;
   signal rssiSlave  : AxiStreamSlaveType;

begin

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

   --------------------------
   -- AXI-Lite: Timing Module
   --------------------------
   U_Timing : entity work.ProtoDuneDpmTiming
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G,
         AXI_BASE_ADDR_G  => XBAR_CONFIG_C(TIMING_INDEX_C).baseAddr)   
      port map (
         -- Timing Interface (clk domain)
         clk             => clk,
         rst             => rst,
         emuEnable       => emuEnable,
         runEnable       => runEnable,
         swFlush         => swFlush,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(TIMING_INDEX_C),
         axilReadSlave   => axilReadSlaves(TIMING_INDEX_C),
         axilWriteMaster => axilWriteMasters(TIMING_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(TIMING_INDEX_C),
         -- DTM Interface
         dtmRefClkP      => dtmRefClkP,
         dtmRefClkN      => dtmRefClkN,
         dtmClkP         => dtmClkP,
         dtmClkN         => dtmClkN,
         dtmFbP          => dtmFbP,
         dtmFbN          => dtmFbN);

   -----------------------
   -- AXI-Lite: EMU Module
   -----------------------
   U_Emu : entity work.ProtoDuneDpmEmu
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G,
         AXI_BASE_ADDR_G  => XBAR_CONFIG_C(EMU_INDEX_C).baseAddr)   
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(EMU_INDEX_C),
         axilReadSlave   => axilReadSlaves(EMU_INDEX_C),
         axilWriteMaster => axilWriteMasters(EMU_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(EMU_INDEX_C),
         -- TX EMU Interface
         clk             => clk,
         rst             => rst,
         emuLoopback     => emuLoopback,
         emuEnable       => emuEnable,
         emuData         => emuData,
         emuDataK        => emuDataK,
         txPreCursor     => txPreCursor,
         txPostCursor    => txPostCursor,
         txDiffCtrl      => txDiffCtrl);

   -----------------------
   -- AXI-Lite: WIB Module
   -----------------------
   U_Wib : entity work.ProtoDuneDpmWib
      generic map (
         TPD_G            => TPD_G,
         CASCADE_SIZE_G   => CASCADE_SIZE_G,
         AXI_CLK_FREQ_G   => AXI_CLK_FREQ_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G,
         AXI_BASE_ADDR_G  => XBAR_CONFIG_C(WIB_INDEX_C).baseAddr)   
      port map (
         -- Stable clock and reset reference
         clk             => clk,
         rst             => rst,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(WIB_INDEX_C),
         axilReadSlave   => axilReadSlaves(WIB_INDEX_C),
         axilWriteMaster => axilWriteMasters(WIB_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(WIB_INDEX_C),
         -- RTM Interface
         ref250ClkP      => ref250ClkP,
         ref250ClkN      => ref250ClkN,
         dpmToRtmHsP     => dpmToRtmHsP,
         dpmToRtmHsN     => dpmToRtmHsN,
         rtmToDpmHsP     => rtmToDpmHsP,
         rtmToDpmHsN     => rtmToDpmHsN,
         -- Timing Interface (clk domain)
         swFlush         => swFlush,
         runEnable       => runEnable,
         -- TX EMU Interface (clk domain)
         emuLoopback     => emuLoopback,
         emuData         => emuData,
         emuDataK        => emuDataK,
         txPreCursor     => txPreCursor,
         txPostCursor    => txPostCursor,
         txDiffCtrl      => txDiffCtrl,
         -- WIB Interface (axilClk domain)
         wibMasters      => wibMasters,
         wibSlaves       => wibSlaves);     

   -----------------------
   -- AXI-Lite: HLS Module
   -----------------------
   U_Hls : entity work.ProtoDuneDpmHls
      generic map (
         TPD_G            => TPD_G,
         CASCADE_SIZE_G   => CASCADE_SIZE_G,
         AXI_CLK_FREQ_G   => AXI_CLK_FREQ_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G,
         AXI_BASE_ADDR_G  => XBAR_CONFIG_C(HLS_INDEX_C).baseAddr)   
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(HLS_INDEX_C),
         axilReadSlave   => axilReadSlaves(HLS_INDEX_C),
         axilWriteMaster => axilWriteMasters(HLS_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(HLS_INDEX_C),
         -- WIB Interface (axilClk domain)
         wibMasters      => wibMasters,
         wibSlaves       => wibSlaves,
         -- AXI Stream Interface (dmaClk domain)
         dmaClk          => dmaClk,
         dmaRst          => dmaRst,
         dmaIbMaster     => dmaIbMaster,
         dmaIbSlave      => dmaIbSlave);

   ----------------------------   
   -- AXI-Lite: User ETH Module
   ----------------------------   
   U_Eth : entity work.ProtoDuneDpmEth
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G,
         AXI_BASE_ADDR_G  => XBAR_CONFIG_C(RSSI_INDEX_C).baseAddr)   
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(RSSI_INDEX_C),
         axilReadSlave   => axilReadSlaves(RSSI_INDEX_C),
         axilWriteMaster => axilWriteMasters(RSSI_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(RSSI_INDEX_C),
         -- AXI Stream Interface (dmaClk domain)
         dmaClk          => dmaClk,
         dmaRst          => dmaRst,
         dmaObMaster     => dmaObMaster,
         dmaObSlave      => dmaObSlave,
         -- User ETH interface (userEthClk domain)
         ethClk          => ethClk,
         ethRst          => ethRst,
         localIp         => localIp,
         localMac        => localMac,
         ibMacMaster     => ibMacMaster,
         ibMacSlave      => ibMacSlave,
         obMacMaster     => obMacMaster,
         obMacSlave      => obMacSlave);

end architecture mapping;