-------------------------------------------------------------------------------
-- File       : ProtoDuneDpmHlsMon.vhd
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-09-01
-- Last update: 2018-09-17
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
use work.SsiPkg.all;
use work.ProtoDuneDpmPkg.all;
use work.RceG3Pkg.all;

entity ProtoDuneDpmHlsMon is
   generic (
      TPD_G          : time := 1 ns;
      AXI_CLK_FREQ_G : real := 125.0E+6);  -- units of Hz
   port (
      -- AXI-Lite Interface (axilClk domain)
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType;
      -- HLS Interface (axilClk domain)
      ibHlsMasters    : in  AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      ibHlsSlaves     : in  AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);
      obHlsMasters    : in  AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      obHlsSlaves     : out AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);
      hlsMasters      : out AxiStreamMasterArray(WIB_SIZE_C-1 downto 0);
      hlsSlaves       : in  AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0));
end ProtoDuneDpmHlsMon;

architecture rtl of ProtoDuneDpmHlsMon is

   type RegType is record
      blowoff        : slv(WIB_SIZE_C-1 downto 0);
      hardRst        : sl;
      axilReadSlave  : AxiLiteReadSlaveType;
      axilWriteSlave : AxiLiteWriteSlaveType;
   end record;

   constant REG_INIT_C : RegType := (
      blowoff        => (others => '1'),
      hardRst        => '0',
      axilReadSlave  => AXI_LITE_READ_SLAVE_INIT_C,
      axilWriteSlave => AXI_LITE_WRITE_SLAVE_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;


   signal ibFrameRate : Slv32Array(WIB_SIZE_C-1 downto 0);
   signal ibBandwidth : Slv64Array(WIB_SIZE_C-1 downto 0);

   signal obFrameRate : Slv32Array(WIB_SIZE_C-1 downto 0);
   signal obBandwidth : Slv64Array(WIB_SIZE_C-1 downto 0);

   signal slaves : AxiStreamSlaveArray(WIB_SIZE_C-1 downto 0);

   attribute dont_touch      : string;
   attribute dont_touch of r : signal is "TRUE";

begin

   GEN_LINK :
   for i in (WIB_SIZE_C-1) downto 0 generate
      ------------------------------               
      -- Inbound AXIS Monitor Module
      ------------------------------               
      U_Mon0 : entity work.AxiStreamMon
         generic map (
            TPD_G           => TPD_G,
            AXIS_CLK_FREQ_G => AXI_CLK_FREQ_G,
            AXIS_CONFIG_G   => RCEG3_AXIS_DMA_CONFIG_C)
         port map (
            -- AXIS Stream Interface
            axisClk    => axilClk,
            axisRst    => axilRst,
            axisMaster => ibHlsMasters(i),
            axisSlave  => ibHlsSlaves(i),
            -- Status Interface
            statusClk  => axilClk,
            statusRst  => axilRst,
            frameRate  => ibFrameRate(i),
            bandwidth  => ibBandwidth(i));
      -------------------------------               
      -- Outbound AXIS Monitor Module
      -------------------------------               
      U_Mon1 : entity work.AxiStreamMon
         generic map (
            TPD_G           => TPD_G,
            AXIS_CLK_FREQ_G => AXI_CLK_FREQ_G,
            AXIS_CONFIG_G   => RCEG3_AXIS_DMA_CONFIG_C)
         port map (
            -- AXIS Stream Interface
            axisClk    => axilClk,
            axisRst    => axilRst,
            axisMaster => obHlsMasters(i),
            axisSlave  => slaves(i),
            -- Status Interface
            statusClk  => axilClk,
            statusRst  => axilRst,
            frameRate  => obFrameRate(i),
            bandwidth  => obBandwidth(i));

      hlsMasters(i)  <= obHlsMasters(i) when(r.blowoff(i) = '0') else SSI_MASTER_FORCE_EOFE_C;
      slaves(i)      <= hlsSlaves(i)    when(r.blowoff(i) = '0') else AXI_STREAM_SLAVE_FORCE_C;
      obHlsSlaves(i) <= slaves(i);

   end generate GEN_LINK;

   comb : process (axilReadMaster, axilRst, axilWriteMaster, ibBandwidth,
                   ibFrameRate, ibHlsMasters, ibHlsSlaves, obBandwidth,
                   obFrameRate, obHlsMasters, r, slaves) is
      variable v      : RegType;
      variable regCon : AxiLiteEndPointType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      v.hardRst := '0';

      -- Check for hard reset
      if (r.hardRst = '1') then
         -- Reset the register
         v := REG_INIT_C;
      end if;

      -- Determine the transaction type
      axiSlaveWaitTxn(regCon, axilWriteMaster, axilReadMaster, v.axilWriteSlave, v.axilReadSlave);

      -- -- Map the read registers
      for i in (WIB_SIZE_C-1) downto 0 loop
         axiSlaveRegisterR(regCon, toSlv((24*i)+0, 12), 0, ibFrameRate(i));
         axiSlaveRegisterR(regCon, toSlv((24*i)+4, 12), 0, ibBandwidth(i));
         axiSlaveRegisterR(regCon, toSlv((24*i)+12, 12), 0, obFrameRate(i));
         axiSlaveRegisterR(regCon, toSlv((24*i)+16, 12), 0, obBandwidth(i));
      end loop;
      axiSlaveRegisterR(regCon, x"400", 0, ibHlsMasters(0).tValid);
      axiSlaveRegisterR(regCon, x"400", 1, ibHlsMasters(1).tValid);
      axiSlaveRegisterR(regCon, x"400", 2, ibHlsSlaves(0).tReady);
      axiSlaveRegisterR(regCon, x"400", 3, ibHlsSlaves(1).tReady);
      axiSlaveRegisterR(regCon, x"400", 4, obHlsMasters(0).tValid);
      axiSlaveRegisterR(regCon, x"400", 5, obHlsMasters(1).tValid);
      axiSlaveRegisterR(regCon, x"400", 6, slaves(0).tReady);
      axiSlaveRegisterR(regCon, x"400", 7, slaves(1).tReady);

      -- Map the write registers
      axiSlaveRegister(regCon, x"800", 0, v.blowoff);
      axiSlaveRegister(regCon, x"FFC", 0, v.hardRst);

      -- Closeout the transaction
      axiSlaveDefault(regCon, v.axilWriteSlave, v.axilReadSlave, AXI_RESP_DECERR_C);

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

end rtl;
