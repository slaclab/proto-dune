-------------------------------------------------------------------------------
-- File       : DuneDataCompressionFilter.vhd
-- Company    : SLAC National Accelerator Laboratory
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
use work.AxiStreamPkg.all;
use work.SsiPkg.all;
use work.RceG3Pkg.all;

entity DuneDataCompressionFilter is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Clock and Reset
      axisClk     : in  sl;
      axisRst     : in  sl;
      blowoff     : in  sl;
      -- Inbound Interface
      sAxisMaster : in  AxiStreamMasterType;
      -- Outbound Interface
      mAxisMaster : out AxiStreamMasterType;
      mAxisSlave  : in  AxiStreamSlaveType);
end entity DuneDataCompressionFilter;

architecture rtl of DuneDataCompressionFilter is

   type StateType is (
      MOVE_S,
      BLOWOFF_S);

   type RegType is record
      mAxisMaster : AxiStreamMasterType;
      state       : StateType;
   end record RegType;

   constant REG_INIT_C : RegType := (
      mAxisMaster => AXI_STREAM_MASTER_INIT_C,
      state       => BLOWOFF_S);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

begin

   comb : process (axisRst, mAxisSlave, r, sAxisMaster, blowoff) is
      variable v : RegType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      if (mAxisSlave.tReady = '1') then
         v.mAxisMaster.tValid := '0';
         v.mAxisMaster.tLast  := '0';
         v.mAxisMaster.tUser  := (others => '0');
      end if;

      -- Main state machine
      case r.state is
         ----------------------------------------------------------------------
         when MOVE_S =>
            -- Check for new data
            if (sAxisMaster.tValid = '1') then
               -- Check if ready to move data
               if (v.mAxisMaster.tValid = '0' or blowoff = '1' ) then
                  -- Forward the data
                  v.mAxisMaster := sAxisMaster;
               else
                  -- Indicate that there was data dropped
                  ssiSetUserEofe(RCEG3_AXIS_DMA_CONFIG_C, v.mAxisMaster, '1');
                  -- Terminate the frame
                  v.mAxisMaster.tLast := '1';
                  -- Next state
                  v.state             := BLOWOFF_S;
               end if;
            end if;
         ----------------------------------------------------------------------
         when BLOWOFF_S =>
            -- Check for last transfer to move data and ready for SOF in next cycle
            if (blowoff = '0') and (sAxisMaster.tValid = '1') and (sAxisMaster.tLast = '1') and (v.mAxisMaster.tValid = '0') then
               -- Next state
               v.state := MOVE_S;
            end if;
      ----------------------------------------------------------------------
      end case;

      -- Reset
      if (axisRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Registered Outputs
      mAxisMaster <= r.mAxisMaster;

   end process comb;

   seq : process (axisClk) is
   begin
      if (rising_edge(axisClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

end rtl;
