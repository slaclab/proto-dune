-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmWibRxFramer.vhd
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
use work.AxiStreamPkg.all;
use work.RceG3Pkg.all;
use work.ProtoDuneDpmPkg.all;

entity ProtoDuneDpmWibRxFramer is
   generic (
      TPD_G            : time            := 1 ns;
      AXI_CLK_FREQ_G   : real            := 125.0E+6;  -- units of Hz
      AXI_ERROR_RESP_G : slv(1 downto 0) := AXI_RESP_DECERR_C;
      CASCADE_SIZE_G   : positive        := 1);
   port (
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- RX Data Interface (clk domain)
      clk             : in  sl;
      rst             : in  sl;
      rxValid         : in  sl;
      rxData          : in  slv(15 downto 0);
      rxdataK         : in  slv(1 downto 0);
      rxDecErr        : in  slv(1 downto 0);
      rxDispErr       : in  slv(1 downto 0);
      rxBufStatus     : in  slv(2 downto 0);
      rxPolarity      : out sl;
      txPolarity      : out sl;
      cPllLock        : in  sl;
      gtRst           : out sl;
      logEn           : out sl;
      logClr          : out sl;
      -- Timing Interface (clk domain)
      swFlush         : in  sl;
      runEnable       : in  sl;
      -- WIB Interface (axilClk domain)
      wibMaster       : out AxiStreamMasterType;
      wibSlave        : in  AxiStreamSlaveType);
end ProtoDuneDpmWibRxFramer;

architecture rtl of ProtoDuneDpmWibRxFramer is

   constant IDLE_C : slv(15 downto 0) := (K28_2_C & K28_1_C);

   type StateType is (
      IDLE_S,
      MOVE_S,
      CRC_LO_S,
      CRC_HI_S,
      LAST_S);

   type DbgStateType is (
      IDLE_S,
      LOG_S,
      DLY_S);

   type RegType is record
      logEn      : sl;
      oneShot    : sl;
      wibSofDet  : sl;
      swFlush    : sl;
      notSwFlush : sl;
      rxValid    : sl;
      rxData     : slv(15 downto 0);
      rxdataK    : slv(1 downto 0);
      eofe       : sl;
      errPktDrop : sl;
      errPktLen  : sl;
      errCrc     : sl;
      crcValid   : sl;
      crcRst     : sl;
      crcData    : slv(15 downto 0);
      crc        : slv(31 downto 0);
      cnt        : natural range 0 to 124;
      dbgCnt     : natural range 0 to 511;
      pktLen     : slv(7 downto 0);
      txMaster   : AxiStreamMasterType;
      state      : StateType;
      dbgCrcRcv  : slv(31 downto 0);
      dbgCrcCalc : slv(31 downto 0);
      dbgState   : DbgStateType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      logEn      => '0',
      oneShot    => '0',
      wibSofDet  => '0',
      swFlush    => '0',
      notSwFlush => '1',
      rxValid    => '0',
      rxData     => (others => '0'),
      rxdataK    => (others => '0'),
      eofe       => '0',
      errPktDrop => '0',
      errPktLen  => '0',
      errCrc     => '0',
      crcValid   => '0',
      crcRst     => '1',
      crcData    => (others => '0'),
      crc        => (others => '0'),
      cnt        => 0,
      dbgCnt     => 0,
      pktLen     => (others => '0'),
      txMaster   => AXI_STREAM_MASTER_INIT_C,
      state      => IDLE_S,
      dbgCrcRcv  => (others => '0'),
      dbgCrcCalc => (others => '0'),
      dbgState   => IDLE_S);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal txCtrl    : AxiStreamCtrlType;
   signal crcResult : slv(31 downto 0);

   signal master     : AxiStreamMasterType;
   signal slave      : AxiStreamSlaveType;
   signal termFrame  : sl;
   signal pktLenStrb : sl;
   signal startLog   : sl;
   signal oneShotLog : sl;

   -- attribute dont_touch              : string;
   -- attribute dont_touch of r         : signal is "TRUE";
   -- attribute dont_touch of crcResult : signal is "TRUE";

begin

   comb : process (crcResult, oneShotLog, r, rst, runEnable, rxData, rxDecErr,
                   rxDispErr, rxValid, rxdataK, startLog, swFlush, txCtrl) is
      variable v : RegType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the signals
      v.txMaster.tValid := '0';
      v.txMaster.tLast  := '0';
      v.txMaster.tUser  := (others => '0');
      v.crcRst          := '0';
      v.crcValid        := '0';
      v.errPktDrop      := '0';
      v.errPktLen       := '0';
      v.errCrc          := '0';
      v.wibSofDet       := '0';

      -- Register the 8B10B and check for valid data
      v.rxValid := rxValid and not(uOr(rxDecErr)) and not(uOr(rxDispErr));
      v.rxData  := rxData;
      v.rxdataK := rxdataK;

      -- State Machine
      case r.state is
         ----------------------------------------------------------------------
         when IDLE_S =>                 -- CNT = 0
            -- Reset the flag
            v.eofe := '0';
            -- Check for start of packet
            if (r.rxValid = '1') and (r.rxdataK = "01") and (r.rxData(7 downto 0) = K28_5_C) then
               -- Check for back pressure
               if (txCtrl.pause = '1') then
                  -- Error Detected
                  v.errPktDrop := '1';
               else
                  -- Forward the data to HLS
                  v.txMaster.tValid             := '1';
                  v.txMaster.tData(15 downto 0) := r.rxData;
                  -- Set firstTuser
                  axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_RUN_EN_C, runEnable, 0);
                  axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_SOF_C, '1', 0);
                  -- Forward the data to CRC 
                  v.crcValid                    := '1';
                  v.crcData                     := r.rxData;
                  -- Save the flush value
                  v.swFlush                     := swFlush;
                  v.notSwFlush                  := not(swFlush);
                  -- Preset the counter
                  v.cnt                         := 1;
                  -- Next state
                  v.state                       := MOVE_S;
               end if;
            -- Check for flush packet insertion 
            elsif (swFlush = '1') and (txCtrl.pause = '0') then
               -- Forward the data to HLS
               v.txMaster.tValid             := '1';
               v.txMaster.tData(15 downto 0) := r.rxData;
               -- Set firstTuser
               axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_RUN_EN_C, runEnable, 0);
               axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_SOF_C, '1', 0);
               -- Forward the data to CRC 
               v.crcValid                    := '1';
               v.crcData                     := r.rxData;
               -- Save the flush value
               v.swFlush                     := swFlush;
               v.notSwFlush                  := not(swFlush);
               -- Preset the counter
               v.cnt                         := 1;
               -- Next state
               v.state                       := MOVE_S;
            end if;
         ----------------------------------------------------------------------
         when MOVE_S =>                 -- CNT = [119:1]
            -- Forward the data to HLS
            v.txMaster.tValid             := '1';
            v.txMaster.tData(15 downto 0) := r.rxData;
            -- Forward the data to CRC 
            v.crcValid                    := '1';
            v.crcData                     := r.rxData;
            -- Check if EOFE not set yet
            if r.eofe = '0' then
               -- Check no invalid RX data
               if (r.rxValid /= '1') or (r.rxdataK /= "00") then
                  -- Error Detected
                  v.errPktLen := r.notSwFlush;
                  v.eofe      := r.notSwFlush;
               end if;
            end if;
            -- Increment the counter
            v.cnt := r.cnt + 1;
            -- Check the counter 
            if r.cnt = 119 then
               -- Holding off forwarding data to HLS
               v.txMaster.tValid := '0';
               -- Next state
               v.state           := CRC_LO_S;
            end if;
         ----------------------------------------------------------------------
         when CRC_LO_S =>               -- CNT  =  120
            -- Latch the WIB CRC 
            v.crc(15 downto 0) := r.rxData;
            -- Check if EOFE not set yet
            if r.eofe = '0' then
               -- Check no invalid RX data
               if (r.rxValid /= '1') or (r.rxdataK /= "00") then
                  -- Error Detected
                  v.errPktLen := r.notSwFlush;
                  v.eofe      := r.notSwFlush;
               end if;
            end if;
            -- Next state
            v.state := CRC_HI_S;
         ----------------------------------------------------------------------
         when CRC_HI_S =>               -- CNT = 121       
            -- Latch the WIB CRC 
            v.crc(31 downto 16) := r.rxData;
            -- Check if EOFE not set yet
            if r.eofe = '0' then
               -- Check no invalid RX data
               if (r.rxValid /= '1') or (r.rxdataK /= "00") then
                  -- Error Detected
                  v.errPktLen := r.notSwFlush;
                  v.eofe      := r.notSwFlush;
               end if;
            end if;
            -- Next state
            v.state := LAST_S;
         ----------------------------------------------------------------------
         when LAST_S =>                 -- CNT = 122  
            -- Check if EOFE not set yet
            if r.eofe = '0' then
               -- Check no IDLE characters
               if (r.rxValid /= '1') or (r.rxdataK /= "11") or (r.rxdata /= IDLE_C) then
                  -- Error Detected
                  v.errPktLen := r.notSwFlush;
                  v.eofe      := r.notSwFlush;
               end if;
               -- Check for invalid CRC 
               if (r.crc /= crcResult) and (v.eofe = '0') then
                  -- Error Detected
                  v.errCrc := r.notSwFlush;
                  v.eofe   := r.notSwFlush;
                  -- Check if we are in the debug logging state
                  if (r.dbgState = LOG_S) then
                     -- Latch the CRC received and calculated values
                     v.dbgCrcRcv  := r.crc;
                     v.dbgCrcCalc := crcResult;
                  end if;
               end if;
            end if;
            -- Send the last RX data
            v.txMaster.tValid := '1';
            v.txMaster.tLast  := '1';
            -- Set lastTuser
            axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_EOFE_C, v.eofe);
            axiStreamSetUserBit(WIB_AXIS_CONFIG_C, v.txMaster, WIB_FLUSH_C, r.swFlush);
            -- Reset the CRC module
            v.crcRst          := '1';
            -- Next state
            v.state           := IDLE_S;
      ----------------------------------------------------------------------
      end case;

      -- Check for one-shot logging
      if (oneShotLog = '1') then
         -- Set the flag
         v.oneShot := '1';
      end if;

      -- Debug State Machine
      case (r.dbgState) is
         ----------------------------------------------------------------------
         when IDLE_S =>
            -- Reset the flag
            v.logEn := '0';
            -- Wait for a start
            if (startLog = '1') or (oneShotLog = '1') then
               -- Next state
               v.dbgState := LOG_S;
            end if;
         ----------------------------------------------------------------------
         when LOG_S =>
            -- Set the flag
            v.logEn := '1';
            -- Check for timeout
            if (r.dbgCnt = 511) then
               -- Check for error event
               if (r.eofe = '1') or (r.oneShot = '1') then
                  -- Reset the counter
                  v.dbgCnt   := 0;
                  -- Reset the flag
                  v.oneShot  := '0';
                  -- Next state
                  v.dbgState := DLY_S;
               end if;
            else
               -- Increment the counter
               v.dbgCnt := r.dbgCnt + 1;
            end if;
         ----------------------------------------------------------------------
         when DLY_S =>
            -- Increment the counter
            v.dbgCnt := r.dbgCnt + 1;
            -- Check for trigger event hold off
            if (r.dbgCnt = 5) then
               -- Reset the counter
               v.dbgCnt   := 0;
               -- Next state
               v.dbgState := IDLE_S;
            end if;
      ----------------------------------------------------------------------
      end case;

      -- Monitor the packet length
      if r.state = IDLE_S then
         v.pktLen := x"00";
      elsif v.errPktLen = '0' then
         v.pktLen := v.pktLen + 1;
      end if;

      -- Monitor for SOF independent of state machine phase
      if (r.rxValid = '1') and (r.rxdataK = "01") and (r.rxData(7 downto 0) = K28_5_C) then
         v.wibSofDet := '1';
      end if;

      -- Reset
      if (rst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      pktLenStrb <= r.txMaster.tLast;
      logEn      <= r.logEn;
      logClr     <= startLog or oneShotLog;

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
   -- -- TPD_G        => TPD_G,
   -- BYTE_WIDTH_G => 2)
   -- port map (
   -- crcClk       => clk,
   -- crcReset     => r.crcRst,
   -- crcDataWidth => "001",         -- 2 bytes 
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

   U_TxFifo : entity work.AxiStreamFifoV2
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         PIPE_STAGES_G       => 1,
         SLAVE_READY_EN_G    => false,  -- Using pause
         VALID_THOLD_G       => 1,
         -- FIFO configurations
         BRAM_EN_G           => true,
         XIL_DEVICE_G        => "7SERIES",
         USE_BUILT_IN_G      => false,
         GEN_SYNC_FIFO_G     => false,
         CASCADE_SIZE_G      => 1,
         FIFO_ADDR_WIDTH_G   => 10,
         FIFO_FIXED_THRESH_G => true,
         FIFO_PAUSE_THRESH_G => 512,  -- 512 = 1024 deep FIFO - ((122 WIB 16-bit words) + padding)
         CASCADE_PAUSE_SEL_G => 0,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => WIB_AXIS_CONFIG_C,
         MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
      port map (
         -- Slave Port
         sAxisClk    => clk,
         sAxisRst    => rst,
         sAxisMaster => r.txMaster,
         sAxisCtrl   => txCtrl,
         -- Master Port
         mAxisClk    => axilClk,
         mAxisRst    => axilRst,
         mAxisMaster => master,
         mAxisSlave  => slave);

   U_SsiFifo : entity work.SsiFifo
      generic map (
         -- General Configurations
         TPD_G               => TPD_G,
         INT_PIPE_STAGES_G   => 0,
         PIPE_STAGES_G       => 1,
         SLAVE_READY_EN_G    => true,
         EN_FRAME_FILTER_G   => true,
         OR_DROP_FLAGS_G     => true,
         VALID_THOLD_G       => 1,
         -- FIFO configurations
         BRAM_EN_G           => true,
         GEN_SYNC_FIFO_G     => true,
         CASCADE_SIZE_G      => CASCADE_SIZE_G,
         FIFO_ADDR_WIDTH_G   => 9,
         -- AXI Stream Port Configurations
         SLAVE_AXI_CONFIG_G  => RCEG3_AXIS_DMA_CONFIG_C,
         MASTER_AXI_CONFIG_G => RCEG3_AXIS_DMA_CONFIG_C)
      port map (
         -- Slave Port
         sAxisClk       => axilClk,
         sAxisRst       => axilRst,
         sAxisMaster    => master,
         sAxisSlave     => slave,
         -- Master Port
         mAxisClk       => axilClk,
         mAxisRst       => axilRst,
         mAxisMaster    => wibMaster,
         mAxisSlave     => wibSlave,
         mAxisTermFrame => termFrame);

   U_Reg : entity work.ProtoDuneDpmWibRxFramerReg
      generic map (
         TPD_G            => TPD_G,
         AXI_CLK_FREQ_G   => AXI_CLK_FREQ_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Status/Configuration Interface (clk domain)
         clk             => clk,
         rst             => rst,
         rxLinkUp        => rxValid,
         rxDecErr        => rxDecErr,
         rxDispErr       => rxDispErr,
         rxBufStatus     => rxBufStatus,
         rxPolarity      => rxPolarity,
         txPolarity      => txPolarity,
         cPllLock        => cPllLock,
         pktSent         => r.txMaster.tLast,
         backpressure    => txCtrl.pause,
         errPktDrop      => r.errPktDrop,
         errPktLen       => r.errPktLen,
         errCrc          => r.errCrc,
         overflow        => txCtrl.overflow,
         termFrame       => termFrame,
         pktLenStrb      => pktLenStrb,
         pktLen          => r.pktLen,
         wibSofDet       => r.wibSofDet,
         gtRst           => gtRst,
         startLog        => startLog,
         oneShotLog      => oneShotLog,
         dbgCrcRcv       => r.dbgCrcRcv,
         dbgCrcCalc      => r.dbgCrcCalc,
         -- AXI-Lite Interface (axilClk domain)
         axilClk         => axilClk,
         axilRst         => axilRst,
         axilReadMaster  => axilReadMaster,
         axilReadSlave   => axilReadSlave,
         axilWriteMaster => axilWriteMaster,
         axilWriteSlave  => axilWriteSlave);

end rtl;
