-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmCdrMmcm.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2017-02-02
-- Last update: 2017-02-02
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
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

use work.StdRtlPkg.all;

entity ProtoDuneDtmCdrMmcm is
   generic (
      TPD_G             : time := 1 ns;
      LOC_CLK_FREQ_G    : real := 125.0E+6;  -- Local clock frequency (in units of Hz)
      REF_CLK_FREQ_G    : real := 200.0E+6;  -- Reference clock frequency (in units of Hz)
      CLK_FREQ_TARGET_G : real := 250.0E+6;  -- CDR's targeted clock frequency (in units of Hz)
      CLK_FREQ_BW_G     : real := 1.0E+6;  -- CDR's allow frequency bandwidth (in units of Hz)
      REFRESH_RATE_G    : real := 1.0E+3);  -- Clock frequency refresh  (in units of Hz)
   port (
      -- Input CDR clock
      clkIn         : in  sl;
      -- Reference clock
      refClk        : in  sl;
      refRst        : in  sl;
      -- Clock Monitor Interface (locClk domain)
      locClk        : in  sl;
      freqMonlocked : out sl;
      freqMeasured  : out slv(31 downto 0);  -- units of Hz  
      mmcmLocked    : out sl;           -- MMCM locked status
      -- MMCM clock and reset Output  
      clkOut        : out sl;           -- MMCM clock output
      rstOut        : out sl);          -- MMCM synchronous reset output
end ProtoDuneDtmCdrMmcm;

architecture mapping of ProtoDuneDtmCdrMmcm is

   constant DURATION_100MS_C  : natural := getTimeRatio(LOC_CLK_FREQ_G, 10.0);
   constant CLK_UPPER_LIMIT_C : real    := CLK_FREQ_TARGET_G + (CLK_FREQ_BW_G/2);
   constant CLK_LOWER_LIMIT_C : real    := CLK_FREQ_TARGET_G - (CLK_FREQ_BW_G/2);

   signal locked : slv(1 downto 0);
   signal rstIn  : sl;

begin

   -- Measure the clock frequency and determine if (CLK_LOWER_LIMIT_G < clkIn < CLK_UPPER_LIMIT_G)
   U_ClockFreq : entity work.SyncClockFreq
      generic map (
         TPD_G             => TPD_G,
         REF_CLK_FREQ_G    => REF_CLK_FREQ_G,
         REFRESH_RATE_G    => REFRESH_RATE_G,
         CLK_UPPER_LIMIT_G => CLK_UPPER_LIMIT_C,
         CLK_LOWER_LIMIT_G => CLK_LOWER_LIMIT_C,
         CNT_WIDTH_G       => 32)
      port map (
         -- Frequency Measurement and Monitoring Outputs (locClk domain)
         freqOut => freqMeasured,
         locked  => locked(0),
         -- Clocks
         clkIn   => clkIn,
         locClk  => locClk,
         refClk  => refClk);

   -- If not(CLK_LOWER_LIMIT_G < clkIn < CLK_UPPER_LIMIT_G), force reset on MMCM
   U_PwrUpRst : entity work.PwrUpRst
      generic map(
         TPD_G          => TPD_G,
         IN_POLARITY_G  => '0',
         OUT_POLARITY_G => '1',
         DURATION_G     => DURATION_100MS_C)
      port map (
         arst   => locked(0),
         clk    => locClk,
         rstOut => rstIn);

   U_MMCM : entity work.ClockManager7
      generic map(
         TPD_G              => TPD_G,
         TYPE_G             => "MMCM",
         INPUT_BUFG_G       => false,
         FB_BUFG_G          => true,
         RST_IN_POLARITY_G  => '1',
         NUM_CLOCKS_G       => 1,
         -- MMCM attributes
         BANDWIDTH_G        => "OPTIMIZED",
         CLKIN_PERIOD_G     => 4.0,     -- 250 MHz 
         DIVCLK_DIVIDE_G    => 1,       -- 250 MHz = 250 MHz/1
         CLKFBOUT_MULT_F_G  => 4.0,     -- 1GHz = 4 x 250 MHz
         CLKOUT0_DIVIDE_F_G => 4.0)     -- 250 MHz = 1GHz/4
      port map(
         clkIn     => clkIn,
         rstIn     => rstIn,
         clkOut(0) => clkOut,
         rstOut(0) => rstOut,
         locked    => locked(1));

   freqMonlocked <= locked(0);

   U_mmcmLocked : entity work.Synchronizer
      generic map (
         TPD_G => TPD_G)
      port map (
         clk     => locClk,
         dataIn  => locked(1),
         dataOut => mmcmLocked);

end architecture;
