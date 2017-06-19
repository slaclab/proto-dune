-------------------------------------------------------------------------------
-- File       : WibTb.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-08-23
-- Last update: 2017-06-19
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
use work.RceG3Pkg.all;

entity WibTb is end WibTb;

architecture testbed of WibTb is

   constant BYPASS_GTX_C     : boolean := true;
   constant WIB_CLK_PERIOD_C : time    := 4.001 ns;
   constant DMA_CLK_PERIOD_C : time    := 5.001 ns;
   constant CLK_PERIOD_C     : time    := 8 ns;
   constant TPD_G            : time    := 1 ns;

   signal clk    : sl := '0';
   signal rst    : sl := '0';
   signal wibClk : sl := '0';
   signal wibRst : sl := '0';
   signal dmaClk : sl := '0';
   signal dmaRst : sl := '0';

   signal linkP : sl := '0';
   signal linkN : sl := '1';

   signal txData  : slv(15 downto 0) := (others => '0');
   signal txdataK : slv(1 downto 0)  := (others => '0');

   signal rxValid     : sl               := '1';
   signal rxData      : slv(15 downto 0) := (others => '0');
   signal rxdataK     : slv(1 downto 0)  := (others => '0');
   signal rxDecErr    : slv(1 downto 0)  := (others => '0');
   signal rxDispErr   : slv(1 downto 0)  := (others => '0');
   signal rxBufStatus : slv(2 downto 0)  := (others => '0');
   signal cPllLock    : sl               := '1';

   type StateType is (
      HDR_S,
      SOF_S,
      MOVE_S,
      TAIL_S,
      WAIT_S);

   type RegType is record
      swFlush   : sl;
      runEnable : sl;
      error     : sl;
      errorDly  : sl;
      wrd       : Slv16Array(3 downto 0);
      pressure  : slv(7 downto 0);
      cnt       : natural range 0 to 255;
      frame     : natural range 0 to 1023;
      dmaSlave  : AxiStreamSlaveType;
      state     : StateType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      swFlush   => '0',
      runEnable => '1',
      error     => '0',
      errorDly  => '0',
      wrd       => (others => x"FFFF"),
      pressure  => x"00",
      cnt       => 0,
      frame     => 0,
      dmaSlave  => AXI_STREAM_SLAVE_INIT_C,
      state     => HDR_S);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal wibMaster : AxiStreamMasterType;
   signal wibSlave  : AxiStreamSlaveType;

   signal hlsMaster : AxiStreamMasterType;
   signal hlsSlave  : AxiStreamSlaveType;

   signal ssiMaster : AxiStreamMasterType;
   signal ssiSlave  : AxiStreamSlaveType;

   signal muxMaster : AxiStreamMasterType;
   signal muxSlave  : AxiStreamSlaveType;

   signal dmaMaster : AxiStreamMasterType;
   signal dmaSlave  : AxiStreamSlaveType;

   signal axilReadMaster  : AxiLiteReadMasterType  := AXI_LITE_READ_MASTER_INIT_C;
   signal axilReadSlave   : AxiLiteReadSlaveType   := AXI_LITE_READ_SLAVE_INIT_C;
   signal axilWriteMaster : AxiLiteWriteMasterType := AXI_LITE_WRITE_MASTER_INIT_C;
   signal axilWriteSlave  : AxiLiteWriteSlaveType  := AXI_LITE_WRITE_SLAVE_INIT_C;

begin

   -----------------------------
   -- Generate clocks and resets
   -----------------------------
   U_ClkRst0 : entity work.ClkRst
      generic map (
         CLK_PERIOD_G      => CLK_PERIOD_C,
         RST_START_DELAY_G => 0 ns,  -- Wait this long into simulation before asserting reset
         RST_HOLD_TIME_G   => 1000 ns)  -- Hold reset for this long)
      port map (
         clkP => clk,
         rst  => rst);

   U_ClkRst1 : entity work.ClkRst
      generic map (
         CLK_PERIOD_G      => WIB_CLK_PERIOD_C,
         RST_START_DELAY_G => 0 ns,  -- Wait this long into simulation before asserting reset
         RST_HOLD_TIME_G   => 1000 ns)  -- Hold reset for this long)
      port map (
         clkP => wibClk,
         rst  => wibRst);

   U_ClkRst2 : entity work.ClkRst
      generic map (
         CLK_PERIOD_G      => DMA_CLK_PERIOD_C,
         RST_START_DELAY_G => 0 ns,  -- Wait this long into simulation before asserting reset
         RST_HOLD_TIME_G   => 1000 ns)  -- Hold reset for this long)
      port map (
         clkP => dmaClk,
         rst  => dmaRst);

   ---------------------------           
   -- Frame Transmitter Module
   ---------------------------   
   U_Emu : entity work.ProtoDuneDpmEmu
      generic map (
         TPD_G         => TPD_G,
         SIM_START_G   => '1',
         DEFAULT_CNT_G => '1')
      port map (
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => clk,
         axilRst         => rst,
         axilReadMaster  => AXI_LITE_READ_MASTER_INIT_C,
         axilWriteMaster => AXI_LITE_WRITE_MASTER_INIT_C,
         -- TX EMU Interface
         clk             => wibClk,
         rst             => wibRst,
         timingTrig      => '0',
         timingTs        => (others => '0'),
         emuData         => txData,
         emuDataK        => txDataK);

   GEN_GTX : if (BYPASS_GTX_C = false) generate
      U_GTX : entity work.ProtoDuneDpmGtx7
         generic map (
            TPD_G                 => TPD_G,
            SIM_GTRESET_SPEEDUP_G => "TRUE",
            SIMULATION_G          => true)
         port map (
            -- Clock and Reset
            refClk      => wibClk,
            clk         => wibClk,
            rst         => wibRst,
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
            emuClk      => wibClk,
            emuRst      => wibRst,
            emuLoopback => '0',
            emuData     => txData,
            emuDataK    => txDataK,
            -- RX Data Interface (clk domain)
            rxValid     => rxValid,
            rxData      => rxData,
            rxdataK     => rxdataK,
            rxDecErr    => rxDecErr,
            rxDispErr   => rxDispErr);
   end generate;

   GEN_BYPASS : if (BYPASS_GTX_C = true) generate
      rxData  <= txData;
      rxdataK <= txDataK;
   end generate;

   ------------------------  
   -- Frame Receiver Module
   ------------------------  
   U_RX : entity work.ProtoDuneDpmWibRxFramer
      generic map (
         TPD_G          => TPD_G,
         CASCADE_SIZE_G => 8)
      port map (
         -- AXI-Lite Port (axilClk domain)
         axilClk         => clk,
         axilRst         => rst,
         axilReadMaster  => AXI_LITE_READ_MASTER_INIT_C,
         axilReadSlave   => open,
         axilWriteMaster => AXI_LITE_WRITE_MASTER_INIT_C,
         axilWriteSlave  => open,
         -- RX Data Interface (clk domain)
         clk             => wibClk,
         rst             => wibRst,
         rxValid         => rxValid,
         rxData          => rxData,
         rxdataK         => rxdataK,
         rxDecErr        => rxDecErr,
         rxDispErr       => rxDispErr,
         rxBufStatus     => rxBufStatus,
         cPllLock        => cPllLock,
         -- Timing Interface (clk domain)
         swFlush         => r.swFlush,
         runEnable       => r.runEnable,
         -- WIB Interface (axilClk domain)
         wibMaster       => wibMaster,
         wibSlave        => wibSlave);

   U_HLS : entity work.DuneDataCompression
      generic map (
         TPD_G   => TPD_G,
         INDEX_G => 0)
      port map (
         -- AXI-Lite Port (axilClk domain)
         axilClk         => clk,
         axilRst         => rst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave,
         -- Inbound Interface
         sAxisMaster     => wibMaster,
         sAxisSlave      => wibSlave,
         -- Outbound Interface
         mAxisMaster     => hlsMaster,
         mAxisSlave      => hlsSlave);

   U_Filter : entity work.SsiFifo
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         INT_PIPE_STAGES_G   => 1,
         PIPE_STAGES_G       => 1,
         SLAVE_READY_EN_G    => true,
         VALID_THOLD_G       => 1,
         -- FIFO configurations
         BRAM_EN_G           => false,
         GEN_SYNC_FIFO_G     => true,
         CASCADE_SIZE_G      => 1,
         FIFO_ADDR_WIDTH_G   => 4,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
         MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
      port map (
         -- Slave Port
         sAxisClk    => clk,
         sAxisRst    => rst,
         sAxisMaster => hlsMaster,
         sAxisSlave  => hlsSlave,
         -- Master Port
         mAxisClk    => clk,
         mAxisRst    => rst,
         mAxisMaster => ssiMaster,
         mAxisSlave  => ssiSlave);

   U_Fifo : entity work.AxiStreamFifoV2
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         PIPE_STAGES_G       => 0,
         -- VALID_THOLD_G       => 1,
         VALID_THOLD_G       => 128,
         VALID_BURST_MODE_G  => true,
         -- FIFO configurations
         BRAM_EN_G           => true,
         XIL_DEVICE_G        => "7SERIES",
         USE_BUILT_IN_G      => false,
         GEN_SYNC_FIFO_G     => false,
         CASCADE_SIZE_G      => 1,
         FIFO_ADDR_WIDTH_G   => 12,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
         MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
      port map (
         -- Slave Port
         sAxisClk    => clk,
         sAxisRst    => rst,
         sAxisMaster => ssiMaster,
         sAxisSlave  => ssiSlave,
         -- Master Port
         mAxisClk    => dmaClk,
         mAxisRst    => dmaRst,
         mAxisMaster => muxMaster,
         mAxisSlave  => muxSlave);

   U_Mux : entity work.AxiStreamMux
      generic map (
         TPD_G          => TPD_G,
         PIPE_STAGES_G  => 1,
         NUM_SLAVES_G   => 1,
         MODE_G         => "INDEXED",
         TDEST_LOW_G    => 0,
         ILEAVE_EN_G    => true,
         ILEAVE_REARB_G => 0)
      port map (
         -- Clock and reset
         axisClk         => dmaClk,
         axisRst         => dmaRst,
         -- Slaves
         sAxisMasters(0) => muxMaster,
         sAxisSlaves(0)  => muxSlave,
         -- Master
         mAxisMaster  => dmaMaster,
         mAxisSlave   => dmaSlave);

   comb : process (dmaMaster, dmaRst, r) is
      variable v          : RegType;
      variable cntPattern : slv(63 downto 0);
   begin
      -- Latch the current value
      v := r;

      v.dmaSlave := AXI_STREAM_SLAVE_INIT_C;
      v.errorDly := r.error;
      cntPattern := r.wrd(3) & r.wrd(2) & r.wrd(1) & r.wrd(0);

      v.swFlush   := '0';
      v.runEnable := '1';

      v.pressure := r.pressure + 1;

      if (r.pressure < 68) then
         -- State Machine
         case r.state is
            ----------------------------------------------------------------------
            when HDR_S =>
               if (dmaMaster.tValid = '1') then
                  v.dmaSlave.tReady := '1';
                  if (dmaMaster.tData(63 downto 0) /= x"0000_0001_1100_0000") then
                     v.error := '1';
                  end if;
                  v.state := SOF_S;
               end if;
            ----------------------------------------------------------------------
            when SOF_S =>
               if (dmaMaster.tValid = '1') then
                  v.dmaSlave.tReady := '1';
                  if (dmaMaster.tData(63 downto 0) /= x"0003_0002_0001_00BC") then
                     v.error := '1';
                  end if;
                  v.wrd(3) := x"0007";
                  v.wrd(2) := x"0006";
                  v.wrd(1) := x"0005";
                  v.wrd(0) := x"0004";
                  v.state  := MOVE_S;
               end if;
            ----------------------------------------------------------------------
            when MOVE_S =>
               if (dmaMaster.tValid = '1') then
                  v.dmaSlave.tReady := '1';
                  if (dmaMaster.tData(63 downto 0) /= cntPattern) then
                     v.error := '1';
                  end if;
                  v.wrd(3) := r.wrd(3) + 4;
                  v.wrd(2) := r.wrd(2) + 4;
                  v.wrd(1) := r.wrd(1) + 4;
                  v.wrd(0) := r.wrd(0) + 4;
                  v.cnt    := r.cnt + 1;
                  if r.cnt = 28 then
                     v.cnt := 0;
                     if r.frame = 1023 then
                        v.frame := 0;
                        v.state := TAIL_S;
                     else
                        v.frame := r.frame + 1;
                        v.state := SOF_S;
                     end if;
                  end if;
                  if (dmaMaster.tLast = '1') then
                     v.error := '1';
                  end if;
               end if;
            ----------------------------------------------------------------------
            when TAIL_S =>
               if (dmaMaster.tValid = '1') then
                  v.dmaSlave.tReady := '1';
                  if (dmaMaster.tData(63 downto 0) /= x"708b_309e_1103_c010") then
                     v.error := '1';
                  end if;
                  if (dmaMaster.tLast = '0') then
                     v.error := '1';
                  end if;
                  v.state := WAIT_S;
               end if;
            ----------------------------------------------------------------------
            when WAIT_S =>
               if (r.cnt = 255) then
                  v.cnt   := 0;
                  v.state := HDR_S;
               else
                  v.cnt := r.cnt + 1;
               end if;
         ----------------------------------------------------------------------
         end case;
      end if;

      -- Synchronous Reset
      if (dmaRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs        
      dmaSlave <= v.dmaSlave;

   end process comb;

   seq : process (dmaClk) is
   begin
      if (rising_edge(dmaClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;


   test : process is
   begin
      wait until rst = '1';
      wait until rst = '0';
      wait until clk = '1';
      axiLiteBusSimWrite(clk, axilWriteMaster, axilWriteSlave, X"0000_0000", x"0000_0081", true);
   end process;

   process(r)
   begin
      if r.errorDly = '1' then
         assert false
            report "Simulation Failed!" severity failure;
      end if;
   end process;

end testbed;
