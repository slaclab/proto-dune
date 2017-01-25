library ieee;
use ieee.std_logic_1164.all;
-- Constants for this WIB design
package WIB_Constants is
  constant FEMB_COUNT     : integer := 4;  -- number of Front-End MotherBoards   
  constant CDAS_PER_FEMB  : integer := 2;  -- Number of COLDATA ASICS per FEMB
  constant LINKS_PER_CDA  : integer := 2;  -- Number of links per COLDATA ASIC
  constant LINKS_PER_FEMB : integer := CDAS_PER_FEMB * LINKS_PER_CDA;
  constant LINK_COUNT     : integer := FEMB_COUNT*LINKS_PER_FEMB;
  constant FW_VERSION     : std_logic_vector(31 downto 0) := x"17010801";
end package WIB_Constants;
