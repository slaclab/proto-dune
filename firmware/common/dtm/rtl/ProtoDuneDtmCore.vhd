-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmCore.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2016-10-31
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
use work.ProtoDuneDtmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDtmCore is
   generic (
      TPD_G            : time             := 1 ns;
      AXI_CLK_FREQ_G   : real             := 125.0E+6;  -- units of Hz
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");      
   port (
      -- -- RTM High Speed
      --dtmToRtmHsP : out   sl;
      --dtmToRtmHsN : out   sl;
      --rtmToDtmHsP : in    sl;
      --rtmToDtmHsN : in    sl;  
      -- RTM Low Speed
      dtmToRtmLsP     : inout slv(5 downto 0);
      dtmToRtmLsN     : inout slv(5 downto 0);
      -- DPM Signals
      dpmClkP         : out   slv(2 downto 0);
      dpmClkN         : out   slv(2 downto 0);
      dpmFbP          : in    slv(7 downto 0);
      dpmFbN          : in    slv(7 downto 0);
      -- Reference 200 MHz clock
      refClk200       : in    sl;
      refRst200       : in    sl;
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in    sl;
      axilRst         : in    sl;
      axilReadMaster  : in    AxiLiteReadMasterType;
      axilReadSlave   : out   AxiLiteReadSlaveType;
      axilWriteMaster : in    AxiLiteWriteMasterType;
      axilWriteSlave  : out   AxiLiteWriteSlaveType);   
end ProtoDuneDtmCore;

architecture rtl of ProtoDuneDtmCore is

   constant NUM_AXIL_MASTERS_C : natural := 1;

   constant CORE_INDEX_C : natural := 0;

   constant XBAR_CONFIG_C : AxiLiteCrossbarMasterConfigArray(NUM_AXIL_MASTERS_C-1 downto 0) := genAxiLiteConfig(NUM_AXIL_MASTERS_C, AXI_BASE_ADDR_G, 28, 24);

   signal axilWriteMasters : AxiLiteWriteMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray(NUM_AXIL_MASTERS_C-1 downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray(NUM_AXIL_MASTERS_C-1 downto 0);

   signal status : ProtoDuneDtmStatusType;
   signal config : ProtoDuneDtmConfigType;

   signal timingClk  : sl;
   signal timingData : sl;
   signal sfpTx      : sl;
   signal qsfpRst    : sl;
   signal sfpTxDis   : sl;
   signal dpmBusy    : slv(7 downto 0);

begin

   ----------------
   -- RTM Interface
   ----------------
   DTM_RTM0 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(0),
         IB => dtmToRtmLsN(0),
         O  => timingClk);  

   DTM_RTM1 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(1),
         IB => dtmToRtmLsN(1),
         O  => timingData);           

   sfpTx <= '0';                        -- Unused
   DTM_RTM2 : OBUFDS
      port map (
         I  => sfpTx,
         O  => dtmToRtmLsP(2),
         OB => dtmToRtmLsN(2));    

   qsfpRst <= axilRst or config.hardRst;
   DTM_RTM3 : OBUFDS
      port map (
         I  => qsfpRst,
         O  => dtmToRtmLsP(3),
         OB => dtmToRtmLsN(3));   

   sfpTxDis <= '0';                     -- Unused
   DTM_RTM4 : OBUFDS
      port map (
         I  => sfpTxDis,
         O  => dtmToRtmLsP(4),
         OB => dtmToRtmLsN(4));

   DTM_RTM5 : OBUFDS
      port map (
         I  => status.busyOut,
         O  => dtmToRtmLsP(5),
         OB => dtmToRtmLsN(5));         

   ----------------
   -- DPM Interface
   ----------------
   -- DPM Feedback Signals
   U_DpmFbGen : for i in 0 to 7 generate
      U_DpmFbIn : IBUFDS
         generic map (
            DIFF_TERM => true)
         port map(
            I  => dpmFbP(i),
            IB => dpmFbN(i),
            O  => dpmBusy(i));
   end generate;

   -- DPM's MGT Clock
   DPM_MGT_CLK : OBUFDS
      port map(
         I  => timingClk,
         O  => dpmClkP(0),              --DPM_CLK0_P
         OB => dpmClkN(0));             --DPM_CLK0_M

   -- DPM's FPGA Clock
   DPM_FPGA_CLK : OBUFDS
      port map(
         I  => timingClk,
         O  => dpmClkP(1),              --DPM_CLK1_P
         OB => dpmClkN(1));             --DPM_CLK1_M 

   -- DPM's FPGA Data
   DPM_FPGA_DATA : OBUFDS
      port map (
         I  => timingData,
         O  => dpmClkP(2),              -- DPM_CLK2_P
         OB => dpmClkN(2));             --DPM_CLK2_M     

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

   ------------------------
   -- AXI-Lite: Core Module
   ------------------------
   U_Reg : entity work.ProtoDuneDtmReg
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)   
      port map (
         -- Status/Configuration Interface
         status          => status,
         config          => config,
         -- AXI-Lite Interface 
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMasters(CORE_INDEX_C),
         axilReadSlave   => axilReadSlaves(CORE_INDEX_C),
         axilWriteMaster => axilWriteMasters(CORE_INDEX_C),
         axilWriteSlave  => axilWriteSlaves(CORE_INDEX_C));   

   --------------
   -- Busy Module
   --------------
   U_Busy : entity work.ProtoDuneDtmBusy
      generic map (
         TPD_G => TPD_G)   
      port map (
         axilClk => axilClk,
         axilRst => axilRst,
         config  => config,
         dpmBusy => dpmBusy,
         busyVec => status.busyVec,
         busyOut => status.busyOut);

end architecture rtl;
