-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmPkg.vhd
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

package ProtoDuneDtmPkg is

   type ProtoDuneDtmConfigType is record
      busyMask     : slv(7 downto 0);
      forceBusy    : sl;
      emuTimingSel : sl;
      emuClkSel    : sl;
      softRst      : sl;
      hardRst      : sl;
   end record;
   constant PROTO_DUNE_DTM_CONFIG_INIT_C : ProtoDuneDtmConfigType := (
      busyMask     => (others => '0'),
      forceBusy    => '1',
      emuTimingSel => '0',
      emuClkSel    => '0',
      softRst      => '0',
      hardRst      => '0');      

   type ProtoDuneDtmStatusType is record
      busyVec : slv(7 downto 0);
      busyOut : sl;
   end record;
   constant PROTO_DUNE_DTM_STATUS_INIT_C : ProtoDuneDtmStatusType := (
      busyVec => (others => '0'),
      busyOut => '0');

end ProtoDuneDtmPkg;
