
library ieee;
use ieee.std_logic_1164.all;

entity RCE_FIFO is
   port (
      data    : in  std_logic_vector (35 downto 0);
      rdclk   : in  std_logic;
      rdreq   : in  std_logic;
      wrclk   : in  std_logic;
      wrreq   : in  std_logic;
      q       : out std_logic_vector (35 downto 0);
      rdempty : out std_logic;
      wrfull  : out std_logic);
end RCE_FIFO;

architecture mapping of RCE_FIFO is

begin

   U_FifoAsync : entity work.FifoAsync
      generic map (
         BRAM_EN_G    => false,
         FWFT_EN_G    => false,
         DATA_WIDTH_G => 36,
         ADDR_WIDTH_G => 4)
      port map (
         rst    => '0',
         wr_clk => wrclk,
         wr_en  => wrreq,
         din    => data,
         full   => wrfull,
         rd_clk => rdclk,
         rd_en  => rdreq,
         dout   => q,
         empty  => rdempty);

end architecture mapping;
