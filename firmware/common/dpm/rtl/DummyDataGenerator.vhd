-------------------------------------------------------------------------------
-- File       : DummyDataGenerator.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-05-02
-- Last update: 2017-06-19
-------------------------------------------------------------------------------
-- Description:  
-------------------------------------------------------------------------------
-- This file is part of 'DUNE Data compression'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'DUNE Data compression', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;

use work.StdRtlPkg.all;
use work.AxiLitePkg.all;
use work.AxiStreamPkg.all;
use work.SsiPkg.all;
use work.RceG3Pkg.all;

entity DummyDataGenerator is
   generic (
      TPD_G   : time    := 1 ns;
      INDEX_G : natural := 0);
   port (
      -- Clock and Reset
      axilClk         : in  sl;
      axilRst         : in  sl;
      -- AXI-Lite Port
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- Inbound Interface
      sAxisMaster     : in  AxiStreamMasterType;
      sAxisSlave      : out AxiStreamSlaveType;
      -- Outbound Interface
      mAxisMaster     : out AxiStreamMasterType;
      mAxisSlave      : in  AxiStreamSlaveType);
end DummyDataGenerator;

architecture mapping of DummyDataGenerator is

   type StateType is (
      HDR_S,
      MOVE_S,
      TAIL_S);

   type RegType is record
      byte        : Slv8Array(7 downto 0);
      cnt         : slv(4 downto 0);
      frame       : slv(10 downto 0);
      mAxisMaster : AxiStreamMasterType;
      state       : StateType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      byte        => (others => x"00"),
      cnt         => toSlv(1,5),
      frame       => toSlv(1,11),
      mAxisMaster => AXI_STREAM_MASTER_INIT_C,
      state       => HDR_S);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

begin

   sAxisSlave <= AXI_STREAM_SLAVE_FORCE_C;

   U_AxiLiteEmpty : entity work.AxiLiteEmpty
      generic map (
         TPD_G            => TPD_G,
         AXI_ERROR_RESP_G => AXI_RESP_OK_C)
      port map (
         axiClk         => axilClk,
         axiClkRst      => axilRst,
         axiReadMaster  => axilReadMaster,
         axiReadSlave   => axilReadSlave,
         axiWriteMaster => axilWriteMaster,
         axiWriteSlave  => axilWriteSlave);

   comb : process (axilRst, mAxisSlave, r) is
      variable v        : RegType;
      variable i        : natural;
      variable pattern  : slv(63 downto 0);
   begin
      -- Latch the current value
      v := r;

      -- Reset the flags
      if (mAxisSlave.tReady = '1') then
         v.mAxisMaster.tValid := '0';
         v.mAxisMaster.tLast  := '0';
         v.mAxisMaster.tUser  := (others => '0');
         v.mAxisMaster.tKeep  := (others => '1');
      end if;

      -- Create the pattern
      for i in 7 downto 0 loop
         pattern(7+(8*i) downto (8*i)) := r.byte(i);
      end loop;

      -- Check if ready to move data
      if (v.mAxisMaster.tValid = '0') then
         -- Set the flag
         v.mAxisMaster.tValid := '1';
         -- State Machine
         case r.state is
            ----------------------------------------------------------------------
            when HDR_S =>
               -- Set SOF
               ssiSetUserSof(RCEG3_AXIS_DMA_CONFIG_C, v.mAxisMaster, '1');
               -- Set the data bus
               v.mAxisMaster.tData(63 downto 0) := x"0000_0001_1100_0000";
               -- Update the pattern
               for i in 7 downto 0 loop
                  v.byte(i) := toSlv(i, 8);
               end loop;
               -- Next state
               v.state := MOVE_S;
            ----------------------------------------------------------------------
            when MOVE_S =>
               -- Set the data bus
               v.mAxisMaster.tData(63 downto 0) := pattern;
               -- Update the pattern
               for i in 7 downto 0 loop
                  v.byte(i) := r.byte(i) + 8;
               end loop;
               -- Check the WIB packet counter
               if r.cnt = 30 then
                  -- Reset the counter
                  v.cnt := toSlv(1,5);
                  -- Check the frame counter
                  if r.frame = 1024 then
                     -- Reset the counter                  
                     v.frame := toSlv(1,11);
                     v.state := TAIL_S;
                  else
                     -- Increment the counter
                     v.frame := r.frame + 1;
                     -- Update the pattern
                     for i in 7 downto 0 loop
                        v.byte(i) := r.frame(7 downto 0) + i;
                     end loop;
                  end if;
               else
                  -- Increment the counter
                  v.cnt := r.cnt + 1;
               end if;
            ----------------------------------------------------------------------
            when TAIL_S =>
               -- Set EOF
               v.mAxisMaster.tLast              := '1';
               -- Set the data bus
               v.mAxisMaster.tData(63 downto 0) := x"708b_309e_1103_c010";
               -- Next state
               v.state                          := HDR_S;
         ----------------------------------------------------------------------
         end case;
      end if;

      -- Synchronous Reset
      if (axilRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs        
      mAxisMaster <= r.mAxisMaster;

   end process comb;

   seq : process (axilClk) is
   begin
      if (rising_edge(axilClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

end mapping;
