-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmRtmIntf.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-03-20
-- Last update: 2017-03-20
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

entity ProtoDuneDtmRtmIntf is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Clocks and Resets
      axilClk      : in  sl;
      axilRst      : in  sl;
      refClk200    : in  sl;
      refRst200    : in  sl;
      -- RTM Interface
      hardRst      : in  sl;
      busyOut      : in  sl;
      sfpTxDis     : in  sl;
      sfpTx        : in  sl;
      -- Status (axilClk domain)
      cdrLocked    : out sl;
      freqMeasured : out slv(31 downto 0);
      -- CDR Interface
      recClk       : out sl;
      recData      : out sl;
      recLol       : out sl);
end ProtoDuneDtmRtmIntf;

architecture mapping of ProtoDuneDtmRtmIntf is

   signal qsfpRst : sl;
   signal locked  : sl;

begin

   U_ClkMon : entity work.ProtoDuneDtmCdrMmcm
      generic map (
         TPD_G             => TPD_G,
         LOC_CLK_FREQ_G    => 125.0E+6,  -- Local clock frequency (in units of Hz)
         REF_CLK_FREQ_G    => 200.0E+6,  -- Reference clock frequency (in units of Hz)
         CLK_FREQ_TARGET_G => 250.0E+6,  -- CDR's targeted clock frequency (in units of Hz)
         CLK_FREQ_BW_G     => 1.0E+6,  -- CDR's allow frequency bandwidth (in units of Hz)
         REFRESH_RATE_G    => 1.0E+3)  -- Clock frequency refresh  (in units of Hz)
      port map (
         -- Input CDR clock
         clkIn         => timingClk,
         -- Reference clock
         refClk        => refClk200,
         refRst        => refRst200,
         -- Clock Monitor Interface (locClk domain)
         locClk        => axilClk,
         freqMonlocked => locked,
         freqMeasured  => freqMeasured);

   qsfpRst <= axilRst or hardRst;

   cdrLocked <= locked;
   recLol    <= not(locked);

   ----------------
   -- RTM Interface
   ----------------
   DTM_RTM0 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(0),
         IB => dtmToRtmLsN(0),
         O  => timingClock);

   U_Bufg : BUFG
      port map (
         I => timingClock,
         O => timingClk);

   DTM_RTM1 : IBUFDS
      generic map (
         DIFF_TERM => true)
      port map(
         I  => dtmToRtmLsP(1),
         IB => dtmToRtmLsN(1),
         O  => cdrData);

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
