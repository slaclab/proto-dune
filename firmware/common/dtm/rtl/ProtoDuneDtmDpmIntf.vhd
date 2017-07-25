-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmDpmIntf.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-03-20
-- Last update: 2017-07-24
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

entity ProtoDuneDtmDpmIntf is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Busy Interface
      dpmBusy    : out slv(7 downto 0);
      -- CDR Interface
      recClk     : in  sl;
      recData    : in  sl;
      cdrLocked  : in  sl;
      -- Reference 250 Clock
      refClk250P : in  sl;
      refClk250N : in  sl;
      -- DPM Ports
      dpmClkP    : out slv(2 downto 0);
      dpmClkN    : out slv(2 downto 0);
      dpmFbP     : in  slv(7 downto 0);
      dpmFbN     : in  slv(7 downto 0));
end ProtoDuneDtmDpmIntf;

architecture mapping of ProtoDuneDtmDpmIntf is

   signal recDataDdr   : sl;
   signal cdrLockedL   : sl;
   signal stableClock  : sl;
   signal stableClk    : sl;
   signal dpmMgtRefClk : sl;

begin

   cdrLockedL <= not(cdrLocked);

   U_IBUFDS_GTE2 : IBUFDS_GTE2
      port map (
         I     => refClk250P,
         IB    => refClk250N,
         CEB   => '0',
         ODIV2 => open,
         O     => stableClock);

   U_BUFG : BUFG
      port map (
         I => stableClock,
         O => stableClk);

   U_BUFGMUX : BUFGMUX
      port map (
         O  => dpmMgtRefClk,            -- 1-bit output: Clock output
         I0 => stableClk,               -- 1-bit input: Clock input (S=0)
         I1 => recClk,                  -- 1-bit input: Clock input (S=1)
         S  => cdrLocked);    -- 1-bit input: Clock select                

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
   DPM_MGT_CLK : entity work.ClkOutBufDiff
      generic map (
         TPD_G => TPD_G)
      port map (
         clkIn   => dpmMgtRefClk,
         clkOutP => dpmClkP(0),
         clkOutN => dpmClkN(0));

   -- DPM's FPGA Clock
   DPM_FPGA_CLK : entity work.ClkOutBufDiff
      generic map (
         TPD_G          => TPD_G,
         RST_POLARITY_G => '1',
         INVERT_G       => false)
      port map (
         rstIn   => cdrLockedL,
         clkIn   => recClk,
         clkOutP => dpmClkP(1),         --DPM_CLK1_P
         clkOutN => dpmClkN(1));        --DPM_CLK1_M   

   -- DPM's FPGA Data
   U_ODDR : ODDR
      generic map(
         DDR_CLK_EDGE => "OPPOSITE_EDGE",  -- "OPPOSITE_EDGE" or "SAME_EDGE" 
         INIT         => '0',  -- Initial value for Q port ('1' or '0')
         SRTYPE       => "SYNC")        -- Reset Type ("ASYNC" or "SYNC")
      port map (
         D1 => recData,                 -- 1-bit data input (positive edge)
         D2 => recData,                 -- 1-bit data input (negative edge)
         Q  => recDataDdr,              -- 1-bit DDR output
         C  => recClk,                  -- 1-bit clock input
         CE => '1',                     -- 1-bit clock enable input
         R  => cdrLockedL,              -- 1-bit reset
         S  => '0');                    -- 1-bit set

   DPM_FPGA_DATA : OBUFDS
      port map (
         I  => recDataDdr,
         O  => dpmClkP(2),              -- DPM_CLK2_P
         OB => dpmClkN(2));             --DPM_CLK2_M 


end architecture mapping;
