-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmHls.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2017-06-02
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
      -- WIB Interface (axilClk domain)
      wibMasters      : in  AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      wibSlaves       : out AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);
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

   signal ibHlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal ibHlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal obHlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal obHlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal hlsMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal hlsSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal ssiMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal ssiSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal dmaIbMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal dmaIbSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   -- attribute dont_touch                 : string;
   -- attribute dont_touch of ibHlsMasters : signal is "TRUE";
   -- attribute dont_touch of ibHlsSlaves  : signal is "TRUE";
   -- attribute dont_touch of obHlsMasters : signal is "TRUE";
   -- attribute dont_touch of obHlsSlaves  : signal is "TRUE";
   -- attribute dont_touch of dmaIbMasters : signal is "TRUE";
   -- attribute dont_touch of dmaIbSlaves  : signal is "TRUE";

begin

   --------------------
   -- AXI-Lite Crossbar
   --------------------
   U_XBAR : entity work.AxiLiteCrossbar
      generic map (
         TPD_G              => TPD_G,
         NUM_SLAVE_SLOTS_G  => 1,
         NUM_MASTER_SLOTS_G => (WIB_SIZE_C+1),
         DEC_ERROR_RESP_G   => AXI_ERROR_RESP_G,
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
            axilRst         => axilRst,
            -- AXI-Lite Port
            axilReadMaster  => axilReadMasters(i),
            axilReadSlave   => axilReadSlaves(i),
            axilWriteMaster => axilWriteMasters(i),
            axilWriteSlave  => axilWriteSlaves(i),
            -- Inbound Interface
            sAxisMaster     => ibHlsMasters(i),
            sAxisSlave      => ibHlsSlaves(i),
            -- Outbound Interface
            mAxisMaster     => obHlsMasters(i),
            mAxisSlave      => obHlsSlaves(i));

      --------------
      -- FIFO Module
      --------------    
      U_Filter : entity work.SsiFifo
         generic map (
            -- General Configurations
            TPD_G               => TPD_G,
            INT_PIPE_STAGES_G   => 1,
            PIPE_STAGES_G       => 1,
            SLAVE_READY_EN_G    => true,
            VALID_THOLD_G       => 1,
            -- FIFO configurations
            BRAM_EN_G           => false,
            GEN_SYNC_FIFO_G     => true,
            CASCADE_SIZE_G      => 1,
            FIFO_ADDR_WIDTH_G   => 4,
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
            mAxisClk    => axilClk,
            mAxisRst    => axilRst,
            mAxisMaster => ssiMasters(i),
            mAxisSlave  => ssiSlaves(i));

      U_Fifo : entity work.AxiStreamFifoV2
         generic map (
            -- General Configurations
            TPD_G               => TPD_G,
            PIPE_STAGES_G       => 0,
            -- VALID_THOLD_G       => 1,
            VALID_THOLD_G       => 128,
            VALID_BURST_MODE_G  => true,
            -- FIFO configurations
            BRAM_EN_G           => true,
            XIL_DEVICE_G        => "7SERIES",
            USE_BUILT_IN_G      => false,
            GEN_SYNC_FIFO_G     => false,
            CASCADE_SIZE_G      => 1,
            FIFO_ADDR_WIDTH_G   => 12,
            -- AXI Stream Port Configurations
            SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
            MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
         port map (
            -- Slave Port
            sAxisClk    => axilClk,
            sAxisRst    => axilRst,
            sAxisMaster => ssiMasters(i),
            sAxisSlave  => ssiSlaves(i),
            -- Master Port
            mAxisClk    => dmaClk,
            mAxisRst    => dmaRst,
            mAxisMaster => dmaIbMasters(i),
            mAxisSlave  => dmaIbSlaves(i));

   end generate GEN_LINK;

   ----------------------               
   -- AXIS Monitor Module
   ----------------------             
   U_Mon : entity work.ProtoDuneDpmHlsMon
      generic map(
         TPD_G            => TPD_G,
         AXI_CLK_FREQ_G   => AXI_CLK_FREQ_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map(
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(WIB_SIZE_C),
         axilReadSlave   => axilReadSlaves(WIB_SIZE_C),
         axilWriteMaster => axilWriteMasters(WIB_SIZE_C),
         axilWriteSlave  => axilWriteSlaves(WIB_SIZE_C),
         -- HLS Interface (axilClk domain)
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
         TPD_G          => TPD_G,
         PIPE_STAGES_G  => 1,
         NUM_SLAVES_G   => (WIB_SIZE_C),
         MODE_G         => "INDEXED",
         TDEST_LOW_G    => 0,
         ILEAVE_EN_G    => true,
         ILEAVE_REARB_G => 0)
      port map (
         -- Clock and reset
         axisClk      => dmaClk,
         axisRst      => dmaRst,
         -- Slaves
         sAxisMasters => dmaIbMasters,
         sAxisSlaves  => dmaIbSlaves,
         -- Master
         mAxisMaster  => dmaIbMaster,
         mAxisSlave   => dmaIbSlave);

end mapping;
