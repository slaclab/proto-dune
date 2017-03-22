-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmRtmIntf.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-03-20
-- Last update: 2017-03-22
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

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDtmRtmIntf is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Control Interface
      cdrEdgeSel  : in    sl;
      cdrDataInv  : in    sl;
      qsfpRst     : in    sl;
      busyOut     : in    sl;
      sfpTxDis    : in    sl;
      sfpTx       : in    sl;
      -- CDR Interface
      recClk      : out   sl;
      recData     : out   sl;
      -- RTM Low Speed Ports
      dtmToRtmLsP : inout slv(5 downto 0);
      dtmToRtmLsN : inout slv(5 downto 0));
end ProtoDuneDtmRtmIntf;

architecture mapping of ProtoDuneDtmRtmIntf is

   signal clock : sl;
   signal clk   : sl;
   signal data  : sl;
   signal Q1    : sl;
   signal Q2    : sl;

begin

   recClk <= clk;

   ----------------
   -- RTM Interface
   ----------------
   DTM_RTM0 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(0),
         IB => dtmToRtmLsN(0),
         O  => clock);

   U_BUFG : BUFG
      port map (
         I => clock,
         O => clk);

   DTM_RTM1 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(1),
         IB => dtmToRtmLsN(1),
         O  => data);

   U_IDDR : IDDR
      generic map (
         DDR_CLK_EDGE => "SAME_EDGE_PIPELINED",  -- "OPPOSITE_EDGE", "SAME_EDGE", or "SAME_EDGE_PIPELINED"
         INIT_Q1      => '0',           -- Initial value of Q1: '0' or '1'
         INIT_Q2      => '0',           -- Initial value of Q2: '0' or '1'
         SRTYPE       => "SYNC")        -- Set/Reset type: "SYNC" or "ASYNC" 
      port map (
         D  => data,                    -- 1-bit DDR data input
         C  => clk,                     -- 1-bit clock input
         CE => '1',                     -- 1-bit clock enable input
         R  => '0',                     -- 1-bit reset
         S  => '0',                     -- 1-bit set
         Q1 => Q1,   -- 1-bit output for positive edge of clock 
         Q2 => Q2);  -- 1-bit output for negative edge of clock          

   process(clk)
   begin
      if rising_edge(clk) then
         if (cdrEdgeSel = '0') then
            recData <= Q1 xor cdrDataInv after TPD_G;
         else
            recData <= Q2 xor cdrDataInv after TPD_G;
         end if;
      end if;
   end process;

   DTM_RTM2 : OBUFDS
      port map (
         I  => sfpTx,
         O  => dtmToRtmLsP(2),
         OB => dtmToRtmLsN(2));

   DTM_RTM3 : OBUFDS
      port map (
         I  => qsfpRst,
         O  => dtmToRtmLsP(3),
         OB => dtmToRtmLsN(3));

   DTM_RTM4 : OBUFDS
      port map (
         I  => sfpTxDis,
         O  => dtmToRtmLsP(4),
         OB => dtmToRtmLsN(4));

   DTM_RTM5 : OBUFDS
      port map (
         I  => busyOut,
         O  => dtmToRtmLsP(5),
         OB => dtmToRtmLsN(5));

end architecture mapping;
