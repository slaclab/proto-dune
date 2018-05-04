-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmTimingReg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-09-26
-- Last update: 2018-03-20
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
use work.ProtoDuneDpmPkg.all;

use work.pdts_defs.all;

entity ProtoDuneDpmTimingReg is
   generic (
      TPD_G            : time            := 1 ns);
   port (
      -- WIB Interface (wibClk domain)
      wibClk           : in  sl;
      wibRst           : in  sl;
      runEnable        : in  sl;
      swFlush          : out sl;
      -- Timing RX Interface (cdrClk domain)
      cdrClk           : in  sl;
      cdrRst           : in  sl;
      cdrEdgeSel       : out sl;
      cdrDataInv       : out sl;
      cdrLocked        : in  sl;
      freqMeasured     : in  slv(31 downto 0);
      timingBus        : in  ProtoDuneDpmTimingType;
      timingMsgDrop    : in  sl;
      timingRunEnable  : in  sl;
      triggerDet       : in  sl;
      pdtsEndpointAddr : out slv(7 downto 0);
      pdtsEndpointTgrp : out slv(1 downto 0);
      -- AXI-Lite Interface (axilClk domain)
      axilClk          : in  sl;
      axilRst          : in  sl;
      axilReadMaster   : in  AxiLiteReadMasterType;
      axilReadSlave    : out AxiLiteReadSlaveType;
      axilWriteMaster  : in  AxiLiteWriteMasterType;
      axilWriteSlave   : out AxiLiteWriteSlaveType;
      softRst          : out sl);
end ProtoDuneDpmTimingReg;

architecture rtl of ProtoDuneDpmTimingReg is

   constant STATUS_SIZE_C : positive := 6;

   type RegType is record
      pdtsEndpointAddr : slv(7 downto 0);
      pdtsEndpointTgrp : slv(1 downto 0);
      cdrEdgeSel       : sl;
      cdrDataInv       : sl;
      swFlush          : sl;
      rollOverEn       : slv(STATUS_SIZE_C-1 downto 0);
      cntRst           : sl;
      softRst          : sl;
      hardRst          : sl;
      axilReadSlave    : AxiLiteReadSlaveType;
      axilWriteSlave   : AxiLiteWriteSlaveType;
   end record;

   constant REG_INIT_C : RegType := (
      pdtsEndpointAddr => (others => '0'),
      pdtsEndpointTgrp => (others => '0'),
      cdrEdgeSel       => '0',
      cdrDataInv       => '0',
      swFlush          => '1',
      rollOverEn       => (others => '0'),
      cntRst           => '1',
      softRst          => '1',
      hardRst          => '0',          -- must be zero to prevent lock up
      axilReadSlave    => AXI_LITE_READ_SLAVE_INIT_C,
      axilWriteSlave   => AXI_LITE_WRITE_SLAVE_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal statusOut : slv(STATUS_SIZE_C-1 downto 0);
   signal statusCnt : SlVectorArray(STATUS_SIZE_C-1 downto 0, 31 downto 0);

   signal timingStat   : slv(3 downto 0);
   signal eventCnt     : slv(31 downto 0);
   signal trigRate     : slv(31 downto 0);
   signal trigRateSync : slv(31 downto 0);

   -- attribute dont_touch               : string;
   -- attribute dont_touch of r          : signal is "TRUE";

begin

   comb : process (axilReadMaster, axilRst, axilWriteMaster, eventCnt,
                   freqMeasured, r, statusCnt, statusOut, timingStat,
                   trigRateSync) is
      variable v      : RegType;
      variable regCon : AxiLiteEndPointType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      v.hardRst := '0';
      v.softRst := '0';
      v.cntRst  := '0';

      -- Check for hard reset
      if (r.hardRst = '1') then
         -- Reset the register
         v := REG_INIT_C;
      end if;

      -- Determine the transaction type
      axiSlaveWaitTxn(regCon, axilWriteMaster, axilReadMaster, v.axilWriteSlave, v.axilReadSlave);

      -- Map the read registers
      for i in STATUS_SIZE_C-1 downto 0 loop
         axiSlaveRegisterR(regCon, toSlv((4*i), 12), 0, muxSlVectorArray(statusCnt, i));
      end loop;
      axiSlaveRegisterR(regCon, x"400", 0, statusOut);
      axiSlaveRegisterR(regCon, x"404", 0, freqMeasured);
      axiSlaveRegisterR(regCon, x"408", 0, timingStat);
      axiSlaveRegisterR(regCon, x"40C", 0, eventCnt);
      axiSlaveRegisterR(regCon, x"410", 0, trigRateSync);

      -- Map the write registers
      axiSlaveRegister(regCon, x"800", 0, v.swFlush);
      axiSlaveRegister(regCon, x"804", 0, v.cdrEdgeSel);
      axiSlaveRegister(regCon, x"808", 0, v.cdrDataInv);
      axiSlaveRegister(regCon, x"80C", 0, v.pdtsEndpointAddr);
      axiSlaveRegister(regCon, x"810", 0, v.pdtsEndpointTgrp);

      axiSlaveRegister(regCon, x"FF0", 0, v.rollOverEn);
      axiSlaveRegister(regCon, x"FF4", 0, v.cntRst);
      axiSlaveRegister(regCon, x"FF8", 0, v.softRst);
      axiSlaveRegister(regCon, x"FFC", 0, v.hardRst);

      -- Closeout the transaction
      axiSlaveDefault(regCon, v.axilWriteSlave, v.axilReadSlave, AXI_RESP_DECERR_C);

      -- Synchronous Reset
      if (axilRst = '1') then
         -- Reset the register
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      axilWriteSlave   <= r.axilWriteSlave;
      axilReadSlave    <= r.axilReadSlave;
      cdrEdgeSel       <= r.cdrEdgeSel;
      cdrDataInv       <= r.cdrDataInv;
      pdtsEndpointAddr <= r.pdtsEndpointAddr;
      pdtsEndpointTgrp <= r.pdtsEndpointTgrp;

   end process comb;

   seq : process (axilClk) is
   begin
      if (rising_edge(axilClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   U_softRst : entity work.PwrUpRst
      generic map (
         TPD_G      => TPD_G,
         DURATION_G => 125000000)
      port map (
         arst   => r.softRst,
         clk    => axilClk,
         rstOut => softRst);

   U_SyncOutVec : entity work.SynchronizerVector
      generic map (
         TPD_G   => TPD_G,
         WIDTH_G => 1)
      port map (
         clk        => wibClk,
         dataIn(0)  => r.swFlush,
         dataOut(0) => swFlush);

   U_Stat : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 4)
      port map (
         rst    => cdrRst,
         -- Write Ports (wr_clk domain)
         wr_clk => cdrClk,
         din    => timingBus.stat,
         -- Read Ports (rd_clk domain)
         rd_clk => axilClk,
         dout   => timingStat);

   U_EventCnt : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 32)
      port map (
         rst    => cdrRst,
         -- Write Ports (wr_clk domain)
         wr_clk => cdrClk,
         din    => timingBus.eventCnt,
         -- Read Ports (rd_clk domain)
         rd_clk => axilClk,
         dout   => eventCnt);

   U_TrigRate : entity work.SyncTrigRate
      generic map (
         TPD_G          => TPD_G,
         REF_CLK_FREQ_G => 125.0E+6,
         CNT_WIDTH_G    => 32)
      port map (
         -- Trigger Input (locClk domain)
         trigIn      => triggerDet,
         -- Trigger Rate Output (locClk domain)
         trigRateOut => trigRate,
         -- Clocks
         locClk      => cdrClk,
         locRst      => cdrRst,
         refClk      => axilClk);

   U_TrigRateSync : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 32)
      port map (
         rst    => cdrRst,
         -- Write Ports (wr_clk domain)
         wr_clk => cdrClk,
         din    => trigRate,
         -- Read Ports (rd_clk domain)
         rd_clk => axilClk,
         dout   => trigRateSync);

   U_SyncStatusVector : entity work.SyncStatusVector
      generic map (
         TPD_G          => TPD_G,
         OUT_POLARITY_G => '1',
         CNT_RST_EDGE_G => true,
         CNT_WIDTH_G    => 32,
         WIDTH_G        => STATUS_SIZE_C)
      port map (
         -- Input Status bit Signals (wrClk domain)
         statusIn(5)  => cdrRst,
         statusIn(4)  => triggerDet,
         statusIn(3)  => timingMsgDrop,
         statusIn(2)  => cdrLocked,
         statusIn(1)  => timingBus.rdy,
         statusIn(0)  => timingRunEnable,
         -- Output Status bit Signals (rdClk domain)  
         statusOut    => statusOut,
         -- Status Bit Counters Signals (rdClk domain) 
         cntRstIn     => r.cntRst,
         rollOverEnIn => r.rollOverEn,
         cntOut       => statusCnt,
         -- Clocks and Reset Ports
         wrClk        => cdrClk,
         rdClk        => axilClk);

end rtl;
