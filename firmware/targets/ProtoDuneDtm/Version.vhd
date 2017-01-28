------------------------------------------------------------------------------
-- This file is part of 'DUNE Development Firmware'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'DUNE Development Firmware', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

package Version is

   constant FPGA_VERSION_C : std_logic_vector(31 downto 0) := x"00000002";  -- MAKE_VERSION

   constant BUILD_STAMP_C : string := "ProtoDuneDtm: Vivado v2016.2 (x86_64) Built Fri Jan 27 16:43:54 PST 2017 by ruckman";

end Version;

-------------------------------------------------------------------------------
-- Revision History:
--
-- 01/27/2016 (0x00000002): Rebuilding with example ILA core
-- 10/28/2016 (0x00000001): Initial version
--
-------------------------------------------------------------------------------

