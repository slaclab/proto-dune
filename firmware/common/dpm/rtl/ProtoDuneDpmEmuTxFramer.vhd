-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmEmuTxFramer.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2017-03-16
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

entity ProtoDuneDpmEmuTxFramer is
   generic (
      TPD_G : time := 1 ns);
   port (
      -- Clock and Reset
      clk     : in  sl;
      rst     : in  sl;
      -- EMU Data Interface
      enable  : in  sl;
      sendCnt : in  sl;
      adcData : in  Slv12Array(127 downto 0);
      convt   : out sl;
      -- TX Data Interface
      txData  : out slv(15 downto 0);
      txdataK : out slv(1 downto 0));
end ProtoDuneDpmEmuTxFramer;

architecture rtl of ProtoDuneDpmEmuTxFramer is

   constant DLY_C : positive := 3;

   type RegType is record
      convt      : sl;
      txData     : Slv16Array(DLY_C downto 0);
      txdataK    : Slv2Array(DLY_C downto 0);
      timestamp  : slv(63 downto 0);
      convertCnt : slv(15 downto 0);
      crcValid   : sl;
      crcRst     : sl;
      crcData    : slv(15 downto 0);
      cnt        : natural range 0 to 125;
      idx        : natural range 0 to 47;
   end record RegType;
   constant REG_INIT_C : RegType := (
      convt      => '0',
      txData     => (others => (K28_2_C & K28_1_C)),
      txdataK    => (others => "11"),
      timestamp  => (others => '0'),
      convertCnt => (others => '0'),
      crcValid   => '0',
      crcRst     => '1',
      crcData    => (others => '0'),
      cnt        => 1,
      idx        => 0);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal crcResult : slv(31 downto 0);

   attribute dont_touch              : string;
   attribute dont_touch of r         : signal is "TRUE";
   attribute dont_touch of crcResult : signal is "TRUE";

begin

   comb : process (adcData, crcResult, enable, r, rst, sendCnt) is
      variable v         : RegType;
      variable i         : natural;
      variable coldData1 : slv((64*12)-1 downto 0);
      variable coldData2 : slv((64*12)-1 downto 0);
   begin
      -- Latch the current value
      v := r;

      -- Map the ADC data to coldData format
      for i in 63 downto 0 loop
         coldData1((i*12)+11 downto (i*12)) := adcData(i+0);
         coldData2((i*12)+11 downto (i*12)) := adcData(i+64);
      end loop;

      -- Reset strobing signals
      v.convt := '0';

      -- Check the enable or in middle of sending frame
      if (enable = '1') or (r.cnt /= 1) then
         -- Increment the counter
         if r.cnt = 125 then
            v.cnt := 1;
         else
            v.cnt := r.cnt + 1;
         end if;
      else
         -- Pre-fill the SlvDelay modules
         v.convt := '1';
      end if;

      -- Shift Register
      v.txdataK(DLY_C downto 1) := r.txdataK(DLY_C-1 downto 0);
      v.txData(DLY_C downto 1)  := r.txData(DLY_C-1 downto 0);

      -- State Machine
      case r.cnt is
         ----------------------------------------------------------------------
         when 1 =>
            -- Check the enable
            if enable = '1' then
               -- Send Start of Frame pattern
               v.txdataK(0)             := "01";
               v.txData(0)(7 downto 0)  := K28_5_C;
               v.txData(0)(15 downto 8) := x"01";
               -- Start the CRC engine
               v.crcValid               := '1';
               v.crcRst                 := '0';
            else
               -- Reset the time stamp
               v.timestamp  := (others => '0');
               v.convertCnt := (others => '0');
               -- Send IDLE pattern
               v.txdataK(0) := "11";
               v.txData(0)  := (K28_2_C & K28_1_C);
               -- Reset the CRC module
               v.crcValid   := '0';
               v.crcRst     := '1';
            end if;
         ----------------------------------------------------------------------
         when 5 =>
            -- Send the WIB timestamp       
            v.txData(0) := r.timestamp(15 downto 0);
         ----------------------------------------------------------------------
         when 6 =>
            -- Send the WIB timestamp       
            v.txData(0) := r.timestamp(31 downto 16);
         ----------------------------------------------------------------------
         when 5 =>
            -- Send the WIB timestamp       
            v.txData(0) := r.timestamp(47 downto 32);
         ----------------------------------------------------------------------
         when 6 =>
            -- Send the WIB timestamp       
            v.txData(0) := sendCnt & r.timestamp(62 downto 48);
         ----------------------------------------------------------------------       
         when 12 =>
            -- Send the convert counter        
            v.txData(0) := r.convertCnt;
         ----------------------------------------------------------------------
         when 17 to 64 =>
            -- Send the COLDDATA 1
            v.txData(0) := coldData1((r.idx*16)+15 downto (r.idx*16));
            -- Increment counter
            if r.idx = 47 then
               -- Reset the counter
               v.idx := 0;
            else
               v.idx := r.idx + 1;
            end if;
         ----------------------------------------------------------------------       
         when 68 =>
            -- Send the convert counter        
            v.txData(0) := r.convertCnt;
         ----------------------------------------------------------------------
         when 73 to 120 =>
            -- Send the COLDDATA 2
            v.txData(0) := coldData2((r.idx*16)+15 downto (r.idx*16));
            -- Increment counter
            if r.idx = 47 then
               -- Reset the counter
               v.idx := 0;
            else
               v.idx := r.idx + 1;
            end if;
         ----------------------------------------------------------------------
         when 121 =>                    -- End of datagram @ v.txData(1)
            -- Send IDLE pattern
            v.txdataK(0) := "11";
            v.txData(0)  := (K28_2_C & K28_1_C);
            -- Stop sending data to CRC engine
            v.crcValid   := '0';
            -- Set the flag
            v.convt      := '1';
         ----------------------------------------------------------------------
         when 122 =>                    -- End of datagram @ v.txData(2)
            -- Send IDLE pattern
            v.txdataK(0) := "11";
            v.txData(0)  := (K28_2_C & K28_1_C);
         ----------------------------------------------------------------------
         when 123 =>                    -- End of datagram @ v.txData(DLY_C=3)
            -- Send IDLE pattern
            v.txdataK(0) := "11";
            v.txData(0)  := (K28_2_C & K28_1_C);
         ----------------------------------------------------------------------
         when 124 =>
            -- Send CRC (overwrite last element of shift register)
            v.txdataK(DLY_C) := "00";
            v.txData(DLY_C)  := crcResult(15 downto 0);
         ----------------------------------------------------------------------
         when 125 =>
            -- Send CRC (overwrite last element of shift register)
            v.txdataK(DLY_C) := "00";
            v.txData(DLY_C)  := crcResult(31 downto 16);
            -- Reset the CRC module
            v.crcRst         := '1';
            -- Increment the counters
            v.febTs          := r.febTs + 500;  -- timestamp's LSB is 1ns 
            v.convertCnt     := r.convertCnt + 1;
         ----------------------------------------------------------------------
         when others =>
            -- Send ZEROS pattern
            v.txdataK(0) := "00";
            v.txData(0)  := x"0000";
      ----------------------------------------------------------------------
      end case;

      -- Check for sending debug counter
      if (sendCnt = '1') and (enable = '1') then
         -- Check for first word
         if (r.cnt = 1) then
            -- Mask off the first data byte
            v.txData(0)(15 downto 8) := x"00";
         -- Check for words before the CRC insertation   
         elsif (r.cnt < 121) then
            v.txData(0) := toSlv((r.cnt-1), 16);
         end if;
      end if;

      -- Move the TX data to CRC
      v.crcData := v.txdata(0);

      -- Reset
      if (rst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      convt   <= r.convt;
      txdata  <= r.txdata(DLY_C);
      txdataK <= r.txdataK(DLY_C);

   end process comb;

   seq : process (clk) is
   begin
      if rising_edge(clk) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   -- --------------------
   -- -- SLAC's CRC Engine
   -- --------------------
   -- U_Crc32 : entity work.Crc32Parallel
   -- generic map (
   -- BYTE_WIDTH_G => 2)
   -- port map (
   -- crcClk       => clk,
   -- crcReset     => r.crcRst,
   -- crcDataWidth => "001",-- 2 bytes 
   -- crcDataValid => r.crcValid,
   -- crcIn        => r.crcData,
   -- crcOut       => crcResult);

   -------------------
   -- WIB's CRC Engine
   -------------------         
   U_Crc32 : entity work.EthernetCRCD16B
      port map (
         clk  => clk,
         init => r.crcRst,
         ce   => r.crcValid,
         d    => r.crcData,
         crc  => crcResult);

end rtl;
