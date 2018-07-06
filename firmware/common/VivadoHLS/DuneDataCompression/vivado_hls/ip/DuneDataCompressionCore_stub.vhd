-- Copyright 1986-2018 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2018.1 (lin64) Build 2188600 Wed Apr  4 18:39:19 MDT 2018
-- Date        : Thu Jul  5 23:16:06 2018
-- Host        : rdusr219.slac.stanford.edu running 64-bit Red Hat Enterprise Linux Server release 6.10 (Santiago)
-- Command     : write_vhdl -force -mode synth_stub
--               /u/ey/rherbst/projects/lbne/proto-dune_rth_merged/firmware/common/VivadoHLS/DuneDataCompression/vivado_hls/ip/DuneDataCompressionCore_stub.vhd
-- Design      : DuneDataCompressionCore
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z045ffg900-2
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity DuneDataCompressionCore is
  Port ( 
    s_axi_BUS_A_AWVALID : in STD_LOGIC;
    s_axi_BUS_A_AWREADY : out STD_LOGIC;
    s_axi_BUS_A_AWADDR : in STD_LOGIC_VECTOR ( 9 downto 0 );
    s_axi_BUS_A_WVALID : in STD_LOGIC;
    s_axi_BUS_A_WREADY : out STD_LOGIC;
    s_axi_BUS_A_WDATA : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_BUS_A_WSTRB : in STD_LOGIC_VECTOR ( 3 downto 0 );
    s_axi_BUS_A_ARVALID : in STD_LOGIC;
    s_axi_BUS_A_ARREADY : out STD_LOGIC;
    s_axi_BUS_A_ARADDR : in STD_LOGIC_VECTOR ( 9 downto 0 );
    s_axi_BUS_A_RVALID : out STD_LOGIC;
    s_axi_BUS_A_RREADY : in STD_LOGIC;
    s_axi_BUS_A_RDATA : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_BUS_A_RRESP : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_BUS_A_BVALID : out STD_LOGIC;
    s_axi_BUS_A_BREADY : in STD_LOGIC;
    s_axi_BUS_A_BRESP : out STD_LOGIC_VECTOR ( 1 downto 0 );
    ap_clk : in STD_LOGIC;
    ap_rst_n : in STD_LOGIC;
    interrupt : out STD_LOGIC;
    sAxis_TDATA : in STD_LOGIC_VECTOR ( 63 downto 0 );
    sAxis_TKEEP : in STD_LOGIC_VECTOR ( 7 downto 0 );
    sAxis_TSTRB : in STD_LOGIC_VECTOR ( 7 downto 0 );
    sAxis_TUSER : in STD_LOGIC_VECTOR ( 3 downto 0 );
    sAxis_TLAST : in STD_LOGIC_VECTOR ( 0 to 0 );
    sAxis_TID : in STD_LOGIC_VECTOR ( 0 to 0 );
    sAxis_TDEST : in STD_LOGIC_VECTOR ( 0 to 0 );
    mAxis_TDATA : out STD_LOGIC_VECTOR ( 63 downto 0 );
    mAxis_TKEEP : out STD_LOGIC_VECTOR ( 7 downto 0 );
    mAxis_TSTRB : out STD_LOGIC_VECTOR ( 7 downto 0 );
    mAxis_TUSER : out STD_LOGIC_VECTOR ( 3 downto 0 );
    mAxis_TLAST : out STD_LOGIC_VECTOR ( 0 to 0 );
    mAxis_TID : out STD_LOGIC_VECTOR ( 0 to 0 );
    mAxis_TDEST : out STD_LOGIC_VECTOR ( 0 to 0 );
    moduleIdx_V : in STD_LOGIC_VECTOR ( 0 to 0 );
    mAxis_TVALID : out STD_LOGIC;
    mAxis_TREADY : in STD_LOGIC;
    sAxis_TVALID : in STD_LOGIC;
    sAxis_TREADY : out STD_LOGIC
  );

end DuneDataCompressionCore;

architecture stub of DuneDataCompressionCore is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "s_axi_BUS_A_AWVALID,s_axi_BUS_A_AWREADY,s_axi_BUS_A_AWADDR[9:0],s_axi_BUS_A_WVALID,s_axi_BUS_A_WREADY,s_axi_BUS_A_WDATA[31:0],s_axi_BUS_A_WSTRB[3:0],s_axi_BUS_A_ARVALID,s_axi_BUS_A_ARREADY,s_axi_BUS_A_ARADDR[9:0],s_axi_BUS_A_RVALID,s_axi_BUS_A_RREADY,s_axi_BUS_A_RDATA[31:0],s_axi_BUS_A_RRESP[1:0],s_axi_BUS_A_BVALID,s_axi_BUS_A_BREADY,s_axi_BUS_A_BRESP[1:0],ap_clk,ap_rst_n,interrupt,sAxis_TDATA[63:0],sAxis_TKEEP[7:0],sAxis_TSTRB[7:0],sAxis_TUSER[3:0],sAxis_TLAST[0:0],sAxis_TID[0:0],sAxis_TDEST[0:0],mAxis_TDATA[63:0],mAxis_TKEEP[7:0],mAxis_TSTRB[7:0],mAxis_TUSER[3:0],mAxis_TLAST[0:0],mAxis_TID[0:0],mAxis_TDEST[0:0],moduleIdx_V[0:0],mAxis_TVALID,mAxis_TREADY,sAxis_TVALID,sAxis_TREADY";
begin
end;
