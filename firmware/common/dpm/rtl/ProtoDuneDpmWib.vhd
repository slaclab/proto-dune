-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmWib.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
-- Last update: 2018-08-10
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
use work.AxiStreamPkg.all;
use work.ProtoDuneDpmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDpmWib is
   generic (
      TPD_G           : time             := 1 ns;
      CASCADE_SIZE_G  : positive         := 1;
      AXI_CLK_FREQ_G  : real             := 125.0E+6;  -- units of Hz
      AXI_BASE_ADDR_G : slv(31 downto 0) := x"A0000000");
   port (
      -- Stable clock and reset reference
      clk             : out sl;
      rst             : out sl;
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- RTM Interface
      ref250ClkP      : in  sl;
      ref250ClkN      : in  sl;
      dpmToRtmHsP     : out slv(2 downto 0);
      dpmToRtmHsN     : out slv(2 downto 0);
      rtmToDpmHsP     : in  slv(2 downto 0);
      rtmToDpmHsN     : in  slv(2 downto 0);
      -- Timing Interface (clk domain)
      swFlush         : in  sl;
      runEnable       : in  sl;
      -- TX EMU Interface (emuClk domain)
      emuClk          : in  sl;
      emuRst          : in  sl;
      emuLoopback     : in  sl;
      emuData         : in  Slv16Array(1 downto 0);
      emuDataK        : in  Slv2Array(1 downto 0);
      txPreCursor     : in  slv(4 downto 0) := (others => '0');
      txPostCursor    : in  slv(4 downto 0) := (others => '0');
      txDiffCtrl      : in  slv(3 downto 0) := "1111";
      -- WIB Interface (axilClk domain)
      wibMasters      : out AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      wibSlaves       : in  AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0));
end ProtoDuneDpmWib;

architecture mapping of ProtoDuneDpmWib is

   signal axilWriteMasters : AxiLiteWriteMasterArray((2*WIB_SIZE_C)-1 downto 0);
   signal axilWriteSlaves  : AxiLiteWriteSlaveArray((2*WIB_SIZE_C)-1 downto 0);
   signal axilReadMasters  : AxiLiteReadMasterArray((2*WIB_SIZE_C)-1 downto 0);
   signal axilReadSlaves   : AxiLiteReadSlaveArray((2*WIB_SIZE_C)-1 downto 0);

   signal rxValid     : slv(WIB_SIZE_C-1 downto 0);
   signal rxData      : Slv16Array(WIB_SIZE_C-1 downto 0);
   signal rxDataK     : Slv2Array(WIB_SIZE_C-1 downto 0);
   signal rxDecErr    : Slv2Array(WIB_SIZE_C-1 downto 0);
   signal rxDispErr   : Slv2Array(WIB_SIZE_C-1 downto 0);
   signal rxBufStatus : Slv3Array(WIB_SIZE_C-1 downto 0);
   signal rxPolarity  : slv(WIB_SIZE_C-1 downto 0);
   signal txPolarity  : slv(WIB_SIZE_C-1 downto 0);
   signal cPllLock    : slv(WIB_SIZE_C-1 downto 0);
   signal gtRst       : slv(WIB_SIZE_C-1 downto 0);
   signal waveform    : Slv32Array(WIB_SIZE_C-1 downto 0);
   signal logEn       : slv(WIB_SIZE_C-1 downto 0);
   signal logClr      : slv(WIB_SIZE_C-1 downto 0);

   signal rxMasters : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal rxSlaves  : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal refClk     : sl;
   signal refClkDiv2 : sl;
   signal mmcmClk    : sl;
   signal mmcmRst    : sl;
   signal clock      : sl;
   signal reset      : sl;

begin

   clk <= clock;
   rst <= reset;

   ------------------
   -- Clock and Reset
   ------------------         
   U_IBUFDS_GTE2 : IBUFDS_GTE2
      port map (
         I     => ref250ClkP,
         IB    => ref250ClkN,
         CEB   => '0',
         ODIV2 => refClkDiv2,
         O     => refClk);

   U_BUFG : BUFG
      port map (
         I => refClkDiv2,
         O => mmcmClk);

   U_PwrUpRst : entity work.PwrUpRst
      generic map(
         TPD_G => TPD_G)
      port map (
         clk    => mmcmClk,
         rstOut => mmcmRst);

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
         CLKIN_PERIOD_G     => 8.0,
         DIVCLK_DIVIDE_G    => 1,
         CLKFBOUT_MULT_F_G  => 8.0,
         CLKOUT0_DIVIDE_F_G => 4.0)
      port map(
         clkIn     => mmcmClk,
         rstIn     => mmcmRst,
         clkOut(0) => clock,
         rstOut(0) => reset);

   --------------------
   -- AXI-Lite Crossbar
   --------------------
   U_XBAR : entity work.AxiLiteCrossbar
      generic map (
         TPD_G              => TPD_G,
         NUM_SLAVE_SLOTS_G  => 1,
         NUM_MASTER_SLOTS_G => (2*WIB_SIZE_C),
         MASTERS_CONFIG_G   => genAxiLiteConfig((2*WIB_SIZE_C), AXI_BASE_ADDR_G, 24, 16))
      port map (
         axiClk              => axilClk,
         axiClkRst           => axilRst,
         sAxiWriteMasters(0) => axilWriteMaster,
         sAxiWriteSlaves(0)  => axilWriteSlave,
         sAxiReadMasters(0)  => axilReadMaster,
         sAxiReadSlaves(0)   => axilReadSlave,
         mAxiWriteMasters    => axilWriteMasters,
         mAxiWriteSlaves     => axilWriteSlaves,
         mAxiReadMasters     => axilReadMasters,
         mAxiReadSlaves      => axilReadSlaves);

   WIB_LINK :
   for i in WIB_SIZE_C-1 downto 0 generate

      -------------
      -- GTX Module
      -------------  
      U_GTX : entity work.ProtoDuneDpmGtx7
         generic map (
            TPD_G     => TPD_G,
            PWR_DWN_G => false)
         port map (
            -- Clock and Reset
            refClk       => refClk,
            clk          => clock,
            rst          => reset,
            gtRst        => gtRst(i),
            -- Debug Interface   
            cPllLock     => cPllLock(i),
            rxPolarity   => rxPolarity(i),
            rxBufStatus  => rxBufStatus(i),
            txPolarity   => txPolarity(i),
            txPreCursor  => txPreCursor,
            txPostCursor => txPostCursor,
            txDiffCtrl   => txDiffCtrl,
            -- RTM Interface
            dpmToRtmHsP  => dpmToRtmHsP(i),
            dpmToRtmHsN  => dpmToRtmHsN(i),
            rtmToDpmHsP  => rtmToDpmHsP(i),
            rtmToDpmHsN  => rtmToDpmHsN(i),
            -- TX EMU Interface
            emuClk       => emuClk,
            emuRst       => emuRst,
            emuLoopback  => emuLoopback,
            emuData      => emuData(i),
            emuDataK     => emuDataK(i),
            -- RX Data Interface (clk domain)
            rxValid      => rxValid(i),
            rxData       => rxData(i),
            rxdataK      => rxdataK(i),
            rxDecErr     => rxDecErr(i),
            rxDispErr    => rxDispErr(i));

      ------------------------  
      -- Frame Receiver Module
      ------------------------  
      U_RX : entity work.ProtoDuneDpmWibRxFramer
         generic map (
            TPD_G          => TPD_G,
            AXI_CLK_FREQ_G => AXI_CLK_FREQ_G,
            CASCADE_SIZE_G => CASCADE_SIZE_G)
         port map (
            -- AXI-Lite Port (axilClk domain)
            axilClk         => axilClk,
            axilRst         => axilRst,
            axilReadMaster  => axilReadMasters(i),
            axilReadSlave   => axilReadSlaves(i),
            axilWriteMaster => axilWriteMasters(i),
            axilWriteSlave  => axilWriteSlaves(i),
            -- RX Data Interface (clk domain)
            clk             => clock,
            rst             => reset,
            rxValid         => rxValid(i),
            rxData          => rxData(i),
            rxdataK         => rxdataK(i),
            rxDecErr        => rxDecErr(i),
            rxDispErr       => rxDispErr(i),
            rxBufStatus     => rxBufStatus(i),
            rxPolarity      => rxPolarity(i),
            txPolarity      => txPolarity(i),
            cPllLock        => cPllLock(i),
            gtRst           => gtRst(i),
            logEn           => logEn(i),
            logClr          => logClr(i),
            -- Timing Interface (clk domain)
            swFlush         => swFlush,
            runEnable       => runEnable,
            -- WIB Interface (axilClk domain)
            wibMaster       => wibMasters(i),
            wibSlave        => wibSlaves(i));

      -------------------------------------------
      -- Error WIB Packet Waveform Capture Module
      -------------------------------------------
      Waveform_Capture : entity work.AxiLiteRingBuffer
         generic map (
            TPD_G            => TPD_G,
            BRAM_EN_G        => true,
            REG_EN_G         => true,
            DATA_WIDTH_G     => 32,
            RAM_ADDR_WIDTH_G => 8)
         port map (
            -- Data to store in ring buffer
            dataClk         => clock,
            dataRst         => reset,
            dataValid       => '1',
            dataValue       => waveform(i),
            bufferEnable    => logEn(i),
            bufferClear     => logClr(i),
            -- AXI-Lite interface for readout
            axilClk         => axilClk,
            axilRst         => axilRst,
            axilReadMaster  => axilReadMasters(i+WIB_SIZE_C),
            axilReadSlave   => axilReadSlaves(i+WIB_SIZE_C),
            axilWriteMaster => axilWriteMasters(i+WIB_SIZE_C),
            axilWriteSlave  => axilWriteSlaves(i+WIB_SIZE_C));

      process(clock)
      begin
         if rising_edge(clock) then
            -- Check for Data
            if (rxDataK(i) = "00") and (waveform(i)(31 downto 24) /= x"FF") and (waveform(i)(31 downto 24) /= x"EE") then
               waveform(i)(31 downto 24) <= waveform(i)(31 downto 24) + 1;
            -- Check for IDLE
            elsif (rxDataK(i) = "11") and (rxData(i)(15 downto 8) = K28_2_C) and (rxData(i)(7 downto 0) = K28_1_C) then
               waveform(i)(31 downto 24) <= x"FF";
            -- Check for SOF
            elsif (rxdataK(i) = "01") and (rxData(i)(7 downto 0) = K28_5_C) then
               waveform(i)(31 downto 24) <= x"01";
            -- Else it's an undefined word
            else
               waveform(i)(31 downto 24) <= x"EE";
            end if;
            waveform(i)(23 downto 22) <= '0' & not(rxValid(i));
            waveform(i)(21 downto 20) <= rxDispErr(i);
            waveform(i)(19 downto 18) <= rxDecErr(i);
            waveform(i)(17 downto 16) <= rxDataK(i);
            waveform(i)(15 downto 0)  <= rxData(i);
         end if;
      end process;

   end generate WIB_LINK;

   ---------------------  
   -- Unused GTX Channel
   ---------------------  
   U_UnusedGTX : entity work.ProtoDuneDpmGtx7
      generic map (
         TPD_G     => TPD_G,
         PWR_DWN_G => true)
      port map (
         -- Clock and Reset
         refClk      => refClk,
         clk         => clock,
         rst         => reset,
         -- RTM Interface
         dpmToRtmHsP => dpmToRtmHsP(2),
         dpmToRtmHsN => dpmToRtmHsN(2),
         rtmToDpmHsP => rtmToDpmHsP(2),
         rtmToDpmHsN => rtmToDpmHsN(2),
         -- TX EMU Interface
         emuClk      => emuClk,
         emuRst      => emuRst,
         emuLoopback => emuLoopback,
         emuData     => emuData,
         emuDataK    => emuDataK);

end mapping;
