-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmPkg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2018-09-04
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

package ProtoDuneDtmPkg is

   type ProtoDuneDtmConfigType is record
      pdtsEndpointAddr : slv(7 downto 0);
      pdtsEndpointTgrp : slv(1 downto 0);
      busyMask         : slv(7 downto 0);
      forceBusy        : sl;
      cdrEdgeSel       : sl;
      cdrDataInv       : sl;
      emuTimingSel     : sl;
      emuClkSel        : sl;
      softRst          : sl;
      hardRst          : sl;
   end record;
   constant PROTO_DUNE_DTM_CONFIG_INIT_C : ProtoDuneDtmConfigType := (
      pdtsEndpointAddr => (others => '0'),
      pdtsEndpointTgrp => (others => '0'),
      busyMask         => (others => '0'),
      forceBusy        => '1',
      cdrEdgeSel       => '0',
      cdrDataInv       => '0',
      emuTimingSel     => '0',
      emuClkSel        => '0',
      softRst          => '0',
      hardRst          => '0');

   type ProtoDuneDtmTimingType is record
      stat      : slv(3 downto 0);
      rdy       : sl;
      syncCmd   : slv(3 downto 0);
      syncValid : sl;
      timestamp : slv(63 downto 0);
      eventCnt  : slv(31 downto 0);
   end record;
   constant PROTO_DUNE_DTM_TIMING_INIT_C : ProtoDuneDtmTimingType := (
      stat      => (others => '0'),
      rdy       => '0',
      syncCmd   => (others => '0'),
      syncValid => '0',
      timestamp => (others => '0'),
      eventCnt  => (others => '0'));

   type ProtoDuneDtmStatusType is record
      cdrLocked    : sl;
      freqMeasured : slv(31 downto 0);
      timing       : ProtoDuneDtmTimingType;
      busyVec      : slv(7 downto 0);
      busyOut      : sl;
   end record;
   constant PROTO_DUNE_DTM_STATUS_INIT_C : ProtoDuneDtmStatusType := (
      cdrLocked    => '0',
      freqMeasured => (others => '0'),
      timing       => PROTO_DUNE_DTM_TIMING_INIT_C,
      busyVec      => (others => '0'),
      busyOut      => '0');

end ProtoDuneDtmPkg;
