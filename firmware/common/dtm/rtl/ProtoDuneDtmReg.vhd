-------------------------------------------------------------------------------
-- Title      : 
-------------------------------------------------------------------------------
-- File       : ProtoDuneDtmReg.vhd
-- Author     : Larry Ruckman  <ruckman@slac.stanford.edu>
-- Company    : SLAC National Accelerator Laboratory
-- Created    : 2016-10-28
-- Last update: 2016-10-31
-- Platform   : 
-- Standard   : VHDL'93/02
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
use work.ProtoDuneDtmPkg.all;

entity ProtoDuneDtmReg is
   generic (
      TPD_G            : time            := 1 ns;
      AXI_ERROR_RESP_G : slv(1 downto 0) := AXI_RESP_DECERR_C);
   port (
      -- Status/Configuration Interface
      status          : in  ProtoDuneDtmStatusType;
      config          : out ProtoDuneDtmConfigType;
      -- AXI-Lite Interface
      axilClk         : in  sl;
      axilRst         : in  sl;
      axilReadMaster  : in  AxiLiteReadMasterType;
      axilReadSlave   : out AxiLiteReadSlaveType;
      axilWriteMaster : in  AxiLiteWriteMasterType;
      axilWriteSlave  : out AxiLiteWriteSlaveType);
end ProtoDuneDtmReg;

architecture rtl of ProtoDuneDtmReg is

   constant STATUS_SIZE_C : positive := 9;

   type RegType is record
      cntRst         : sl;
      rollOverEn     : slv(STATUS_SIZE_C-1 downto 0);
      config         : ProtoDuneDtmConfigType;
      axilReadSlave  : AxiLiteReadSlaveType;
      axilWriteSlave : AxiLiteWriteSlaveType;
   end record;

   constant REG_INIT_C : RegType := (
      cntRst         => '1',
      rollOverEn     => (others => '0'),
      config         => PROTO_DUNE_DTM_CONFIG_INIT_C,
      axilReadSlave  => AXI_LITE_READ_SLAVE_INIT_C,
      axilWriteSlave => AXI_LITE_WRITE_SLAVE_INIT_C);

   signal r   : RegType := REG_INIT_C;
   signal rin : RegType;

   signal statusOut : slv(STATUS_SIZE_C-1 downto 0);
   signal statusCnt : SlVectorArray(STATUS_SIZE_C-1 downto 0, 31 downto 0);

   -- attribute dont_touch               : string;
   -- attribute dont_touch of r          : signal is "TRUE";
   
begin

   comb : process (axilReadMaster, axilRst, axilWriteMaster, r, statusCnt, statusOut) is
      variable v      : RegType;
      variable regCon : AxiLiteEndPointType;
   begin
      -- Latch the current value
      v := r;

      -- Reset the strobes
      v.cntRst := '0';

      -- Determine the transaction type
      axiSlaveWaitTxn(regCon, axilWriteMaster, axilReadMaster, v.axilWriteSlave, v.axilReadSlave);

      -- Map the read registers
      for i in STATUS_SIZE_C-1 downto 0 loop
         axiSlaveRegisterR(regCon, toSlv((4*i), 12), 0, muxSlVectorArray(statusCnt, i));
      end loop;
      axiSlaveRegisterR(regCon, x"400", 0, statusOut);

      -- Map the write registers
      axiSlaveRegister(regCon, x"800", 0, v.config.busyMask);
      axiSlaveRegister(regCon, x"804", 0, v.config.forceBusy);
      axiSlaveRegister(regCon, x"900", 0, v.config.emuTimingSel);
      axiSlaveRegister(regCon, x"904", 0, v.config.emuClkSel);
      axiSlaveRegister(regCon, x"FF0", 0, v.config.softRst);
      axiSlaveRegister(regCon, x"FF4", 0, v.config.hardRst);
      axiSlaveRegister(regCon, x"FF8", 0, v.rollOverEn);
      axiSlaveRegister(regCon, x"FFC", 0, v.cntRst);

      -- Closeout the transaction
      axiSlaveDefault(regCon, v.axilWriteSlave, v.axilReadSlave, AXI_ERROR_RESP_G);

      -- Synchronous Reset
      if (axilRst = '1') then
         v := REG_INIT_C;
      end if;

      -- Register the variable for next clock cycle
      rin <= v;

      -- Outputs
      axilWriteSlave <= r.axilWriteSlave;
      axilReadSlave  <= r.axilReadSlave;
      config         <= r.config;
      
   end process comb;

   seq : process (axilClk) is
   begin
      if (rising_edge(axilClk)) then
         r <= rin after TPD_G;
      end if;
   end process seq;

   U_SyncStatusVector : entity work.SyncStatusVector
      generic map (
         TPD_G          => TPD_G,
         OUT_POLARITY_G => '1',
         CNT_RST_EDGE_G => false,
         CNT_WIDTH_G    => 32,
         WIDTH_G        => STATUS_SIZE_C)     
      port map (
         -- Input Status bit Signals (wrClk domain)
         statusIn(8)          => status.busyOut,
         statusIn(7 downto 0) => status.busyVec,
         -- Output Status bit Signals (rdClk domain)  
         statusOut            => statusOut,
         -- Status Bit Counters Signals (rdClk domain) 
         cntRstIn             => r.cntRst,
         rollOverEnIn         => r.rollOverEn,
         cntOut               => statusCnt,
         -- Clocks and Reset Ports
         wrClk                => axilClk,
         rdClk                => axilClk);      

end rtl;