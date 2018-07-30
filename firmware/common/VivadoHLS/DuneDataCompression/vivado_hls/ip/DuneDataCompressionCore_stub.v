// Copyright 1986-2018 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2018.1 (lin64) Build 2188600 Wed Apr  4 18:39:19 MDT 2018
// Date        : Sun Jul 29 19:23:05 2018
// Host        : rdusr219.slac.stanford.edu running 64-bit Red Hat Enterprise Linux Server release 6.10 (Santiago)
// Command     : write_verilog -force -mode synth_stub
//               /u1/DUNE/cob_nfs/russell/proto-dune/firmware/common/VivadoHLS/DuneDataCompression/vivado_hls/ip/DuneDataCompressionCore_stub.v
// Design      : DuneDataCompressionCore
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z045ffg900-2
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
module DuneDataCompressionCore(s_axi_BUS_A_AWVALID, s_axi_BUS_A_AWREADY, 
  s_axi_BUS_A_AWADDR, s_axi_BUS_A_WVALID, s_axi_BUS_A_WREADY, s_axi_BUS_A_WDATA, 
  s_axi_BUS_A_WSTRB, s_axi_BUS_A_ARVALID, s_axi_BUS_A_ARREADY, s_axi_BUS_A_ARADDR, 
  s_axi_BUS_A_RVALID, s_axi_BUS_A_RREADY, s_axi_BUS_A_RDATA, s_axi_BUS_A_RRESP, 
  s_axi_BUS_A_BVALID, s_axi_BUS_A_BREADY, s_axi_BUS_A_BRESP, ap_clk, ap_rst_n, interrupt, 
  sAxis_TDATA, sAxis_TKEEP, sAxis_TSTRB, sAxis_TUSER, sAxis_TLAST, sAxis_TID, sAxis_TDEST, 
  mAxis_TDATA, mAxis_TKEEP, mAxis_TSTRB, mAxis_TUSER, mAxis_TLAST, mAxis_TID, mAxis_TDEST, 
  moduleIdx_V, mAxis_TVALID, mAxis_TREADY, sAxis_TVALID, sAxis_TREADY)
/* synthesis syn_black_box black_box_pad_pin="s_axi_BUS_A_AWVALID,s_axi_BUS_A_AWREADY,s_axi_BUS_A_AWADDR[9:0],s_axi_BUS_A_WVALID,s_axi_BUS_A_WREADY,s_axi_BUS_A_WDATA[31:0],s_axi_BUS_A_WSTRB[3:0],s_axi_BUS_A_ARVALID,s_axi_BUS_A_ARREADY,s_axi_BUS_A_ARADDR[9:0],s_axi_BUS_A_RVALID,s_axi_BUS_A_RREADY,s_axi_BUS_A_RDATA[31:0],s_axi_BUS_A_RRESP[1:0],s_axi_BUS_A_BVALID,s_axi_BUS_A_BREADY,s_axi_BUS_A_BRESP[1:0],ap_clk,ap_rst_n,interrupt,sAxis_TDATA[63:0],sAxis_TKEEP[7:0],sAxis_TSTRB[7:0],sAxis_TUSER[3:0],sAxis_TLAST[0:0],sAxis_TID[0:0],sAxis_TDEST[0:0],mAxis_TDATA[63:0],mAxis_TKEEP[7:0],mAxis_TSTRB[7:0],mAxis_TUSER[3:0],mAxis_TLAST[0:0],mAxis_TID[0:0],mAxis_TDEST[0:0],moduleIdx_V[0:0],mAxis_TVALID,mAxis_TREADY,sAxis_TVALID,sAxis_TREADY" */;
  input s_axi_BUS_A_AWVALID;
  output s_axi_BUS_A_AWREADY;
  input [9:0]s_axi_BUS_A_AWADDR;
  input s_axi_BUS_A_WVALID;
  output s_axi_BUS_A_WREADY;
  input [31:0]s_axi_BUS_A_WDATA;
  input [3:0]s_axi_BUS_A_WSTRB;
  input s_axi_BUS_A_ARVALID;
  output s_axi_BUS_A_ARREADY;
  input [9:0]s_axi_BUS_A_ARADDR;
  output s_axi_BUS_A_RVALID;
  input s_axi_BUS_A_RREADY;
  output [31:0]s_axi_BUS_A_RDATA;
  output [1:0]s_axi_BUS_A_RRESP;
  output s_axi_BUS_A_BVALID;
  input s_axi_BUS_A_BREADY;
  output [1:0]s_axi_BUS_A_BRESP;
  input ap_clk;
  input ap_rst_n;
  output interrupt;
  input [63:0]sAxis_TDATA;
  input [7:0]sAxis_TKEEP;
  input [7:0]sAxis_TSTRB;
  input [3:0]sAxis_TUSER;
  input [0:0]sAxis_TLAST;
  input [0:0]sAxis_TID;
  input [0:0]sAxis_TDEST;
  output [63:0]mAxis_TDATA;
  output [7:0]mAxis_TKEEP;
  output [7:0]mAxis_TSTRB;
  output [3:0]mAxis_TUSER;
  output [0:0]mAxis_TLAST;
  output [0:0]mAxis_TID;
  output [0:0]mAxis_TDEST;
  input [0:0]moduleIdx_V;
  output mAxis_TVALID;
  input mAxis_TREADY;
  input sAxis_TVALID;
  output sAxis_TREADY;
endmodule
