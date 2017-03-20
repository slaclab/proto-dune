-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmBusy.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2016-10-31
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
use work.ProtoDuneDtmPkg.all;

entity ProtoDuneDtmBusy is
   generic (
      TPD_G : time := 1 ns);
   port (
      axilClk : in  sl;
      axilRst : in  sl;
      config  : in  ProtoDuneDtmConfigType;
      dpmBusy : in  slv(7 downto 0);
      busyVec : out slv(7 downto 0);
      busyOut : out sl);
end ProtoDuneDtmBusy;

architecture rtl of ProtoDuneDtmBusy is

   type RegType is record
      busyOut : sl;
   end record;

   constant REG_INIT_C : RegType := (
      busyOut => '1');

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal busy : slv(7 downto 0);

   -- attribute dont_touch               : string;
   -- attribute dont_touch of r          : signal is "TRUE";
   
begin

   busyVec <= busy;

   U_Sync : entity work.SynchronizerVector
      generic map (
         TPD_G   => TPD_G,
         WIDTH_G => 8)  
      port map (
         clk     => axilClk,
         dataIn  => dpmBusy,
         dataOut => busy);

   comb : process (axilRst, busy, config, r) is
      variable v       : RegType;
      variable i       : natural;
      variable intBusy : slv(7 downto 0);
   begin
      -- Latch the current value
      v := r;

      -- Loop through the RCE channels
      for i in 7 downto 0 loop
         intBusy(i) := busy(i) and not(config.busyMask(i));
      end loop;

      -- Update the output
      v.busyOut := config.forceBusy or uOr(intBusy);

      -- Synchronous Reset
      if (axilRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      busyOut <= r.busyOut;
      
   end process comb;

   seq : process (axilClk) is
   begin
      if (rising_edge(axilClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;
   
end rtl;
