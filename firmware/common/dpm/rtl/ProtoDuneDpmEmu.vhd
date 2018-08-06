-------------------------------------------------------------------------------
-- File       : DuneDpmReg.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-04
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
use work.AxiLitePkg.all;
use work.ProtoDuneDpmPkg.all;

entity ProtoDuneDpmEmu is
   generic (
      TPD_G            : time             := 1 ns;
      SIM_START_G      : sl               := '0';
      DEFAULT_CNT_G    : sl               := '0';
      AXI_BASE_ADDR_G  : slv(31 downto 0) := x"A0000000");
   port (
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- TX EMU Interface (clk domain)
      clk             : in  sl;
      rst             : in  sl;
      timingTrig      : in  sl;
      timingTs        : in  slv(63 downto 0);
      emuLoopback     : out sl;
      emuConvt        : out sl;
      emuEnable       : out sl;
      emuData         : out slv(15 downto 0);
      emuDataK        : out slv(1 downto 0);
      txPreCursor     : out slv(4 downto 0);
      txPostCursor    : out slv(4 downto 0);
      txDiffCtrl      : out slv(3 downto 0));
end ProtoDuneDpmEmu;

architecture mapping of ProtoDuneDpmEmu is

   signal enableTx     : sl;
   signal enableTrig   : sl;
   signal sendCnt      : sl;
   signal chNoiseCgf   : Slv4Array(127 downto 0);
   signal cmNoiseCgf   : slv(3 downto 0);
   signal trigRate     : slv(31 downto 0);
   signal chDlyCfg     : EmuDlyCfg;
   signal convt        : sl;
   signal adcData      : Slv12Array(127 downto 0);
   signal adcDataDly   : Slv12Array(127 downto 0);
   signal adcDataDlyCM : Slv12Array(127 downto 0);
   signal cmNoise      : slv(11 downto 0);

begin

   emuEnable <= enableTx;
   emuConvt <= convt;

   ---------------------
   -- AXI-Lite Registers
   ---------------------
   U_Reg : entity work.ProtoDuneDpmEmuReg
      generic map (
         TPD_G            => TPD_G,
         SIM_START_G      => SIM_START_G,
         DEFAULT_CNT_G    => DEFAULT_CNT_G)
      port map (
         -- Status/Configuration Interface (clk domain)
         clk             => clk,
         rst             => rst,
         enableTx        => enableTx,
         enableTrig      => enableTrig,
         sendCnt         => sendCnt,
         loopback        => emuLoopback,
         cmNoiseCgf      => cmNoiseCgf,
         chNoiseCgf      => chNoiseCgf,
         chDlyCfg        => chDlyCfg,
         txPreCursor     => txPreCursor,
         txPostCursor    => txPostCursor,
         txDiffCtrl      => txDiffCtrl,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave);

   ---------------------------         
   -- Emulation Data Generator
   ---------------------------         
   U_DataGen : entity work.ProtoDuneDpmEmuData
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Clock and Reset
         clk        => clk,
         rst        => rst,
         -- EMU Data Interface
         enableTx   => enableTx,
         enableTrig => enableTrig,
         convt      => convt,
         cmNoiseCgf => cmNoiseCgf,
         chNoiseCgf => chNoiseCgf,
         timingTrig => timingTrig,
         cmNoise    => cmNoise,
         adcData    => adcData);

   ----------------
   -- Delay Modules
   ----------------
   GEN_VEC :
   for i in 127 downto 0 generate

      U_DlyTaps : entity work.SlvDelay
         generic map (
            TPD_G        => TPD_G,
            SRL_EN_G     => true,
            DELAY_G      => EMU_DELAY_TAPS_C,
            REG_OUTPUT_G => true,
            WIDTH_G      => 12)
         port map (
            clk   => clk,
            en    => convt,
            delay => chDlyCfg(i),
            din   => adcData(i),
            dout  => adcDataDly(i));

      process(clk)
      begin
         if rising_edge(clk) then
            if convt = '1' then
               adcDataDlyCM(i) <= adcDataDly(i) + cmNoise after TPD_G;
            end if;
         end if;
      end process;

   end generate GEN_VEC;

   ---------------------
   -- TX Frame Formatter
   ---------------------
   U_TxFrammer : entity work.ProtoDuneDpmEmuTxFramer
      generic map (
         TPD_G => TPD_G)
      port map (
         -- Clock and Reset
         clk      => clk,
         rst      => rst,
         -- EMU Data Interface
         enable   => enableTx,
         sendCnt  => sendCnt,
         adcData  => adcDataDlyCM,
         convt    => convt,
         timingTs => timingTs,
         -- TX Data Interface
         txData   => emuData,
         txdataK  => emuDataK);

end mapping;
