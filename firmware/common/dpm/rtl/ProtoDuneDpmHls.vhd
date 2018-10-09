-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmHls.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2018-09-18
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
use work.SsiPkg.all;
use work.ProtoDuneDpmPkg.all;
use work.RceG3Pkg.all;

entity ProtoDuneDpmHls is
   generic (
      TPD_G           : time             := 1 ns;
      CASCADE_SIZE_G  : positive         := 1;
      AXI_CLK_FREQ_G  : real             := 125.0E+6;  -- units of Hz
      AXI_BASE_ADDR_G : slv(31 downto 0) := x"A0000000");
   port (
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- WIB Interface (axilClk domain)
      wibMasters      : in  AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      wibSlaves       : out AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);
      -- DMA Loopback Path Interface (dmaClk domain)
      loopbackMaster  : in  AxiStreamMasterType;
      loopbackSlave   : out AxiStreamSlaveType;
      -- AXI Stream Interface (dmaClk domain)
      dmaClk          : in  sl;
      dmaRst          : in  sl;
      dmaIbMaster     : out AxiStreamMasterType;
      dmaIbSlave      : in  AxiStreamSlaveType);
end ProtoDuneDpmHls;

architecture mapping of ProtoDuneDpmHls is

   signal axilWriteMasters : AxiLiteWriteMasterArray(WIB_SIZE_C downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray(WIB_SIZE_C downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray(WIB_SIZE_C downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray(WIB_SIZE_C downto 0);

   signal ibHlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal ibHlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);

   signal obMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);

   signal obHlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal obHlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);

   signal hlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal hlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);

   signal dmaIbMasters : AxiStreamMasterArray(WIB_SIZE_C downto 0) := (others => AXI_STREAM_MASTER_INIT_C);
   signal dmaIbSlaves  : AxiStreamSlaveArray(WIB_SIZE_C downto 0)  := (others => AXI_STREAM_SLAVE_FORCE_C);

   signal hlsRst   : sl;
   signal hlsReset : sl;

   attribute dont_touch                 : string;
   attribute dont_touch of ibHlsMasters : signal is "TRUE";
   attribute dont_touch of ibHlsSlaves  : signal is "TRUE";
   attribute dont_touch of obMasters    : signal is "TRUE";
   attribute dont_touch of obHlsMasters : signal is "TRUE";
   attribute dont_touch of obHlsSlaves  : signal is "TRUE";
   attribute dont_touch of hlsMasters   : signal is "TRUE";
   attribute dont_touch of hlsSlaves    : signal is "TRUE";
   attribute dont_touch of dmaIbMasters : signal is "TRUE";
   attribute dont_touch of dmaIbSlaves  : signal is "TRUE";

begin

   hlsReset <= hlsRst or axilRst;

   --------------------
   -- AXI-Lite Crossbar
   --------------------
   U_XBAR : entity work.AxiLiteCrossbar
      generic map (
         TPD_G              => TPD_G,
         NUM_SLAVE_SLOTS_G  => 1,
         NUM_MASTER_SLOTS_G => (WIB_SIZE_C+1),
         MASTERS_CONFIG_G   => genAxiLiteConfig((WIB_SIZE_C+1), AXI_BASE_ADDR_G, 24, 16))
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

   GEN_LINK :
   for i in (WIB_SIZE_C-1) downto 0 generate


      ibHlsMasters(i) <= wibMasters(i);
      wibSlaves(i)    <= ibHlsSlaves(i);

      -------------
      -- HLS Module
      -------------  
      U_HLS : entity work.DuneDataCompression
         generic map (
            TPD_G   => TPD_G,
            INDEX_G => i)
         port map (
            -- Clock and Reset
            axilClk         => axilClk,
            axilRst         => hlsReset,
            -- AXI-Lite Port
            axilReadMaster  => axilReadMasters(i),
            axilReadSlave   => axilReadSlaves(i),
            axilWriteMaster => axilWriteMasters(i),
            axilWriteSlave  => axilWriteSlaves(i),
            -- Inbound Interface
            sAxisMaster     => ibHlsMasters(i),
            sAxisSlave      => ibHlsSlaves(i),
            -- Outbound Interface
            mAxisMaster     => obMasters(i),
            mAxisSlave      => AXI_STREAM_SLAVE_FORCE_C);  -- Never back pressure the HLS module

      -------------------    
      -- Filter Out Drops
      -------------------    
      U_Filter : entity work.DuneDataCompressionFilter
         generic map (
            TPD_G => TPD_G)
         port map (
            -- Clock and Reset
            axisClk     => axilClk,
            axisRst     => axilRst,
            -- Inbound Interface
            sAxisMaster => obMasters(i),
            -- Outbound Interface
            mAxisMaster => obHlsMasters(i),
            mAxisSlave  => obHlsSlaves(i));

      --------------
      -- Packet FIFO
      --------------              
      U_Fifo : entity work.AxiStreamFifoV2
         generic map (
            -- General Configurations
            TPD_G               => TPD_G,
            INT_PIPE_STAGES_G   => 1,
            PIPE_STAGES_G       => 1,
            SLAVE_READY_EN_G    => true,
            VALID_THOLD_G       => 128,  -- Hold until enough to burst into the interleaving MUX
            VALID_BURST_MODE_G  => true,
            -- FIFO configurations
            BRAM_EN_G           => true,
            GEN_SYNC_FIFO_G     => false,
            FIFO_ADDR_WIDTH_G   => 12,
            -- AXI Stream Port Configurations
            SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
            MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
         port map (
            -- Slave Port
            sAxisClk    => axilClk,
            sAxisRst    => axilRst,
            sAxisMaster => hlsMasters(i),
            sAxisSlave  => hlsSlaves(i),
            -- Master Port
            mAxisClk    => dmaClk,
            mAxisRst    => dmaRst,
            mAxisMaster => dmaIbMasters(i),
            mAxisSlave  => dmaIbSlaves(i));

   end generate GEN_LINK;

   -- Connect the loopback module
   dmaIbMasters(WIB_SIZE_C) <= loopbackMaster;
   loopbackSlave            <= dmaIbSlaves(WIB_SIZE_C);

   ----------------------               
   -- AXIS Monitor Module
   ----------------------             
   U_Mon : entity work.ProtoDuneDpmHlsMon
      generic map(
         TPD_G          => TPD_G,
         AXI_CLK_FREQ_G => AXI_CLK_FREQ_G)
      port map(
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(WIB_SIZE_C),
         axilReadSlave   => axilReadSlaves(WIB_SIZE_C),
         axilWriteMaster => axilWriteMasters(WIB_SIZE_C),
         axilWriteSlave  => axilWriteSlaves(WIB_SIZE_C),
         -- HLS Interface (axilClk domain)
         hlsRst          => hlsRst,
         ibHlsMasters    => ibHlsMasters,
         ibHlsSlaves     => ibHlsSlaves,
         obHlsMasters    => obHlsMasters,
         obHlsSlaves     => obHlsSlaves,
         hlsMasters      => hlsMasters,
         hlsSlaves       => hlsSlaves);

   --------------
   -- MUX Module
   --------------               
   U_Mux : entity work.AxiStreamMux
      generic map (
         TPD_G                => TPD_G,
         NUM_SLAVES_G         => (WIB_SIZE_C+1),
         MODE_G               => "INDEXED",
         ILEAVE_EN_G          => true,
         ILEAVE_ON_NOTVALID_G => false,
         ILEAVE_REARB_G       => 128,
         PIPE_STAGES_G        => 1)
      port map (
         -- Clock and reset
         axisClk      => dmaClk,
         axisRst      => dmaRst,
         -- Slaves
         sAxisMasters => dmaIbMasters,
         sAxisSlaves  => dmaIbSlaves,
         -- Master
         --mAxisMaster  => dmaIbMaster,
         --mAxisSlave   => dmaIbSlave);
         mAxisMaster  => open,
         mAxisSlave   => dmaIbSlave);

   dmaIbMaster <= AXI_STREAM_MASTER_INIT_C;

end mapping;
