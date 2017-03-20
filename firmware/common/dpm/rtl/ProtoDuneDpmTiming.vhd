-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmTiming.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2016-10-07
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

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDpmTiming is
   generic (
      TPD_G            : time             := 1 ns;
      CASCADE_SIZE_G   : positive         := 1;
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");
   port (
      -- Timing Interface (clk domain)
      clk             : in  sl;
      rst             : in  sl;
      emuEnable       : in  sl;
      runEnable       : out sl;
      swFlush         : out sl;
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- DTM Interface
      dtmRefClkP      : in  sl;
      dtmRefClkN      : in  sl;
      dtmClkP         : in  slv(1 downto 0);
      dtmClkN         : in  slv(1 downto 0);
      dtmFbP          : in  sl;
      dtmFbN          : in  sl);       
end ProtoDuneDpmTiming;

architecture mapping of ProtoDuneDpmTiming is

   signal dtmClk          : slv(1 downto 0);
   signal dtmFb           : sl;
   signal timingRunEnable : sl;
   signal runEn           : sl;

   attribute dont_touch           : string;
   attribute dont_touch of dtmClk : signal is "TRUE";
   attribute dont_touch of dtmFb  : signal is "TRUE";

   attribute KEEP_HIERARCHY               : string;
   attribute KEEP_HIERARCHY of U_IBUFGDS0 : label is "TRUE";
   attribute KEEP_HIERARCHY of U_IBUFGDS1 : label is "TRUE";
   attribute KEEP_HIERARCHY of U_IBUFGDS2 : label is "TRUE";
   
begin

   runEnable <= runEn;
   runEn     <= timingRunEnable or emuEnable;

   U_IBUFGDS0 : IBUFGDS
      generic map (
         DIFF_TERM => true)
      port map (
         I  => dtmClkP(0),
         IB => dtmClkN(0),
         O  => dtmClk(0));  

   U_IBUFGDS1 : IBUFGDS
      generic map (
         DIFF_TERM => true)
      port map (
         I  => dtmClkP(1),
         IB => dtmClkN(1),
         O  => dtmClk(1));

   U_IBUFGDS2 : IBUFGDS
      generic map (
         DIFF_TERM => true)
      port map (
         I  => dtmFbP,
         IB => dtmFbN,
         O  => dtmFb);         

   U_TimingReg : entity work.ProtoDuneDpmTimingReg
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Timing Interface (clk domain)
         clk             => clk,
         rst             => rst,
         runEnable       => runEn,
         swFlush         => swFlush,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave);


   -- place holder for future code
   timingRunEnable <= '0';
   
end mapping;
