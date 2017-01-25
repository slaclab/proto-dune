--------------------------------------------------------------------------------
-- Copyright (C) 1999-2008 Easics NV.
-- This source file may be used and distributed without restriction
-- provided that this copyright statement is not removed from the file
-- and that any derivative work contains the original copyright notice
-- and the associated disclaimer.
--
-- THIS SOURCE FILE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS
-- OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
-- WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
--
-- Purpose : synthesizable CRC function
--   * polynomial: (0 1 2 4 5 7 8 10 11 12 16 22 23 26 32)
--   * data width: 8
--
-- Info : tools@easics.be
--        http://www.easics.com
--------------------------------------------------------------------------------
----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    13:20:34 12/01/2011 
-- Design Name: 
-- Module Name:    EthernetCRC - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER  
-- This module is provided only as an example, no correctness or any usefullness is implied.
-- Use of it is at users' own risk. 
-- Do not remove this disclaimer.
-- !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER !!!DISCLAIMER  
-- Description: Ethernet CRC calculation, derived from PCK_CRC32_D8 generated using easics.com tools
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity EthernetCRCD16B is
  Port ( clk : in  STD_LOGIC;
         init : in  STD_LOGIC;
         ce : in  STD_LOGIC;
         d : in  STD_LOGIC_VECTOR (15 downto 0);
         crc : out  STD_LOGIC_VECTOR (31 downto 0);
         bad_crc : out  STD_LOGIC
         );
end EthernetCRCD16B;

architecture Behavioral of EthernetCRCD16B is
  constant crc_R : std_logic_vector(31 downto 0) := x"c704dd7b";
  signal c : std_logic_vector(31 downto 0) := (others => '0');
  signal di : std_logic_vector(15 downto 0) := (others => '0');
begin
  process(c,d)
  begin
    for i in 0 to 31 loop
      crc(i) <= not c(31-i);
    end loop;
    for i in 0 to 15 loop
      di(i) <= d(15-i);
    end loop;
  end process;
  bad_crc <= '0' when c = crc_R else '1';
  process(clk)
  begin
    if(clk'event and clk = '1')then      
      if(init = '1')then
        c <= (others => '1');
      elsif(ce = '1')then
        c(0)  <= di(12) xor di(10) xor di(9)  xor di(6) xor di(0) xor c(16) xor c(22) xor c(25) xor c(26) xor c(28);
        c(1)  <= di(13) xor di(12) xor di(11) xor di(9) xor di(7) xor di(6) xor di(1) xor di(0) xor c(16) xor c(17) xor c(22) xor c(23) xor c(25) xor c(27) xor c(28) xor c(29);
        c(2)  <= di(14) xor di(13) xor di(9)  xor di(8) xor di(7) xor di(6) xor di(2) xor di(1) xor di(0) xor c(16) xor c(17) xor c(18) xor c(22) xor c(23) xor c(24) xor c(25) xor c(29) xor c(30);
        c(3)  <= di(15) xor di(14) xor di(10) xor di(9) xor di(8) xor di(7) xor di(3) xor di(2) xor di(1) xor c(17) xor c(18) xor c(19) xor c(23) xor c(24) xor c(25) xor c(26) xor c(30) xor c(31);
        c(4)  <= di(15) xor di(12) xor di(11) xor di(8) xor di(6) xor di(4) xor di(3) xor di(2) xor di(0) xor c(16) xor c(18) xor c(19) xor c(20) xor c(22) xor c(24) xor c(27) xor c(28) xor c(31);
        c(5)  <= di(13) xor di(10) xor di(7)  xor di(6) xor di(5) xor di(4) xor di(3) xor di(1) xor di(0) xor c(16) xor c(17) xor c(19) xor c(20) xor c(21) xor c(22) xor c(23) xor c(26) xor c(29);
        c(6)  <= di(14) xor di(11) xor di(8)  xor di(7) xor di(6) xor di(5) xor di(4) xor di(2) xor di(1) xor c(17) xor c(18) xor c(20) xor c(21) xor c(22) xor c(23) xor c(24) xor c(27) xor c(30);
        c(7)  <= di(15) xor di(10) xor di(8)  xor di(7) xor di(5) xor di(3) xor di(2) xor di(0) xor c(16) xor c(18) xor c(19) xor c(21) xor c(23) xor c(24) xor c(26) xor c(31);
        c(8)  <= di(12) xor di(11) xor di(10) xor di(8) xor di(4) xor di(3) xor di(1) xor di(0) xor c(16) xor c(17) xor c(19) xor c(20) xor c(24) xor c(26) xor c(27) xor c(28);
        c(9)  <= di(13) xor di(12) xor di(11) xor di(9) xor di(5) xor di(4) xor di(2) xor di(1) xor c(17) xor c(18) xor c(20) xor c(21) xor c(25) xor c(27) xor c(28) xor c(29);
        c(10) <= di(14) xor di(13) xor di(9)  xor di(5) xor di(3) xor di(2) xor di(0) xor c(16) xor c(18) xor c(19) xor c(21) xor c(25) xor c(29) xor c(30);
        c(11) <= di(15) xor di(14) xor di(12) xor di(9) xor di(4) xor di(3) xor di(1) xor di(0) xor c(16) xor c(17) xor c(19) xor c(20) xor c(25) xor c(28) xor c(30) xor c(31);
        c(12) <= di(15) xor di(13) xor di(12) xor di(9) xor di(6) xor di(5) xor di(4) xor di(2) xor di(1) xor di(0) xor c(16) xor c(17) xor c(18) xor c(20) xor c(21) xor c(22) xor c(25) xor c(28) xor c(29) xor c(31);
        c(13) <= di(14) xor di(13) xor di(10) xor di(7) xor di(6) xor di(5) xor di(3) xor di(2) xor di(1) xor c(17) xor c(18) xor c(19) xor c(21) xor c(22) xor c(23) xor c(26) xor c(29) xor c(30);
        c(14) <= di(15) xor di(14) xor di(11) xor di(8) xor di(7) xor di(6) xor di(4) xor di(3) xor di(2) xor c(18) xor c(19) xor c(20) xor c(22) xor c(23) xor c(24) xor c(27) xor c(30) xor c(31);
        c(15) <= di(15) xor di(12) xor di(9)  xor di(8) xor di(7) xor di(5) xor di(4) xor di(3) xor c(19) xor c(20) xor c(21) xor c(23) xor c(24) xor c(25) xor c(28) xor c(31);
        c(16) <= di(13) xor di(12) xor di(8)  xor di(5) xor di(4) xor di(0) xor c(0)  xor c(16) xor c(20) xor c(21) xor c(24) xor c(28) xor c(29);
        c(17) <= di(14) xor di(13) xor di(9)  xor di(6) xor di(5) xor di(1) xor c(1)  xor c(17) xor c(21) xor c(22) xor c(25) xor c(29) xor c(30);
        c(18) <= di(15) xor di(14) xor di(10) xor di(7) xor di(6) xor di(2) xor c(2)  xor c(18) xor c(22) xor c(23) xor c(26) xor c(30) xor c(31);
        c(19) <= di(15) xor di(11) xor di(8)  xor di(7) xor di(3) xor c(3)  xor c(19) xor c(23) xor c(24) xor c(27) xor c(31);
        c(20) <= di(12) xor di(9)  xor di(8)  xor di(4) xor c(4)  xor c(20) xor c(24) xor c(25) xor c(28);
        c(21) <= di(13) xor di(10) xor di(9)  xor di(5) xor c(5)  xor c(21) xor c(25) xor c(26) xor c(29);
        c(22) <= di(14) xor di(12) xor di(11) xor di(9) xor di(0) xor c(6)  xor c(16) xor c(25) xor c(27) xor c(28) xor c(30);
        c(23) <= di(15) xor di(13) xor di(9)  xor di(6) xor di(1) xor di(0) xor c(7)  xor c(16) xor c(17) xor c(22) xor c(25) xor c(29) xor c(31);
        c(24) <= di(14) xor di(10) xor di(7)  xor di(2) xor di(1) xor c(8)  xor c(17) xor c(18) xor c(23) xor c(26) xor c(30);
        c(25) <= di(15) xor di(11) xor di(8)  xor di(3) xor di(2) xor c(9)  xor c(18) xor c(19) xor c(24) xor c(27) xor c(31);
        c(26) <= di(10) xor di(6)  xor di(4)  xor di(3) xor di(0) xor c(10) xor c(16) xor c(19) xor c(20) xor c(22) xor c(26);
        c(27) <= di(11) xor di(7)  xor di(5)  xor di(4) xor di(1) xor c(11) xor c(17) xor c(20) xor c(21) xor c(23) xor c(27);
        c(28) <= di(12) xor di(8)  xor di(6)  xor di(5) xor di(2) xor c(12) xor c(18) xor c(21) xor c(22) xor c(24) xor c(28);
        c(29) <= di(13) xor di(9)  xor di(7)  xor di(6) xor di(3) xor c(13) xor c(19) xor c(22) xor c(23) xor c(25) xor c(29);
        c(30) <= di(14) xor di(10) xor di(8)  xor di(7) xor di(4) xor c(14) xor c(20) xor c(23) xor c(24) xor c(26) xor c(30);
        c(31) <= di(15) xor di(11) xor di(9)  xor di(8) xor di(5) xor c(15) xor c(21) xor c(24) xor c(25) xor c(27) xor c(31);
      end if;
    end if;
  end process;

end Behavioral;

