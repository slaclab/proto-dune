-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : DuneDataCompression.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-05-02
-- Last update: 2017-01-06
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description:  
-------------------------------------------------------------------------------
-- This file is part of 'DUNE Data compression'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'DUNE Data compression', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.StdRtlPkg.all;
use work.AxiLitePkg.all;
use work.AxiStreamPkg.all;
use work.RceG3Pkg.all;

entity DuneDataCompression is
   generic (
      TPD_G   : time                 := 1 ns;
      INDEX_G : natural range 0 to 1 := 0);
   port (
      -- Clock and Reset
      axilClk         : in  sl;
      axilRst         : in  sl;
      -- AXI-Lite Port
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- Inbound Interface
      sAxisMaster     : in  AxiStreamMasterType;
      sAxisSlave      : out AxiStreamSlaveType;
      -- Outbound Interface
      mAxisMaster     : out AxiStreamMasterType;
      mAxisSlave      : in  AxiStreamSlaveType);
end DuneDataCompression;

architecture mapping of DuneDataCompression is

   -- 2017/01/06 llr -- Decreased back to 7 (not sure if I have the most up-to-date version of JJ's code or if he forgot to SVN commit this file with his other HLS code)
   -- 2016/11/29 jjr -- Increased back to 8 after adding some new stuff
   -- 2016/10/18 jjr -- Reduced after eliminating the channel-by-channel status
   constant ADDR_WIDTH_C : natural := 7;

   signal axilRstL    : sl;
   signal slaveTUser  : slv(3 downto 0);
   signal masterTUser : slv(3 downto 0);
   signal master      : AxiStreamMasterType := AXI_STREAM_MASTER_INIT_C;

begin

   axilRstL    <= not(axilRst);
   mAxisMaster <= master;

   SLAVE_TUSER :
   for i in 3 downto 0 generate
      slaveTUser(i) <= axiStreamGetUserBit(RCEG3_AXIS_DMA_CONFIG_C, sAxisMaster, i, 0) when(sAxisMaster.tLast = '0') else axiStreamGetUserBit(RCEG3_AXIS_DMA_CONFIG_C, sAxisMaster, i);
   end generate SLAVE_TUSER;

   -----------------------------------
   -- Vivado HLS Data Compression Core
   -----------------------------------
   U_Core : entity work.DuneDataCompressionCore
      port map (
         s_axi_BUS_A_AWVALID => axilWriteMaster.awvalid,
         s_axi_BUS_A_AWREADY => axilWriteSlave.awready,
         s_axi_BUS_A_AWADDR  => axilWriteMaster.awaddr(ADDR_WIDTH_C-1 downto 0),
         s_axi_BUS_A_WVALID  => axilWriteMaster.wvalid,
         s_axi_BUS_A_WREADY  => axilWriteSlave.wready,
         s_axi_BUS_A_WDATA   => axilWriteMaster.wdata,
         s_axi_BUS_A_WSTRB   => axilWriteMaster.wstrb,
         s_axi_BUS_A_ARVALID => axilReadMaster.arvalid,
         s_axi_BUS_A_ARREADY => axilReadSlave.arready,
         s_axi_BUS_A_ARADDR  => axilReadMaster.araddr(ADDR_WIDTH_C-1 downto 0),
         s_axi_BUS_A_RVALID  => axilReadSlave.rvalid,
         s_axi_BUS_A_RREADY  => axilReadMaster.rready,
         s_axi_BUS_A_RDATA   => axilReadSlave.rdata,
         s_axi_BUS_A_RRESP   => axilReadSlave.rresp,
         s_axi_BUS_A_BVALID  => axilWriteSlave.bvalid,
         s_axi_BUS_A_BREADY  => axilWriteMaster.bready,
         s_axi_BUS_A_BRESP   => axilWriteSlave.bresp,
         ap_clk              => axilClk,
         ap_rst_n            => axilRstL,
         interrupt           => open,
         -- Inbound Interface
         sAxis_TVALID        => sAxisMaster.tValid,
         sAxis_TDATA         => sAxisMaster.tData(63 downto 0),
         sAxis_TKEEP         => sAxisMaster.tKeep(7 downto 0),
         sAxis_TSTRB         => (others => '1'),
         sAxis_TUSER         => slaveTUser,
         sAxis_TLAST(0)      => sAxisMaster.tLast,
         sAxis_TID           => (others => '0'),
         sAxis_TDEST         => (others => '0'),
         sAxis_TREADY        => sAxisSlave.tReady,
         -- Outbound Interface
         mAxis_TVALID        => master.tValid,
         mAxis_TDATA         => master.tData(63 downto 0),
         mAxis_TKEEP         => master.tKeep(7 downto 0),
         mAxis_TSTRB         => open,
         mAxis_TUSER         => masterTUser,
         mAxis_TLAST(0)      => master.tLast,
         mAxis_TID           => open,
         mAxis_TDEST         => open,
         mAxis_TREADY        => mAxisSlave.tReady,
         -- Misc. 
         moduleIdx_V         => toSlv(INDEX_G, 1));

   MASTER_TUSER :
   for i in 7 downto 0 generate
      master.tUser((8*i)+7 downto (8*i)) <= (x"0" & masterTUser);
   end generate MASTER_TUSER;

end mapping;