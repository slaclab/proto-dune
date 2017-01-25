-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : WibTb.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-23
-- Last update: 2017-01-19
-- Platform   : 
-- Standard   : VHDL'93/02
-------------------------------------------------------------------------------
-- Description: Simulation Testbed for testing the WIB Receiver module
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
use work.SsiPkg.all;
use work.ProtoDuneDpmPkg.all;

entity WibTb is end WibTb;

architecture testbed of WibTb is

   constant CLK_PERIOD_C : time := 4 ns;
   constant TPD_G        : time := CLK_PERIOD_C/4;

   signal clk : sl := '0';
   signal rst : sl := '0';

   signal linkP : sl := '0';
   signal linkN : sl := '1';

   signal txData  : slv(15 downto 0) := (others => '0');
   signal txdataK : slv(1 downto 0)  := (others => '0');

   signal rxValid     : sl               := '0';
   signal rxData      : slv(15 downto 0) := (others => '0');
   signal rxdataK     : slv(1 downto 0)  := (others => '0');
   signal rxDecErr    : slv(1 downto 0)  := (others => '0');
   signal rxDispErr   : slv(1 downto 0)  := (others => '0');
   signal rxBufStatus : slv(2 downto 0)  := (others => '0');
   signal cPllLock    : sl               := '0';

begin

   -----------------------------
   -- Generate clocks and resets
   -----------------------------
   U_ClkRst : entity work.ClkRst
      generic map (
         CLK_PERIOD_G      => CLK_PERIOD_C,
         RST_START_DELAY_G => 0 ns,  -- Wait this long into simulation before asserting reset
         RST_HOLD_TIME_G   => 1000 ns)  -- Hold reset for this long)
      port map (
         clkP => clk,
         rst  => rst);

   ---------------------------           
   -- Frame Transmitter Module
   ---------------------------   
   U_Emu : entity work.ProtoDuneDpmEmuDebug
      generic map (
         TPD_G         => TPD_G,
         SIM_START_G   => '1',
         DEFAULT_CNT_G => '1')
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => clk,
         axilRst         => rst,
         axilReadMaster  => AXI_LITE_READ_MASTER_INIT_C,
         axilReadSlave   => open,
         axilWriteMaster => AXI_LITE_WRITE_MASTER_INIT_C,
         axilWriteSlave  => open,
         -- TX EMU Interface
         clk             => clk,
         rst             => rst,
         emuLoopback     => open,
         emuData         => txData,
         emuDataK        => txDataK);

   -------------
   -- GTX Module
   -------------  
   U_GTX : entity work.ProtoDuneDpmGtx7
      generic map (
         TPD_G                 => TPD_G,
         SIM_GTRESET_SPEEDUP_G => "TRUE",
         SIMULATION_G          => true)
      port map (
         -- Clock and Reset
         refClk      => clk,
         clk         => clk,
         rst         => rst,
         -- Debug Interface   
         cPllLock    => cPllLock,
         rxPolarity  => '0',
         rxBufStatus => rxBufStatus,
         -- RTM Interface
         dpmToRtmHsP => linkP,
         dpmToRtmHsN => linkN,
         rtmToDpmHsP => linkP,
         rtmToDpmHsN => linkN,
         -- TX EMU Interface
         emuLoopback => '0',
         emuData     => txData,
         emuDataK    => txDataK,
         -- RX Data Interface (clk domain)
         rxValid     => rxValid,
         rxData      => rxData,
         rxdataK     => rxdataK,
         rxDecErr    => rxDecErr,
         rxDispErr   => rxDispErr);

   ------------------------  
   -- Frame Receiver Module
   ------------------------  
   U_RX : entity work.ProtoDuneDpmWibRxFramer
      generic map (
         TPD_G          => TPD_G,
         CASCADE_SIZE_G => 1)
      port map (
         -- AXI-Lite Port (axilClk domain)
         axilClk         => clk,
         axilRst         => rst,
         axilReadMaster  => AXI_LITE_READ_MASTER_INIT_C,
         axilReadSlave   => open,
         axilWriteMaster => AXI_LITE_WRITE_MASTER_INIT_C,
         axilWriteSlave  => open,
         -- RX Data Interface (clk domain)
         clk             => clk,
         rst             => rst,
         rxValid         => rxValid,
         rxData          => rxData,
         rxdataK         => rxdataK,
         rxDecErr        => rxDecErr,
         rxDispErr       => rxDispErr,
         rxBufStatus     => rxBufStatus,
         cPllLock        => cPllLock,
         -- Timing Interface (clk domain)
         swFlush         => '0',
         runEnable       => '1',
         -- WIB Interface (axilClk domain)
         wibMaster       => open,
         wibSlave        => AXI_STREAM_SLAVE_FORCE_C);

end testbed;
