----------------------------------------------------------------------------------
-- Company: Boston University EDF
-- Engineer: Dan Gastler
--
-- package for interfaces with the WIB Event builder
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.numeric_std.all;

package DQM_PACKET is

  
  type DQM_packet_t is record
    system_status : std_logic_vector(31 downto 0);
    header_user_info : std_logic_vector(63 downto 0);
    fifo_data : std_logic_vector(15 downto 0);
    fifo_wr   : std_logic;
  end record DQM_packet_t;
  constant DEFAULT_DQM_PACKET : DQM_packet_t := (system_status => x"00000000",
                                                 header_user_info => x"0000000000000000",
                                                 fifo_data => x"0000",
                                                 fifo_wr => '0');
  
  
end DQM_PACKET;
