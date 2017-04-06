-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmWibRxFramerReg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2017-01-19
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

entity ProtoDuneDpmWibRxFramerReg is
   generic (
      TPD_G            : time            := 1 ns;
      AXI_CLK_FREQ_G   : real            := 125.0E+6;  -- units of Hz
      AXI_ERROR_RESP_G : slv(1 downto 0) := AXI_RESP_DECERR_C);
   port (
      -- Status/Configuration Interface (clk domain)
      clk             : in  sl;
      rst             : in  sl;
      rxLinkUp        : in  sl;
      rxDecErr        : in  slv(1 downto 0);
      rxDispErr       : in  slv(1 downto 0);
      rxBufStatus     : in  slv(2 downto 0);
      cPllLock        : in  sl;
      rxPolarity      : out sl;
      txPolarity      : out sl;
      pktSent         : in  sl;
      backpressure    : in  sl;
      errPktDrop      : in  sl;
      errPktLen       : in  sl;
      errCrc          : in  sl;
      overflow        : in  sl;
      termFrame       : in  sl;
      pktLenStrb      : in  sl;
      pktLen          : in  slv(7 downto 0);
      wibSofDet       : in  sl;
      gtRst           : out sl;
      startLog        : out sl;
      oneShotLog      : out sl;
      dbgCrcRcv       : in  slv(31 downto 0);
      dbgCrcCalc      : in  slv(31 downto 0);
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType);
end ProtoDuneDpmWibRxFramerReg;

architecture rtl of ProtoDuneDpmWibRxFramerReg is

   constant STATUS_SIZE_C : positive := 14;

   type RegType is record
      startLog       : sl;
      oneShotLog     : sl;
      gtRst          : sl;
      minPktLen      : slv(7 downto 0);
      rxPolarity     : sl;
      txPolarity     : sl;
      cntRst         : sl;
      rollOverEn     : slv(STATUS_SIZE_C-1 downto 0);
      hardRst        : sl;
      axilReadSlave  : AxiLiteReadSlaveType;
      axilWriteSlave : AxiLiteWriteSlaveType;
   end record;

   constant REG_INIT_C : RegType := (
      startLog       => '0',
      oneShotLog     => '0',
      gtRst          => '0',
      minPktLen      => x"FF",
      rxPolarity     => '0',
      txPolarity     => '0',
      cntRst         => '1',
      rollOverEn     => toSlv(1, STATUS_SIZE_C),
      hardRst        => '0',
      axilReadSlave  => AXI_LITE_READ_SLAVE_INIT_C,
      axilWriteSlave => AXI_LITE_WRITE_SLAVE_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;


   signal statusOut       : slv(STATUS_SIZE_C-1 downto 0);
   signal statusCnt       : SlVectorArray(STATUS_SIZE_C-1 downto 0, 31 downto 0);
   signal rxBufStatusSync : slv(2 downto 0);
   signal pktLength       : slv(7 downto 0);
   signal packetRate      : slv(31 downto 0);
   signal wibSofRate      : slv(31 downto 0);
   signal gtRxFifoErr     : sl;

   -- attribute dont_touch      : string;
   -- attribute dont_touch of r : signal is "TRUE";

begin

   comb : process (axilReadMaster, axilRst, axilWriteMaster, dbgCrcCalc,
                   dbgCrcRcv, packetRate, pktLength, r, rxBufStatusSync,
                   statusCnt, statusOut, wibSofRate) is
      variable v      : RegType;
      variable regCon : AxiLiteEndPointType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      v.hardRst    := '0';
      v.cntRst     := '0';
      v.gtRst      := '0';
      v.startLog   := '0';
      v.oneShotLog := '0';

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
      axiSlaveRegisterR(regCon, x"404", 0, rxBufStatusSync);
      axiSlaveRegisterR(regCon, x"408", 0, r.minPktLen);
      axiSlaveRegisterR(regCon, x"40C", 0, pktLength);
      axiSlaveRegisterR(regCon, x"410", 0, packetRate);
      axiSlaveRegisterR(regCon, x"414", 0, wibSofRate);
      axiSlaveRegisterR(regCon, x"418", 0, dbgCrcRcv);
      axiSlaveRegisterR(regCon, x"41C", 0, dbgCrcCalc);

      -- Map the write registers
      axiSlaveRegister(regCon, x"700", 0, v.rxPolarity);
      axiSlaveRegister(regCon, x"704", 0, v.txPolarity);
      axiSlaveRegister(regCon, x"708", 0, v.gtRst);
      axiSlaveRegister(regCon, x"710", 0, v.startLog);
      axiSlaveRegister(regCon, x"714", 0, v.oneShotLog);
      axiSlaveRegister(regCon, x"7F0", 0, v.rollOverEn);
      axiSlaveRegister(regCon, x"7F4", 0, v.cntRst);
      axiSlaveRegister(regCon, x"7FC", 0, v.hardRst);

      -- Closeout the transaction
      axiSlaveDefault(regCon, v.axilWriteSlave, v.axilReadSlave, AXI_ERROR_RESP_G);

      -- Keep statistics on the packet length min. and current value
      if (r.cntRst = '1') then
         v.minPktLen := x"FF";
      else
         if (pktLength < r.minPktLen) then
            v.minPktLen := pktLength;
         end if;
      end if;

      -- Synchronous Reset
      if (axilRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      axilWriteSlave <= r.axilWriteSlave;
      axilReadSlave  <= r.axilReadSlave;

   end process comb;

   seq : process (axilClk) is
   begin
      if (rising_edge(axilClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   U_startLog : entity work.SynchronizerOneShot
      generic map (
         TPD_G => TPD_G)
      port map (
         clk     => clk,
         dataIn  => r.startLog,
         dataOut => startLog);

   U_oneShotLog : entity work.SynchronizerOneShot
      generic map (
         TPD_G => TPD_G)
      port map (
         clk     => clk,
         dataIn  => r.oneShotLog,
         dataOut => oneShotLog);

   U_gtRst : entity work.PwrUpRst
      generic map (
         TPD_G      => TPD_G,
         DURATION_G => 25000000)        -- 100 ms
      port map (
         arst   => r.gtRst,
         clk    => clk,
         rstOut => gtRst);

   U_packetRate : entity work.SyncTrigRate
      generic map (
         TPD_G          => TPD_G,
         COMMON_CLK_G   => false,
         REF_CLK_FREQ_G => AXI_CLK_FREQ_G,  -- units of Hz
         REFRESH_RATE_G => 1.0,             -- units of Hz
         CNT_WIDTH_G    => 32)              -- Counters' width
      port map (
         -- Trigger Input (locClk domain)
         trigIn      => pktLenStrb,
         -- Trigger Rate Output (locClk domain)
         trigRateOut => packetRate,
         -- Clocks
         locClk      => clk,
         refClk      => axilClk);

   U_wibSofRate : entity work.SyncTrigRate
      generic map (
         TPD_G          => TPD_G,
         COMMON_CLK_G   => false,
         REF_CLK_FREQ_G => AXI_CLK_FREQ_G,  -- units of Hz
         REFRESH_RATE_G => 1.0,             -- units of Hz
         CNT_WIDTH_G    => 32)              -- Counters' width
      port map (
         -- Trigger Input (locClk domain)
         trigIn      => wibSofDet,
         -- Trigger Rate Output (locClk domain)
         trigRateOut => wibSofRate,
         -- Clocks
         locClk      => clk,
         refClk      => axilClk);

   U_pktLen : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 8)
      port map (
         wr_clk => clk,
         wr_en  => pktLenStrb,
         din    => pktLen,
         rd_clk => axilClk,
         dout   => pktLength);

   U_rxBufStatus : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 3)
      port map (
         wr_clk => clk,
         din    => rxBufStatus,
         rd_clk => axilClk,
         dout   => rxBufStatusSync);

   U_SyncOutVec : entity work.SynchronizerVector
      generic map (
         TPD_G   => TPD_G,
         WIDTH_G => 2)
      port map (
         clk        => clk,
         dataIn(0)  => r.rxPolarity,
         dataIn(1)  => r.txPolarity,
         dataOut(0) => rxPolarity,
         dataOut(1) => txPolarity);

   gtRxFifoErr <= rxBufStatus(2) and rxLinkUp;

   U_SyncStatusVector : entity work.SyncStatusVector
      generic map (
         TPD_G          => TPD_G,
         OUT_POLARITY_G => '1',
         CNT_RST_EDGE_G => true,
         CNT_WIDTH_G    => 32,
         WIDTH_G        => STATUS_SIZE_C)
      port map (
         -- Input Status bit Signals (wrClk domain)
         statusIn(13)         => gtRxFifoErr,
         statusIn(12)         => termFrame,
         statusIn(11)         => overflow,
         statusIn(10)         => cPllLock,
         statusIn(9)          => errCrc,
         statusIn(8)          => errPktLen,
         statusIn(7)          => errPktDrop,
         statusIn(6)          => backpressure,
         statusIn(5)          => rxLinkUp,
         statusIn(4 downto 3) => rxDispErr,
         statusIn(2 downto 1) => rxDecErr,
         statusIn(0)          => pktSent,
         -- Output Status bit Signals (rdClk domain)  
         statusOut            => statusOut,
         -- Status Bit Counters Signals (rdClk domain) 
         cntRstIn             => r.cntRst,
         rollOverEnIn         => r.rollOverEn,
         cntOut               => statusCnt,
         -- Clocks and Reset Ports
         wrClk                => clk,
         rdClk                => axilClk);

end rtl;
