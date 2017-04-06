-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDpm10GbE.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2017-03-22
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
use work.RceG3Pkg.all;
use work.ProtoDuneDpmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDpm10GbE is
   generic (
      TPD_G        : time := 1 ns;
      BUILD_INFO_G : BuildInfoType);
   port (
      -- Debug
      led         : out   slv(1 downto 0);
      -- I2C
      i2cSda      : inout sl;
      i2cScl      : inout sl;
      -- Ethernet
      ethRxP      : in    slv(3 downto 0);
      ethRxM      : in    slv(3 downto 0);
      ethTxP      : out   slv(3 downto 0);
      ethTxM      : out   slv(3 downto 0);
      ethRefClkP  : in    sl;
      ethRefClkM  : in    sl;
      -- RTM Interface
      locRefClkP  : in    sl;
      locRefClkM  : in    sl;
      dpmToRtmHsP : out   slv(2 downto 0);
      dpmToRtmHsM : out   slv(2 downto 0);
      rtmToDpmHsP : in    slv(2 downto 0);
      rtmToDpmHsM : in    slv(2 downto 0);
      -- DTM Signals
      dtmRefClkP  : in    sl;
      dtmRefClkM  : in    sl;
      dtmClkP     : in    slv(1 downto 0);
      dtmClkM     : in    slv(1 downto 0);
      dtmFbP      : out   sl;
      dtmFbM      : out   sl;
      -- Clock Select
      clkSelA     : out   slv(1 downto 0);
      clkSelB     : out   slv(1 downto 0));
end ProtoDuneDpm10GbE;

architecture TOP_LEVEL of ProtoDuneDpm10GbE is

   signal dmaClk    : slv(2 downto 0);
   signal dmaClkRst : slv(2 downto 0);

   signal dmaObMaster : AxiStreamMasterArray(2 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal dmaObSlave  : AxiStreamSlaveArray(2 downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);
   signal dmaIbMaster : AxiStreamMasterArray(2 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal dmaIbSlave  : AxiStreamSlaveArray(2 downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);

   signal axilClk   : sl;
   signal axilRst   : sl;
   signal ref200Clk : sl;
   signal ref200Rst : sl;

   signal extAxilReadMaster  : AxiLiteReadMasterType;
   signal extAxilReadSlave   : AxiLiteReadSlaveType;
   signal extAxilWriteMaster : AxiLiteWriteMasterType;
   signal extAxilWriteSlave  : AxiLiteWriteSlaveType;

   signal axilReadMaster  : AxiLiteReadMasterType;
   signal axilReadSlave   : AxiLiteReadSlaveType;
   signal axilWriteMaster : AxiLiteWriteMasterType;
   signal axilWriteSlave  : AxiLiteWriteSlaveType;

   signal ethClk   : sl;
   signal ethRst   : sl;
   signal localMac : slv(47 downto 0);
   signal localIp  : slv(31 downto 0);

   signal ibMacMaster : AxiStreamMasterType;
   signal ibMacSlave  : AxiStreamSlaveType;
   signal obMacMaster : AxiStreamMasterType;
   signal obMacSlave  : AxiStreamSlaveType;

begin

   led <= "00";

   -----------
   -- DPM Core
   -----------
   U_DpmCore : entity work.DpmCore
      generic map (
         TPD_G              => TPD_G,
         BUILD_INFO_G       => BUILD_INFO_G,
         -- RCE_DMA_MODE_G     => RCE_DMA_AXIS_C,-- AXIS V1 Driver
         RCE_DMA_MODE_G     => RCE_DMA_AXISV2_C,-- AXIS V2 Driver
         ETH_10G_EN_G       => true,
         UDP_SERVER_EN_G    => true,
         UDP_SERVER_SIZE_G  => 1,
         UDP_SERVER_PORTS_G => RSSI_PORTS_C)
      port map (
         -- I2C
         i2cSda             => i2cSda,
         i2cScl             => i2cScl,
         -- Ethernet
         ethRxP             => ethRxP,
         ethRxM             => ethRxM,
         ethTxP             => ethTxP,
         ethTxM             => ethTxM,
         ethRefClkP         => ethRefClkP,
         ethRefClkM         => ethRefClkM,
         -- Clock Select
         clkSelA            => clkSelA,
         clkSelB            => clkSelB,
         -- Clocks and Resets
         sysClk125          => open,
         sysClk125Rst       => open,
         sysClk200          => ref200Clk,
         sysClk200Rst       => ref200Rst,
         -- External AXI-Lite Interface [0xA0000000:0xAFFFFFFF]
         axiClk             => axilClk,
         axiClkRst          => axilRst,
         extAxilReadMaster  => extAxilReadMaster,
         extAxilReadSlave   => extAxilReadSlave,
         extAxilWriteMaster => extAxilWriteMaster,
         extAxilWriteSlave  => extAxilWriteSlave,
         -- DMA Interfaces
         dmaClk             => dmaClk,
         dmaClkRst          => dmaClkRst,
         dmaObMaster        => dmaObMaster,
         dmaObSlave         => dmaObSlave,
         dmaIbMaster        => dmaIbMaster,
         dmaIbSlave         => dmaIbSlave,
         -- User ETH interface (userEthClk domain)
         userEthClk         => ethClk,
         userEthClkRst      => ethRst,
         userEthIpAddr      => localIp,
         userEthMacAddr     => localMac,
         userEthUdpIbMaster => ibMacMaster,
         userEthUdpIbSlave  => ibMacSlave,
         userEthUdpObMaster => obMacMaster,
         userEthUdpObSlave  => obMacSlave);

   ------------------
   -- DMA Channel = 0
   ------------------
   -- Loop Back
   dmaClk(0)      <= axilClk;
   dmaClkRst(0)   <= axilRst;
   dmaIbMaster(0) <= dmaObMaster(0);
   dmaObSlave(0)  <= dmaIbSlave(0);

   ------------------
   -- DMA Channel = 1
   ------------------   
   U_DMA_XBAR : entity work.RceG3AppRegCrossbar
      generic map (
         TPD_G        => TPD_G,
         COMMON_CLK_G => true)
      port map (
         -- DMA Bus: SRPv0 Protocol
         dmaClk             => dmaClk(1),
         dmaRst             => dmaClkRst(1),
         dmaObMaster        => dmaObMaster(1),
         dmaObSlave         => dmaObSlave(1),
         dmaIbMaster        => dmaIbMaster(1),
         dmaIbSlave         => dmaIbSlave(1),
         -- CPU AXI-Lite Bus [0xA0000000:0xAFFFFFFF]
         axilClk            => axilClk,
         axilRst            => axilRst,
         extAxilReadMaster  => extAxilReadMaster,
         extAxilReadSlave   => extAxilReadSlave,
         extAxilWriteMaster => extAxilWriteMaster,
         extAxilWriteSlave  => extAxilWriteSlave,
         -- Application AXI-Lite Bus [0xA0000000:0xAFFFFFFF]   
         appClk             => axilClk,
         appRst             => axilRst,
         axilReadMaster     => axilReadMaster,
         axilReadSlave      => axilReadSlave,
         axilWriteMaster    => axilWriteMaster,
         axilWriteSlave     => axilWriteSlave);

   ------------------
   -- DMA Channel = 2
   ------------------
   dmaClk(2)    <= ref200Clk;
   dmaClkRst(2) <= ref200Rst;
   U_App : entity work.ProtoDuneDpmCore
      generic map (
         TPD_G            => TPD_G,
         CASCADE_SIZE_G   => 4,
         AXI_ERROR_RESP_G => AXI_RESP_OK_C,
         AXI_BASE_ADDR_G  => x"A0000000")
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave,
         -- AXI Stream Interface (dmaClk domain)
         dmaClk          => ref200Clk,
         dmaRst          => ref200Rst,
         dmaIbMaster     => dmaIbMaster(2),
         dmaIbSlave      => dmaIbSlave(2),
         dmaObMaster     => dmaObMaster(2),
         dmaObSlave      => dmaObSlave(2),
         -- RTM Interface
         ref250ClkP      => locRefClkP,
         ref250ClkN      => locRefClkM,
         dpmToRtmHsP     => dpmToRtmHsP,
         dpmToRtmHsN     => dpmToRtmHsM,
         rtmToDpmHsP     => rtmToDpmHsP,
         rtmToDpmHsN     => rtmToDpmHsM,
         -- DTM Interface
         dtmRefClkP      => dtmRefClkP,
         dtmRefClkN      => dtmRefClkM,
         dtmClkP         => dtmClkP,
         dtmClkN         => dtmClkM,
         dtmFbP          => dtmFbP,
         dtmFbN          => dtmFbM,
         -- Reference 200 MHz clock
         refClk200       => ref200Clk,
         refRst200       => ref200Rst,            
         -- User ETH interface (ethClk domain)
         ethClk          => ethClk,
         ethRst          => ethRst,
         localIp         => localIp,
         localMac        => localMac,
         ibMacMaster     => ibMacMaster,
         ibMacSlave      => ibMacSlave,
         obMacMaster     => obMacMaster,
         obMacSlave      => obMacSlave);

end architecture TOP_LEVEL;
