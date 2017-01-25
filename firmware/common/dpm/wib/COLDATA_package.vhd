----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package constants of the COLDATA format
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;


package COLDATA_IO is

  --Special data format characters
  constant IDLE_CHARACTER            : std_logic_vector(8 downto 0) := '1'&x"3C";  --K.28.1
  constant SOF_CHARACTER             : std_logic_vector(8 downto 0) := '1'&x"BC";  --K.28.5
                                                                                   
  
  constant FRAME_SIZE                : unsigned(5 downto 0) := to_unsigned(55,6);
  
  constant ADDR_COLDATA_CHECKSUM_LSB : unsigned(5 downto 0) := to_unsigned(0,6);
  constant ADDR_COLDATA_CHECKSUM_MSB : unsigned(5 downto 0) := to_unsigned(1,6);
  constant ADDR_COLDATA_TIME         : unsigned(5 downto 0) := to_unsigned(2,6);
  constant ADDR_COLDATA_ERRORS       : unsigned(5 downto 0) := to_unsigned(3,6);
  constant ADDR_COLDATA_RESERVED     : unsigned(5 downto 0) := to_unsigned(4,6);
  constant ADDR_COLDATA_HEADER1      : unsigned(5 downto 0) := to_unsigned(5,6);
  constant ADDR_COLDATA_HEADER2      : unsigned(5 downto 0) := to_unsigned(6,6);
  constant ADDR_COLDATA_STREAM1      : unsigned(5 downto 0) := to_unsigned(7,6);
  constant ADDR_COLDATA_STREAM2      : unsigned(5 downto 0) := to_unsigned(13,6);
  constant ADDR_COLDATA_STREAM3      : unsigned(5 downto 0) := to_unsigned(19,6);
  constant ADDR_COLDATA_STREAM4      : unsigned(5 downto 0) := to_unsigned(25,6);
  constant ADDR_COLDATA_STREAM5      : unsigned(5 downto 0) := to_unsigned(31,6);
  constant ADDR_COLDATA_STREAM6      : unsigned(5 downto 0) := to_unsigned(37,6);
  constant ADDR_COLDATA_STREAM7      : unsigned(5 downto 0) := to_unsigned(43,6);
  constant ADDR_COLDATA_STREAM8      : unsigned(5 downto 0) := to_unsigned(49,6);  
  
end COLDATA_IO;
