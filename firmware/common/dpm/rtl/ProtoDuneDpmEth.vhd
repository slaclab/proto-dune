-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmEth.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-11-01
-- Last update: 2016-11-03
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
use work.RceG3Pkg.all;
use work.EthMacPkg.all;

entity ProtoDuneDpmEth is
   generic (
      TPD_G            : time             := 1 ns;
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
      dmaObMaster     : in  AxiStreamMasterType;
      dmaObSlave      : out AxiStreamSlaveType;
      -- User ETH interface (userEthClk domain)
      ethClk          : in  sl;
      ethRst          : in  sl;
      localIp         : in  slv(31 downto 0);
      localMac        : in  slv(47 downto 0);
      ibMacMaster     : out AxiStreamMasterType;
      ibMacSlave      : in  AxiStreamSlaveType;
      obMacMaster     : in  AxiStreamMasterType;
      obMacSlave      : out AxiStreamSlaveType);
end ProtoDuneDpmEth;

architecture mapping of ProtoDuneDpmEth is

   constant AXIL_MASTERS_C  : natural := 4;
   constant UDP_INDEX_C     : natural := 0;
   constant RSSI_INDEX_C    : natural := 1;
   constant PRBS_TX_INDEX_C : natural := 2;
   constant PRBS_RX_INDEX_C : natural := 3;

   constant APP_STREAMS_C     : natural                                        := 3;
   constant APP_AXIS_CONFIG_C : AxiStreamConfigArray(APP_STREAMS_C-1 downto 0) := (others => EMAC_AXIS_CONFIG_C);

   signal syncReadMaster  : AxiLiteReadMasterType;
   signal syncReadSlave   : AxiLiteReadSlaveType;
   signal syncWriteMaster : AxiLiteWriteMasterType;
   signal syncWriteSlave  : AxiLiteWriteSlaveType;

   signal axilWriteMasters : AxiLiteWriteMasterArray(AXIL_MASTERS_C-1 downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray(AXIL_MASTERS_C-1 downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray(AXIL_MASTERS_C-1 downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray(AXIL_MASTERS_C-1 downto 0);

   signal obServerMaster : AxiStreamMasterType;
   signal obServerSlave  : AxiStreamSlaveType;
   signal ibServerMaster : AxiStreamMasterType;
   signal ibServerSlave  : AxiStreamSlaveType;

   signal rssiIbMasters : AxiStreamMasterArray(APP_STREAMS_C-1 downto 0);
   signal rssiIbSlaves  : AxiStreamSlaveArray(APP_STREAMS_C-1 downto 0);
   signal rssiObMasters : AxiStreamMasterArray(APP_STREAMS_C-1 downto 0);
   signal rssiObSlaves  : AxiStreamSlaveArray(APP_STREAMS_C-1 downto 0);

begin

   --------------------
   -- Sync AXI-Lite Bus
   --------------------
   U_AxiLiteAsync : entity work.AxiLiteAsync
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Slave Port
         sAxiClk         => axilClk,
         sAxiClkRst      => axilRst,
         sAxiReadMaster  => axilReadMaster,
         sAxiReadSlave   => axilReadSlave,
         sAxiWriteMaster => axilWriteMaster,
         sAxiWriteSlave  => axilWriteSlave,
         -- Master Port
         mAxiClk         => ethClk,
         mAxiClkRst      => ethRst,
         mAxiReadMaster  => syncReadMaster,
         mAxiReadSlave   => syncReadSlave,
         mAxiWriteMaster => syncWriteMaster,
         mAxiWriteSlave  => syncWriteSlave);

   --------------------
   -- AXI-Lite Crossbar
   --------------------
   U_XBAR : entity work.AxiLiteCrossbar
      generic map (
         TPD_G              => TPD_G,
         NUM_SLAVE_SLOTS_G  => 1,
         NUM_MASTER_SLOTS_G => AXIL_MASTERS_C,
         DEC_ERROR_RESP_G   => AXI_ERROR_RESP_G,
         MASTERS_CONFIG_G   => genAxiLiteConfig(AXIL_MASTERS_C, AXI_BASE_ADDR_G, 24, 16))
      port map (
         axiClk              => ethClk,
         axiClkRst           => ethRst,
         sAxiWriteMasters(0) => syncWriteMaster,
         sAxiWriteSlaves(0)  => syncWriteSlave,
         sAxiReadMasters(0)  => syncReadMaster,
         sAxiReadSlaves(0)   => syncReadSlave,
         mAxiWriteMasters    => axilWriteMasters,
         mAxiWriteSlaves     => axilWriteSlaves,
         mAxiReadMasters     => axilReadMasters,
         mAxiReadSlaves      => axilReadSlaves);        

   -----------
   -- IPv4/UDP
   -----------
   U_UDP : entity work.UdpEngineWrapper
      generic map (
         -- Simulation Generics
         TPD_G            => TPD_G,
         -- UDP Server Generics
         SERVER_EN_G      => true,
         SERVER_SIZE_G    => 1,
         SERVER_PORTS_G   => RSSI_PORTS_C,
         -- UDP Client Generics
         CLIENT_EN_G      => false,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Local Configurations
         localMac           => localMac,
         localIp            => localIp,
         -- Interface to Ethernet Media Access Controller (MAC)
         obMacMaster        => obMacMaster,
         obMacSlave         => obMacSlave,
         ibMacMaster        => ibMacMaster,
         ibMacSlave         => ibMacSlave,
         -- Interface to UDP Server engine(s)
         obServerMasters(0) => obServerMaster,
         obServerSlaves(0)  => obServerSlave,
         ibServerMasters(0) => ibServerMaster,
         ibServerSlaves(0)  => ibServerSlave,
         -- AXI-Lite Interface
         axilReadMaster     => axilReadMasters(UDP_INDEX_C),
         axilReadSlave      => axilReadSlaves(UDP_INDEX_C),
         axilWriteMaster    => axilWriteMasters(UDP_INDEX_C),
         axilWriteSlave     => axilWriteSlaves(UDP_INDEX_C),
         -- Clock and Reset
         clk                => ethClk,
         rst                => ethRst);           

   --------------
   -- RSSI Server
   --------------
   U_RSSI : entity work.RssiCoreWrapper
      generic map (
         TPD_G               => TPD_G,
         MAX_SEG_SIZE_G      => 1024,
         SEGMENT_ADDR_SIZE_G => 7,
         APP_STREAMS_G       => APP_STREAMS_C,
         APP_STREAM_ROUTES_G => (
            0                => "0-------",  -- TDEST [0x00:0x7F] routed to DMA
            1                => "10------",  -- TDEST [0x80:0xBF] routed to RSSI loopback
            2                => X"FF"),      -- TDEST 0xFF routed to TX/RX PRBS modules        
         CLK_FREQUENCY_G     => 156.25E+6,
         TIMEOUT_UNIT_G      => 1.0E-3,      -- In units of seconds
         AXI_ERROR_RESP_G    => AXI_ERROR_RESP_G,
         SERVER_G            => true,
         RETRANSMIT_ENABLE_G => true,
         BYPASS_CHUNKER_G    => false,
         WINDOW_ADDR_SIZE_G  => 3,
         PIPE_STAGES_G       => 1,
         APP_AXIS_CONFIG_G   => APP_AXIS_CONFIG_C,
         TSP_AXIS_CONFIG_G   => EMAC_AXIS_CONFIG_C,
         INIT_SEQ_N_G        => 16#80#)
      port map (
         clk_i             => ethClk,
         rst_i             => ethRst,
         openRq_i          => '1',
         -- Application Layer Interface
         sAppAxisMasters_i => rssiIbMasters,
         sAppAxisSlaves_o  => rssiIbSlaves,
         mAppAxisMasters_o => rssiObMasters,
         mAppAxisSlaves_i  => rssiObSlaves,
         -- Transport Layer Interface
         sTspAxisMaster_i  => obServerMaster,
         sTspAxisSlave_o   => obServerSlave,
         mTspAxisMaster_o  => ibServerMaster,
         mTspAxisSlave_i   => ibServerSlave,
         -- AXI-Lite Interface
         axiClk_i          => ethClk,
         axiRst_i          => ethRst,
         axilReadMaster    => axilReadMasters(RSSI_INDEX_C),
         axilReadSlave     => axilReadSlaves(RSSI_INDEX_C),
         axilWriteMaster   => axilWriteMasters(RSSI_INDEX_C),
         axilWriteSlave    => axilWriteSlaves(RSSI_INDEX_C));           

   ----------------------------------      
   -- SYNC DMA Bus: TDEST=[0x00:0x7F]
   ----------------------------------      
   U_DmaFifo : entity work.AxiStreamFifoV2
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
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
         FIFO_FIXED_THRESH_G => true,
         CASCADE_PAUSE_SEL_G => 0,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
         MASTER_AXI_CONFIG_G => EMAC_AXIS_CONFIG_C)            
      port map (
         -- Slave Port
         sAxisClk    => dmaClk,
         sAxisRst    => dmaRst,
         sAxisMaster => dmaObMaster,
         sAxisSlave  => dmaObSlave,
         -- Master Port
         mAxisClk    => ethClk,
         mAxisRst    => ethRst,
         mAxisMaster => rssiIbMasters(0),
         mAxisSlave  => rssiIbSlaves(0));  
   rssiObSlaves(0) <= AXI_STREAM_SLAVE_FORCE_C;

   --------------------------------------     
   -- Loopback Channel: TDEST=[0x80:0xBF]
   --------------------------------------     
   rssiIbMasters(1) <= rssiObMasters(1);
   rssiObSlaves(1)  <= rssiIbSlaves(1);

   ------------------------
   -- TX PRBS: TDEST=[0xFF]
   ------------------------
   U_SsiPrbsTx : entity work.SsiPrbsTx
      generic map (
         TPD_G                      => TPD_G,
         AXI_ERROR_RESP_G           => AXI_ERROR_RESP_G,
         CASCADE_SIZE_G             => 1,
         FIFO_ADDR_WIDTH_G          => 9,
         FIFO_PAUSE_THRESH_G        => 2**8,
         PRBS_SEED_SIZE_G           => 32,
         PRBS_TAPS_G                => (0 => 31, 1 => 6, 2 => 2, 3 => 1),
         MASTER_AXI_STREAM_CONFIG_G => EMAC_AXIS_CONFIG_C,
         MASTER_AXI_PIPE_STAGES_G   => 0)
      port map (
         mAxisClk        => ethClk,
         mAxisRst        => ethRst,
         mAxisMaster     => rssiIbMasters(2),
         mAxisSlave      => rssiIbSlaves(2),
         locClk          => ethClk,
         locRst          => ethRst,
         trig            => '0',
         packetLength    => X"0000FFFF",
         tDest           => X"FF",
         tId             => X"00",
         axilReadMaster  => axilReadMasters(PRBS_TX_INDEX_C),
         axilReadSlave   => axilReadSlaves(PRBS_TX_INDEX_C),
         axilWriteMaster => axilWriteMasters(PRBS_TX_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(PRBS_TX_INDEX_C));  

   ------------------------
   -- RX PRBS: TDEST=[0xFF]
   ------------------------
   U_SsiPrbsRx : entity work.SsiPrbsRx
      generic map (
         TPD_G                     => TPD_G,
         AXI_ERROR_RESP_G          => AXI_ERROR_RESP_G,
         CASCADE_SIZE_G            => 1,
         FIFO_ADDR_WIDTH_G         => 9,
         FIFO_PAUSE_THRESH_G       => 2**8,
         PRBS_SEED_SIZE_G          => 32,
         PRBS_TAPS_G               => (0 => 31, 1 => 6, 2 => 2, 3 => 1),
         SLAVE_AXI_STREAM_CONFIG_G => EMAC_AXIS_CONFIG_C,
         SLAVE_AXI_PIPE_STAGES_G   => 0)
      port map (
         sAxisClk       => ethClk,
         sAxisRst       => ethRst,
         sAxisMaster    => rssiObMasters(2),
         sAxisSlave     => rssiObSlaves(2),
         mAxisClk       => ethClk,
         mAxisRst       => ethRst,
         axiClk         => ethClk,
         axiRst         => ethRst,
         axiReadMaster  => axilReadMasters(PRBS_RX_INDEX_C),
         axiReadSlave   => axilReadSlaves(PRBS_RX_INDEX_C),
         axiWriteMaster => axilWriteMasters(PRBS_RX_INDEX_C),
         axiWriteSlave  => axilWriteSlaves(PRBS_RX_INDEX_C));          

end architecture mapping;
