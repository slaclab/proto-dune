-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmEmuDebug.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-12-01
-- Last update: 2017-01-18
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

use work.WIB_Constants.all;
use work.Convert_IO.all;
use work.EB_IO.all;
use work.COLDATA_IO.all;
use work.types.all;

use work.StdRtlPkg.all;
use work.AxiLitePkg.all;
use work.ProtoDuneDpmPkg.all;

library unisim;
use unisim.vcomponents.all;

entity ProtoDuneDpmEmuDebug is
   generic (
      TPD_G            : time             := 1 ns;
      SIM_START_G      : sl               := '0';
      DEFAULT_CNT_G    : sl               := '0';
      AXI_ERROR_RESP_G : slv(1 downto 0)  := AXI_RESP_DECERR_C;
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
      emuLoopback     : out sl;
      emuEnable       : out sl;
      emuData         : out slv(15 downto 0);
      emuDataK        : out slv(1 downto 0);
      txPreCursor     : out slv(4 downto 0);
      txPostCursor    : out slv(4 downto 0);
      txDiffCtrl      : out slv(3 downto 0));
end ProtoDuneDpmEmuDebug;

architecture rtl of ProtoDuneDpmEmuDebug is

   signal enableTx : sl;
   signal sclk     : sl;
   signal srst     : sl;

   signal convert     : convert_t;
   signal FEMB_stream : FEMB_stream_array_t(3 downto 0);
   signal FEMB_read   : std_logic_vector(3 downto 0);
   signal fifoDin     : std_logic_vector((9 * 4) -1 downto 0);
   signal fifoDout    : std_logic_vector((9 * 4) -1 downto 0);
   signal monitor     : FEMB_EB_Monitor_t;
   signal control     : FEMB_EB_Control_t      := DEFAULT_FEMB_EB_CONTROL;
   signal cnt         : natural range 0 to 125 := 1;

   type StateType is (
      IDLE_S,
      MOVE_S);

   type RegType is record
      rdEn    : sl;
      txData  : slv(15 downto 0);
      txdataK : slv(1 downto 0);
      state   : StateType;
   end record RegType;
   constant REG_INIT_C : RegType := (
      rdEn    => '0',
      txData  => (others => '0'),
      txdataK => (others => '0'),
      state   => IDLE_S);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal rdEn    : sl := '0';
   signal rdValid : sl := '0';

begin

   emuEnable <= enableTx;

   ---------------------
   -- AXI-Lite Registers
   ---------------------
   U_Reg : entity work.ProtoDuneDpmEmuReg
      generic map (
         TPD_G            => TPD_G,
         SIM_START_G      => SIM_START_G,
         DEFAULT_CNT_G    => DEFAULT_CNT_G,
         AXI_ERROR_RESP_G => AXI_ERROR_RESP_G)
      port map (
         -- Status/Configuration Interface (clk domain)
         clk             => clk,
         rst             => rst,
         enableTx        => enableTx,
         enableTrig      => open,
         sendCnt         => open,
         loopback        => emuLoopback,
         cmNoiseCgf      => open,
         chNoiseCgf      => open,
         chDlyCfg        => open,
         trigRate        => open,
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
   U_FEMB_EventBuilder : entity work.FEMB_EventBuilder
      port map (
         clk         => sclk,
         reset       => srst,
         convert     => convert,
         FEMB_stream => FEMB_stream,
         FEMB_read   => FEMB_read,
         data_out    => fifoDin(31 downto 0),
         data_k_out  => fifoDin(35 downto 32),
         monitor     => monitor,
         control     => control);

   U_BUFR : BUFR
      generic map (
         BUFR_DIVIDE => "2",
         SIM_DEVICE  => "7SERIES")
      port map (
         I   => clk,
         CE  => '1',
         CLR => '0',
         O   => sclk);

   U_RstSync : entity work.RstSync
      generic map (
         TPD_G => TPD_G)
      port map (
         clk      => sclk,
         asyncRst => rst,
         syncRst  => srst);

   process(sclk)
      variable i : natural;
   begin
      if rising_edge(sclk) then
         if srst = '1' then
            convert.reset_count   <= (others => '0') after TPD_G;
            convert.convert_count <= (others => '0') after TPD_G;
            convert.time_stamp    <= (others => '0') after TPD_G;
            convert.slot_id       <= (others => '0') after TPD_G;
            convert.crate_id      <= (others => '0') after TPD_G;
            convert.trigger       <= '0'             after TPD_G;
            cnt                   <= 1               after TPD_G;
            for i in 3 downto 0 loop
               FEMB_stream(i).valid          <= '1'             after TPD_G;
               FEMB_stream(i).capture_errors <= (others => '0') after TPD_G;
               FEMB_stream(i).CD_errors      <= (others => '0') after TPD_G;
               FEMB_stream(i).CD_timestamp   <= (others => '0') after TPD_G;
               FEMB_stream(i).data_out       <= (others => '0') after TPD_G;
            end loop;
         else
            if cnt = 125 then
               cnt             <= 1        after TPD_G;
               convert.trigger <= enableTx after TPD_G;
            else
               cnt             <= cnt + 1 after TPD_G;
               convert.trigger <= '0'     after TPD_G;
            end if;
            for i in 3 downto 0 loop
               if (convert.trigger = '1') then
                  FEMB_stream(i).CD_timestamp <= FEMB_stream(i).CD_timestamp + 1 after TPD_G;
                  FEMB_stream(i).data_out     <= FEMB_stream(i).data_out + 1     after TPD_G;
               end if;
            end loop;
         end if;
      end if;
   end process;

   control.debug <= '1';

   U_Resize : entity work.SynchronizerFifo
      generic map (
         TPD_G        => TPD_G,
         DATA_WIDTH_G => 36)
      port map (
         wr_clk => sclk,
         din    => fifoDin,
         -- Read Ports (rd_clk domain)
         rd_clk => clk,
         rd_en  => rdEn,
         valid  => rdValid,
         dout   => fifoDout);

   comb : process (fifoDout, r, rdValid, rst) is
      variable v : RegType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      v.rdEn := '0';

      -- State Machine
      case r.state is
         ----------------------------------------------------------------------
         when IDLE_S =>
            if rdValid = '1' then
               v.txData  := fifoDout(15 downto 0);
               v.txDataK := fifoDout(33 downto 32);
               v.state   := MOVE_S;
            end if;
         ----------------------------------------------------------------------
         when MOVE_S =>
            v.txData  := fifoDout(31 downto 16);
            v.txDataK := fifoDout(35 downto 34);
            v.rdEn    := '1';
            v.state   := IDLE_S;
      ----------------------------------------------------------------------
      end case;

      -- Synchronous Reset
      if (rst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      rdEn     <= v.rdEn;
      emuData  <= r.txData;
      emuDataK <= r.txDataK;

   end process comb;

   seq : process (clk) is
   begin
      if (rising_edge(clk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

end rtl;
