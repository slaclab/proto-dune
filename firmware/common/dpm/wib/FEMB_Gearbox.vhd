---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
-- Constants parameters for the FEMB_Gearbox
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
library ieee;
package FEMB_Gearbox_constants is
  constant WORD_COUNT     : integer := 2;
  constant WORD_COUNT_MAX : integer := 3;
end package FEMB_Gearbox_constants;

---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
--Monitor and control package
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
package FEMB_GB_IO is
  
  type FEMB_GB_Monitor_t is record
    sr_state_5_counter : unsigned(31 downto 0);
    sr_state_6_counter : unsigned(31 downto 0);
    sr_state_7_counter : unsigned(31 downto 0);
    reset_sr_state_5_counter  : std_logic;
    reset_sr_state_6_counter  : std_logic;
    reset_sr_state_7_counter  : std_logic;
    enable_sr_state_5_counter : std_logic;
    enable_sr_state_6_counter : std_logic;
    enable_sr_state_7_counter : std_logic;
  end record FEMB_GB_Monitor_t;
  
  type FEMB_GB_Control_t is record
    reset_sr_state_5_counter  : std_logic;
    reset_sr_state_6_counter  : std_logic;
    reset_sr_state_7_counter  : std_logic;
    enable_sr_state_5_counter : std_logic;
    enable_sr_state_6_counter : std_logic;
    enable_sr_state_7_counter : std_logic;
  end record FEMB_GB_Control_t;
  constant DEFAULT_FEMB_GB_CONTROL : FEMB_GB_Control_t := (reset_sr_state_5_counter  => '0',
                                                           reset_sr_state_6_counter  => '0',
                                                           reset_sr_state_7_counter  => '0',
                                                           enable_sr_state_5_counter => '0',
                                                           enable_sr_state_6_counter => '0',
                                                           enable_sr_state_7_counter => '0');



  
end package FEMB_GB_IO;

---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
-- FEMB_Gearbox
---------------------------------------------------------------------------------
---------------------------------------------------------------------------------
--This component handles converting a series of odd numbered WORD_SIZE words in
-- an event into blocks that must always be an even number of WORDS_SIZE words.
-- This is done by normally taking in 2 words every clock except at the end of
-- an event where there are 3 words.   This relies on the input stream to give
-- a clock tick of nothing every two 3 word inputs. 

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.types.all;
use work.FEMB_Gearbox_constants.all;
use work.FEMB_GB_IO.all;
entity FEMB_Gearbox is
  generic (
    WORD_SIZE      : integer := 16;
    DEFAULT_WORDS   : std_logic_vector := x"3C3C");
  port (
    clk                : in  std_logic;
    reset              : in  std_logic;
    data_in            : in  std_logic_vector(WORD_SIZE * WORD_COUNT_MAX - 1 downto 0);
    data_in_valid      : in  std_logic_vector(WORD_COUNT_MAX - 1 downto 0);
    data_out           : out std_logic_vector(WORD_SIZE*WORD_COUNT -1 downto 0);
    empty_word_request : out std_logic;
    monitor            : out FEMB_GB_Monitor_t;
    control            : in  FEMB_GB_Control_t);

end entity FEMB_Gearbox;

architecture behavioral of FEMB_Gearbox is
  --Error counter primative
  component counter is
    generic (
      roll_over   : std_logic;
      end_value   : std_logic_vector;
      start_value : std_logic_vector;
      DATA_WIDTH  : integer);
    port (
      clk         : in  std_logic;
      reset_async : in  std_logic;
      reset_sync  : in  std_logic;
      enable      : in  std_logic;
      event       : in  std_logic;
      count       : out unsigned(DATA_WIDTH-1 downto 0);
      at_max      : out std_logic);
  end component counter;
  
  type word_array_t is array (integer range <>) of std_logic_vector(WORD_SIZE -1 downto 0);
  
  -------------------------------------------------------------------------------
  -- state machine
  -------------------------------------------------------------------------------
  type shift_register_state_t is (SR_STATE_5,SR_STATE_6,SR_STATE_7);
  signal sr_state : shift_register_state_t := SR_STATE_6;
  
  -------------------------------------------------------------------------------
  -- Input rearrangement
  -------------------------------------------------------------------------------
  signal words_in : word_array_t(2 downto 0) := (others => DEFAULT_WORDS);

  -------------------------------------------------------------------------------
  -- Slipping gear box
  -------------------------------------------------------------------------------
  --Shift register with a constant number of words being pulled out.
  --This shift register has a variable number of words put in, but on average
  --it is the same as being pulled out and only differs by +1 or -2 for one
  --tick rarely over a long period
  signal word_sr : word_array_t(7 downto 0) := (others => DEFAULT_WORDS);

  -------------------------------------------------------------------------------
  -- Error pulses
  -------------------------------------------------------------------------------
  signal slip_state_error_5 : std_logic := '0';                    
  signal slip_state_error_6 : std_logic := '0';              
  signal slip_state_error_7 : std_logic := '0';              

  
  
begin  -- architecture behavioral  

  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  -- notify the user that we are in the deepest state and we'd like an empty word
  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  request_blank_word: process (clk, reset) is
  begin  -- process request_blank_word
    if reset = '1' then               -- asynchronous reset (active high)
      empty_word_request <= '0';
    elsif clk'event and clk = '1' then  -- rising clock edge
      if sr_state = SR_STATE_7 then
        empty_word_request <= '1';
      else
        empty_word_request <= '0';
      end if;
    end if;
  end process request_blank_word;

  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  -- Convert inputs std_logic_vector into an array of words
  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  input_type_rearrange: for iWord in WORD_COUNT_MAX -1  downto 0 generate
    words_in(iWord) <= data_in((WORD_SIZE*iWord) + WORD_SIZE -1 downto WORD_SIZE*iWord);
  end generate input_type_rearrange;
  
  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  --Main process, state machine and shift register operation
  -------------------------------------------------------------------------------
  -------------------------------------------------------------------------------
  shift_register_state_machine : process (clk, reset) is
  begin  -- process shift_register_state_machine
    if reset = '1' then                 -- asynchronous reset (active high)
      word_sr <= (others => DEFAULT_WORDS);
      sr_state <= SR_STATE_6;
      slip_state_error_5 <= '0';                    
      slip_state_error_6 <= '0';              
      slip_state_error_7 <= '0';              

    elsif clk'event and clk = '1' then  -- rising clock edge
      --Reset any pulses used in this process
      slip_state_error_5 <= '0';                    
      slip_state_error_6 <= '0';              
      slip_state_error_7 <= '0';              

      --State machine
      case sr_state is
        when SR_STATE_6 =>
          
          if data_in_valid = "111" then
            ---------------------------------------------------------------------
            -- Special event where we are getting 3 words in instead of 2
            word_sr(6 downto 4) <= words_in(2 downto 0);
            for iWord in 3 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord

            -- State changed to processing 7 deep
            sr_state <= SR_STATE_7;
            
          else
            ---------------------------------------------------------------------
            -- check for invalid input for this state, log any bad states and treat as "011" case
            if data_in_valid /= "011" then
              slip_state_error_6 <= '1';              
            end if;
            
            -- Normal adding of data
            word_sr(5 downto 4) <= words_in(1 downto 0);
            for iWord in 3 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord

            --State is unchanged
            sr_state <= SR_STATE_6;
            
          end if;
        when SR_STATE_7 =>
          if data_in_valid = "000" then           
            ---------------------------------------------------------------------
            -- Special event with no new data, shrink the depth of our shift register
            for iWord in 4 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord

            -- State changed to processing 5 deep
            sr_state <= SR_STATE_5;
            
          else
            ---------------------------------------------------------------------
            -- check for invalid input for this state, log any bad states and treat as "011" case
            if data_in_valid /= "011" then
              slip_state_error_7 <= '1';              
            end if;

            -- Normal adding of data
            word_sr(6 downto 5) <= words_in(1 downto 0);
            for iWord in 4 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord

            -- State is unchanged
            sr_state <= SR_STATE_7;
          end if;
        when SR_STATE_5 =>
          if data_in_valid = "111" then
            ---------------------------------------------------------------------
            -- Special event with 3 new words of data

            word_sr(5 downto 3) <= words_in(2 downto 0);
            for iWord in 2 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord

            -- State is changed to processing 6 deep
            sr_state <= SR_STATE_6;
          else
            ---------------------------------------------------------------------
            -- check for invalid input for this state, log any bad states and treat as "011" case
            if data_in_valid /= "011" then
              slip_state_error_5 <= '1';              
            end if;

            -- Normal adding of data
            word_sr(4 downto 3) <= words_in(1 downto 0);
            for iWord in 2 downto 0 loop
              word_sr(iWord) <= word_sr(iWord + 2);
            end loop;  -- iWord
            
            -- State is unchanged
            sr_state <= SR_STATE_5;
          end if;
        when others => null;
      end case;

      --Output the bottom of our shift register
      data_out   <= word_sr(1) & word_sr(0);
    end if;
  end process shift_register_state_machine;

-------------------------------------------------------------------------------
-- Counters
-------------------------------------------------------------------------------

  counter_sr5: entity work.counter
    generic map (
      roll_over   => '0')
    port map (
      clk         => clk,
      reset_async => reset,
      reset_sync  => control.reset_sr_state_5_counter,
      enable      => control.enable_sr_state_5_counter,
      event       => slip_state_error_5,
      count       => monitor.sr_state_5_counter,
      at_max      => open);
  monitor.reset_sr_state_5_counter <= control.reset_sr_state_5_counter;
  monitor.enable_sr_state_5_counter <= control.enable_sr_state_5_counter;
  counter_sr6: entity work.counter
    generic map (
      roll_over   => '0')
    port map (
      clk         => clk,
      reset_async => reset,
      reset_sync  => control.reset_sr_state_6_counter,
      enable      => control.enable_sr_state_6_counter,
      event       => slip_state_error_6,
      count       => monitor.sr_state_6_counter,
      at_max      => open);
  monitor.reset_sr_state_6_counter <= control.reset_sr_state_6_counter;
  monitor.enable_sr_state_6_counter <= control.enable_sr_state_6_counter;
  counter_sr7: entity work.counter
    generic map (
      roll_over   => '0')
    port map (
      clk         => clk,
      reset_async => reset,
      reset_sync  => control.reset_sr_state_7_counter,
      enable      => control.enable_sr_state_7_counter,
      event       => slip_state_error_7,
      count       => monitor.sr_state_7_counter,
      at_max      => open);
  monitor.reset_sr_state_7_counter <= control.reset_sr_state_7_counter;
  monitor.enable_sr_state_7_counter <= control.enable_sr_state_7_counter;

  
end architecture behavioral;


