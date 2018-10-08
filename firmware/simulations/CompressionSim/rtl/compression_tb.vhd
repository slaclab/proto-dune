------------------------------------------------------------------------------
-- This file is part of 'SLAC Firmware Standard Library'.
-- It is subject to the license terms in the LICENSE.txt file found in the 
-- top-level directory of this distribution and at: 
--    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
-- No part of 'SLAC Firmware Standard Library', including this file, 
-- may be copied, modified, propagated, or distributed except according to 
-- the terms contained in the LICENSE.txt file.
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

use work.StdRtlPkg.all;
use work.AxiStreamPkg.all;
use work.AxiLitePkg.all;
use work.AxiPkg.all;
use work.AxiDmaPkg.all;
use work.RceG3Pkg.all;
use work.ProtoDuneDpmPkg.all;

entity compression_tb is end compression_tb;

-- Define architecture
architecture compression_tb of compression_tb is

   constant TPD_C         : time := 1 ns;

   signal emuClk          : sl;
   signal emuRst          : sl;

   signal axilClk         : sl;
   signal axilRst         : sl;
   signal axilReadMaster  : AxiLiteReadMasterType;
   signal axilReadSlave   : AxiLiteReadSlaveType;
   signal axilWriteMaster : AxiLiteWriteMasterType;
   signal axilWriteSlave  : AxiLiteWriteSlaveType;

   signal wibMasters      : AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
   signal wibSlaves       : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   signal loopbackMaster  : AxiStreamMasterType;
   signal loopbackSlave   : AxiStreamSlaveType;

   signal dmaIbMaster     : AxiStreamMasterType;
   signal dmaIbSlave      : AxiStreamSlaveType;

   signal enableTx     : sl;
   signal enableTrig   : sl;
   signal sendCnt      : sl;
   signal chNoiseCgf   : Slv4Array(127 downto 0);
   signal cmNoiseCgf   : slv(3 downto 0);
   signal chDlyCfg     : EmuDlyCfg;
   signal convt        : sl;
   signal adcDataF     : Slv12Array(127 downto 0);
   signal adcDataE     : Slv12Array(127 downto 0);
   signal adcData      : Slv12Array(127 downto 0);
   signal adcDataDly   : Slv12Array(127 downto 0);
   signal adcDataDlyCM : Slv12Array(127 downto 0);
   signal cmNoise      : slv(11 downto 0);

   signal emuData      : slv(15 downto 0);
   signal emuDataK     : slv(1 downto 0);

   signal rxEnable     : sl;

   signal curCnt       : slv(31 downto 0);
   signal nxtCnt       : slv(31 downto 0);

begin

   process begin
      axilClk <= '1';
      wait for 4 ns;
      axilClk <= '0';
      wait for 4 ns;
   end process;

   process begin
      axilRst <= '1';
      wait for (100 ns);
      axilRst <= '0';
      wait;
   end process;

   process begin
      emuClk <= '1';
      wait for 2.00 ns;
      emuClk <= '0';
      wait for 2.00 ns;
   end process;

   process begin
      emuRst <= '1';
      wait for (42 ns);
      emuRst <= '0';
      wait;
  end process;

   U_Hls: entity work.ProtoDuneDpmHls
      generic map (
         TPD_G           => TPD_C,
         CASCADE_SIZE_G  => 1,
         AXI_CLK_FREQ_G  => 125.0E+6,
         AXI_BASE_ADDR_G => x"A0000000")
      port map (
         axilClk          => axilClk,
         axilRst          => axilRst,
         axilReadMaster   => axilReadMaster,
         axilReadSlave    => axilReadSlave,
         axilWriteMaster  => axilWriteMaster,
         axilWriteSlave   => axilWriteSlave,
         -- WIB Interface (axilClk domain)
         wibMasters       => wibMasters,
         wibSlaves        => wibSlaves,
         -- DMA Loopback Path Interface (dmaClk domain)
         loopbackMaster   => loopbackMaster,
         loopbackSlave    => loopbackSlave,
         -- AXI Stream Interface (dmaClk domain)
         dmaClk           => emuClk,
         dmaRst           => emuRst,
         dmaIbMaster      => dmaIbMaster,
         dmaIbSlave       => dmaIbSlave);

   loopbackMaster <= AXI_STREAM_MASTER_INIT_C;
   dmaIbSlave     <= AXI_STREAM_SLAVE_FORCE_C;

   process(axilClk)
   begin
      if rising_edge(axilClk) then
         if axilRst = '1' then
            curCnt <= (others=>'0');
            nxtCnt <= (others=>'0');
         elsif dmaIbMaster.tValid = '1' then
            nxtCnt <= nxtCnt + 1;

            if dmaIbMaster.tLast = '1' then
               curCnt <= nxtCnt + 1;
               nxtCnt <= (others=>'0');
            end if;
         end if;
      end if;
   end process;

   process begin

      rxEnable        <= '0';
      enableTx        <= '0';
      enableTrig      <= '0';
      chNoiseCgf      <= (others=>(others=>'0'));
      cmNoiseCgf      <= "1011";
      chDlyCfg        <= (others => (others => '0'));
      axilWriteMaster <= AXI_LITE_WRITE_MASTER_INIT_C;
      axilReadMaster  <= AXI_LITE_READ_MASTER_INIT_C;
      sendCnt         <= '0';

      wait for 5 US;

      axiLiteBusSimWrite ( axilClk, axilWriteMaster, axilWriteSlave, x"A0000000", x"00000081", true); -- Enable compression
      axiLiteBusSimWrite ( axilClk, axilWriteMaster, axilWriteSlave, x"A0010000", x"00000081", true); -- Enable compression
      --axiLiteBusSimWrite ( axilClk, axilWriteMaster, axilWriteSlave, x"A0020800", x"00000000", true);

      wait for 1 US;
      enableTx <= '1';
      wait for 5 US;
      rxEnable <= '0';
      wait for 0.6 MS;

      axiLiteBusSimWrite ( axilClk, axilWriteMaster, axilWriteSlave, x"A0020800", x"00000000", true);

      wait;
   end process;

   ------------------------------------------
   -- Emulator
   ------------------------------------------

   U_DataEmu : entity work.ProtoDuneDpmEmuData
      generic map ( TPD_G => TPD_C)
      port map (
         -- Clock and Reset
         clk        => emuClk,
         rst        => emuRst,
         -- EMU Data Interface
         enableTx   => enableTx,
         enableTrig => enableTrig,
         convt      => convt,
         cmNoiseCgf => cmNoiseCgf,
         chNoiseCgf => chNoiseCgf,
         timingTrig => '0',
         cmNoise    => cmNoise,
         adcData    => adcDataE);

   U_DataFile : entity work.ProtoDuneFileData
      generic map ( TPD_G => TPD_C)
      port map (
         -- Clock and Reset
         clk        => emuClk,
         rst        => emuRst,
         -- EMU Data Interface
         enableTx   => enableTx,
         convt      => convt,
         cmNoise    => cmNoise,
         adcData    => adcDataF);

   adcData <= adcDataF;

   ----------------
   -- Delay Modules
   ----------------
   GEN_VEC :
   for i in 127 downto 0 generate

      U_DlyTaps : entity work.SlvDelay
         generic map (
            TPD_G        => TPD_C,
            SRL_EN_G     => true,
            DELAY_G      => EMU_DELAY_TAPS_C,
            REG_OUTPUT_G => true,
            WIDTH_G      => 12)
         port map (
            clk   => emuClk,
            en    => convt,
            delay => chDlyCfg(i),
            din   => adcData(i),
            dout  => adcDataDly(i));

      process(emuClk)
      begin
         if rising_edge(emuClk) then
            if emuRst = '1' then
               adcDataDlyCM(i) <= (others=>'0');
            elsif convt = '1' then
               adcDataDlyCM(i) <= adcDataDly(i) + cmNoise after TPD_C;
            end if;
         end if;
      end process;

   end generate GEN_VEC;

   ---------------------
   -- TX Frame Formatter
   ---------------------
   U_TxFrammer : entity work.ProtoDuneDpmEmuTxFramer
      generic map (
         TPD_G => TPD_C)
      port map (
         -- Clock and Reset
         clk      => emuClk,
         rst      => emuRst,
         -- EMU Data Interface
         enable   => enableTx,
         sendCnt  => sendCnt,
         adcData  => adcDataDlyCM,
         convt    => convt,
         timingTs => (others=>'0'),
         -- TX Data Interface
         txData   => emuData,
         txdataK  => emuDataK);

   U_RxGen: for i in 0 to 1 generate
      U_RxFramer: entity work.ProtoDuneDpmWibRxFramer
         generic map ( TPD_G => TPD_C )
         port map (
            -- AXI-Lite Interface (axilClk domain)
            axilClk         => axilClk,
            axilRst         => axilRst,
            axilReadMaster  => AXI_LITE_READ_MASTER_INIT_C,
            axilReadSlave   => open,
            axilWriteMaster => AXI_LITE_WRITE_MASTER_INIT_C,
            axilWriteSlave  => open,
            -- RX Data Interface (clk domain)
            clk             => emuClk,
            rst             => emuRst,
            rxValid         => '1',
            rxData          => emuData,
            rxdataK         => emuDataK,
            rxDecErr        => (others=>'0'),
            rxDispErr       => (others=>'0'),
            rxBufStatus     => (others=>'0'), 
            rxPolarity      => open,
            txPolarity      => open,
            cPllLock        => '1',
            gtRst           => open,
            logEn           => open,
            logClr          => open,
            -- Timing Interface (clk domain)
            swFlush         => '0',
            runEnable       => rxEnable,
            -- WIB Interface (axilClk domain)
            wibMaster       => wibMasters(i),
            wibSlave        => wibSlaves(i));
   end generate;

end compression_tb;

