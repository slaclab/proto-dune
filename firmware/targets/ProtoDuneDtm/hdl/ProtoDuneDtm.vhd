-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDtm.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2017-07-24
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

entity ProtoDuneDtm is
   generic (
      TPD_G        : time := 1 ns;
      BUILD_INFO_G : BuildInfoType);
   port (
      -- Debug
      led         : out   slv(1 downto 0);
      -- I2C
      i2cSda      : inout sl;
      i2cScl      : inout sl;
      -- PCI Express
      pciRefClkP  : in    sl;
      pciRefClkM  : in    sl;
      pciRxP      : in    sl;
      pciRxM      : in    sl;
      pciTxP      : out   sl;
      pciTxM      : out   sl;
      pciResetL   : out   sl;
      -- COB Ethernet
      ethRxP      : in    sl;
      ethRxM      : in    sl;
      ethTxP      : out   sl;
      ethTxM      : out   sl;
      -- Reference Clock
      locRefClkP  : in    sl;
      locRefClkM  : in    sl;
      -- Clock Select
      clkSelA     : out   sl;
      clkSelB     : out   sl;
      -- Base Ethernet
      ethRxCtrl   : in    slv(1 downto 0);
      ethRxClk    : in    slv(1 downto 0);
      ethRxDataA  : in    Slv(1 downto 0);
      ethRxDataB  : in    Slv(1 downto 0);
      ethRxDataC  : in    Slv(1 downto 0);
      ethRxDataD  : in    Slv(1 downto 0);
      ethTxCtrl   : out   slv(1 downto 0);
      ethTxClk    : out   slv(1 downto 0);
      ethTxDataA  : out   Slv(1 downto 0);
      ethTxDataB  : out   Slv(1 downto 0);
      ethTxDataC  : out   Slv(1 downto 0);
      ethTxDataD  : out   Slv(1 downto 0);
      ethMdc      : out   Slv(1 downto 0);
      ethMio      : inout Slv(1 downto 0);
      ethResetL   : out   Slv(1 downto 0);
      -- RTM Low Speed
      dtmToRtmLsP : inout slv(5 downto 0);
      dtmToRtmLsM : inout slv(5 downto 0);
      -- DPM Signals
      dpmClkP     : out   slv(2 downto 0);
      dpmClkM     : out   slv(2 downto 0);
      dpmFbP      : in    slv(7 downto 0);
      dpmFbM      : in    slv(7 downto 0);
      -- Backplane Clocks
      bpClkIn     : in    slv(5 downto 0);
      bpClkOut    : out   slv(5 downto 0);
      -- IPMI
      dtmToIpmiP  : out   slv(1 downto 0);
      dtmToIpmiM  : out   slv(1 downto 0));
end ProtoDuneDtm;

architecture TOP_LEVEL of ProtoDuneDtm is

   signal axiClk       : sl;
   signal axiClkRst    : sl;
   signal sysClk200    : sl;
   signal sysClk200Rst : sl;

   signal extAxilReadMaster  : AxiLiteReadMasterType;
   signal extAxilReadSlave   : AxiLiteReadSlaveType;
   signal extAxilWriteMaster : AxiLiteWriteMasterType;
   signal extAxilWriteSlave  : AxiLiteWriteSlaveType;

   signal dmaClk      : slv(2 downto 0);
   signal dmaClkRst   : slv(2 downto 0);
   signal dmaObMaster : AxiStreamMasterArray(2 downto 0);
   signal dmaObSlave  : AxiStreamSlaveArray(2 downto 0);
   signal dmaIbMaster : AxiStreamMasterArray(2 downto 0);
   signal dmaIbSlave  : AxiStreamSlaveArray(2 downto 0);

begin

   led      <= (others => '0');
   bpClkOut <= (others => '0');

   -----------
   -- DTM Core
   -----------
   U_DtmCore : entity work.DtmCore
      generic map (
         TPD_G          => TPD_G,
         BUILD_INFO_G   => BUILD_INFO_G,
         RCE_DMA_MODE_G => RCE_DMA_AXIS_C,
         OLD_BSI_MODE_G => false)
      port map (
         i2cSda             => i2cSda,
         i2cScl             => i2cScl,
         pciRefClkP         => pciRefClkP,
         pciRefClkM         => pciRefClkM,
         pciRxP             => pciRxP,
         pciRxM             => pciRxM,
         pciTxP             => pciTxP,
         pciTxM             => pciTxM,
         pciResetL          => pciResetL,
         ethRxP             => ethRxP,
         ethRxM             => ethRxM,
         ethTxP             => ethTxP,
         ethTxM             => ethTxM,
         clkSelA            => clkSelA,
         clkSelB            => clkSelB,
         ethRxCtrl          => ethRxCtrl,
         ethRxClk           => ethRxClk,
         ethRxDataA         => ethRxDataA,
         ethRxDataB         => ethRxDataB,
         ethRxDataC         => ethRxDataC,
         ethRxDataD         => ethRxDataD,
         ethTxCtrl          => ethTxCtrl,
         ethTxClk           => ethTxClk,
         ethTxDataA         => ethTxDataA,
         ethTxDataB         => ethTxDataB,
         ethTxDataC         => ethTxDataC,
         ethTxDataD         => ethTxDataD,
         ethMdc             => ethMdc,
         ethMio             => ethMio,
         ethResetL          => ethResetL,
         dtmToIpmiP         => dtmToIpmiP,
         dtmToIpmiM         => dtmToIpmiM,
         sysClk125          => open,
         sysClk125Rst       => open,
         sysClk200          => sysClk200,
         sysClk200Rst       => sysClk200Rst,
         axiClk             => axiClk,
         axiClkRst          => axiClkRst,
         extAxilReadMaster  => extAxilReadMaster,
         extAxilReadSlave   => extAxilReadSlave,
         extAxilWriteMaster => extAxilWriteMaster,
         extAxilWriteSlave  => extAxilWriteSlave,
         dmaClk             => dmaClk,
         dmaClkRst          => dmaClkRst,
         dmaObMaster        => dmaObMaster,
         dmaObSlave         => dmaObSlave,
         dmaIbMaster        => dmaIbMaster,
         dmaIbSlave         => dmaIbSlave,
         userInterrupt      => (others => '0'));

   ---------------
   -- DMA Loopback
   ---------------
   dmaClk      <= (others => axiClk);
   dmaClkRst   <= (others => axiClkRst);
   dmaIbMaster <= dmaObMaster;
   dmaObSlave  <= dmaIbSlave;

   -----------
   -- App Core
   -----------
   U_App : entity work.ProtoDuneDtmCore
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_RESP_OK_C)
      port map (
         -- RTM Low Speed
         dtmToRtmLsP     => dtmToRtmLsP,
         dtmToRtmLsN     => dtmToRtmLsM,
         -- DPM Signals
         dpmClkP         => dpmClkP,
         dpmClkN         => dpmClkM,
         dpmFbP          => dpmFbP,
         dpmFbN          => dpmFbM,
         -- Reference 200 MHz clock
         refClk200       => sysClk200,
         refRst200       => sysClk200Rst,
         -- Reference 250 Clock
         refClk250P      => locRefClkP,
         refClk250N      => locRefClkM,
         -- AXI-Lite Interface
         axilClk         => axiClk,
         axilRst         => axiClkRst,
         axilReadMaster  => extAxilReadMaster,
         axilReadSlave   => extAxilReadSlave,
         axilWriteMaster => extAxilWriteMaster,
         axilWriteSlave  => extAxilWriteSlave);

end architecture TOP_LEVEL;
