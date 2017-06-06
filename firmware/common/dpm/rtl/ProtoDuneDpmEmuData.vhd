-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmEmuData.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-08
-- Last update: 2017-06-05
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
use work.ProtoDuneDpmPkg.all;

entity ProtoDuneDpmEmuData is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Clock and Reset
      clk        : in  sl;
      rst        : in  sl;
      -- EMU Data Interface
      enableTx   : in  sl;
      enableTrig : in  sl;
      convt      : in  sl;
      cmNoiseCgf : in  slv(2 downto 0);
      chNoiseCgf : in  Slv3Array(127 downto 0);
      timingTrig : in  sl;
      cmNoise    : out slv(11 downto 0);
      adcData    : out Slv12Array(127 downto 0));
end ProtoDuneDpmEmuData;

architecture rtl of ProtoDuneDpmEmuData is

   function genChSeed (baseSeed : slv(31 downto 0)) return Slv32Array is
      variable retVar : Slv32Array(127 downto 0);
      variable i      : natural;
   begin
      for i in 128 downto 1 loop
         retVar(i-1) := baseSeed + toSlv((i*7), 32);
      end loop;
      return retVar;
   end function;

   constant PRBS_TAPS_C   : NaturalArray             := (0 => 31, 1 => 6, 2 => 2, 3 => 1);
   constant CM_RND_SEED_C : slv(31 downto 0)         := x"AE64B770";
   constant CH_RND_SEED_C : Slv32Array(127 downto 0) := genChSeed(CM_RND_SEED_C);

   constant CNT_SIZE_C : natural := 13;
   constant MONO_POLE_C : Slv12Array(CNT_SIZE_C-1 downto 0) := (
      0  => x"800",
      1  => x"900",
      2  => x"A00",
      3  => x"B00",
      4  => x"C00",
      5  => x"D00",
      6  => x"E00",
      7  => x"D00",
      8  => x"C00",
      9  => x"B00",
      10 => x"A00",
      11 => x"900",
      12 => x"800");
   constant BI_POLE_C : Slv12Array(CNT_SIZE_C-1 downto 0) := (
      0  => x"800",
      1  => x"B00",
      2  => x"D00",
      3  => x"E00",
      4  => x"D00",
      5  => x"B00",
      6  => x"800",
      7  => x"500",
      8  => x"300",
      9  => x"100",
      10 => x"300",
      11 => x"500",
      12 => x"800");

   type RegType is record
      cnt     : natural range 0 to CNT_SIZE_C;
      cmPbrs  : slv(31 downto 0);
      chPbrs  : Slv32Array(127 downto 0);
      adcData : Slv12Array(127 downto 0);
   end record RegType;
   constant REG_INIT_C : RegType := (
      cnt     => CNT_SIZE_C,
      cmPbrs  => CM_RND_SEED_C,
      chPbrs  => CH_RND_SEED_C,
      adcData => (others => x"800"));

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   -- attribute dont_touch             : string;
   -- attribute dont_touch of r        : signal is "TRUE";   

begin

   comb : process (chNoiseCgf, cmNoiseCgf, convt, enableTrig, enableTx, r, rst,
                   timingTrig) is
      variable v           : RegType;
      variable i           : natural;
      variable j           : natural;
      variable cmIdx       : natural;
      variable commonNoise : slv(11 downto 0);
      variable chNoise     : Slv12Array(127 downto 0);
   begin
      -- Latch the current value
      v := r;

      -- Generate common mode noise
      cmIdx       := conv_integer(cmNoiseCgf);
      commonNoise := x"000";
      if cmIdx /= 0 then
         commonNoise(cmIdx-1 downto 0) := r.cmPbrs(cmIdx-1 downto 0);
      end if;

      -- Generate channel noise
      for i in 127 downto 0 loop
         cmIdx      := conv_integer(chNoiseCgf(i));
         chNoise(i) := x"000";
         if cmIdx /= 0 then
            chNoise(i)(cmIdx-1 downto 0) := r.chPbrs(i)(cmIdx-1 downto 0);
         end if;
      end loop;

      -- Check if trigger enabled
      if (enableTrig = '1') and (timingTrig = '1') then
         -- Reset the counter
         v.cnt := 0;
      end if;

      -- Check if TX engine is enabled
      if enableTx = '1' then
         -- Check for the convert strobe
         if convt = '1' then
            -- Generate the next random data word
            for j in 31 downto 0 loop
               v.cmPbrs := lfsrShift(v.cmPbrs, PRBS_TAPS_C);
            end loop;
            for i in 127 downto 0 loop
               -- Generate the next random data word
               for j in 31 downto 0 loop
                  v.chPbrs(i) := lfsrShift(v.chPbrs(i), PRBS_TAPS_C);
               end loop;
               -- Check for IDLE state
               if r.cnt = CNT_SIZE_C then
                  v.adcData(i) := x"800" + chNoise(i);
               else
                  -- Increment the counter
                  v.cnt := r.cnt + 1;
                  -- Check the type of channel
                  if (i mod 3) = 2 then
                     v.adcData(i) := MONO_POLE_C(r.cnt) + chNoise(i);
                  else
                     v.adcData(i) := BI_POLE_C(r.cnt) + chNoise(i);
                  end if;
               end if;
            end loop;
         end if;
      else
         -- Reset the configurations
         v := REG_INIT_C;
      end if;

      -- Reset
      if (rst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      adcData <= r.adcData;
      cmNoise <= commonNoise;

   end process comb;

   seq : process (clk) is
   begin
      if rising_edge(clk) then
         r <= rin after TPD_G;
      end if;
   end process seq;

end rtl;
