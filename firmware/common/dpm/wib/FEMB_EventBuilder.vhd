library ieee;
use ieee.std_logic_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_misc.all;
use work.WIB_Constants.all;
use work.Convert_IO.all;
use work.EB_IO.all;
use work.COLDATA_IO.all;
use work.types.all;
use work.FEMB_Gearbox_constants.all;
use work.FEMB_GB_IO.all;

entity FEMB_EventBuilder is
  generic (
    OUTPUT_BYTE_WIDTH : integer := 4;
    FIBER_ID          : integer := 0
    );
  port (
    clk          : in  std_logic;
    reset        : in  std_logic;
    convert      : in  convert_t;
    FEMB_stream  : in  FEMB_stream_array_t(3 downto 0);
    FEMB_read    : out std_logic_vector( 3 downto  0);
    data_out     : out std_logic_vector(31 downto  0);
    data_k_out   : out std_logic_vector( 3 downto  0);
    monitor      : out FEMB_EB_Monitor_t;
    control      : in  FEMB_EB_Control_t
    );

end entity FEMB_EventBuilder;

architecture behavioral of FEMB_EventBuilder is

  component EthernetCRCD32 is
    port (
      clk      : in  STD_LOGIC;
      init     : in  STD_LOGIC;
      ce       : in  STD_LOGIC;
      d        : in  STD_LOGIC_VECTOR (31 downto 0);
      byte_cnt : in  STD_LOGIC_VECTOR (1 downto 0);
      crc      : out STD_LOGIC_VECTOR (31 downto 0);
      bad_crc  : out STD_LOGIC);
  end component EthernetCRCD32;

  component RCE_FIFO is
    port (
      data    : in  std_logic_vector (35 downto 0);
      rdclk   : in  std_logic;
      rdreq   : in  std_logic;
      wrclk   : in  std_logic;
      wrreq   : in  std_logic;
      q       : out std_logic_vector (35 downto 0);
      rdempty : out std_logic;
      wrfull  : out std_logic);
  end component RCE_FIFO;

  component RCE_SPY_BUFFER is
    port (
      data    : IN  STD_LOGIC_VECTOR (35 DOWNTO 0);
      rdclk   : IN  STD_LOGIC;
      rdreq   : IN  STD_LOGIC;
      wrclk   : IN  STD_LOGIC;
      wrreq   : IN  STD_LOGIC;
      q       : OUT STD_LOGIC_VECTOR (35 DOWNTO 0);
      rdempty : OUT STD_LOGIC;
      wrfull  : OUT STD_LOGIC);
  end component RCE_SPY_BUFFER;

  component FEMB_Gearbox is
    generic (
      WORD_SIZE     : integer;
      DEFAULT_WORDS : std_logic_vector);
    port (
      clk                : in  std_logic;
      reset              : in  std_logic;
      data_in            : in  std_logic_vector(WORD_SIZE * WORD_COUNT_MAX - 1 downto 0);
      data_in_valid      : in  std_logic_vector(WORD_COUNT_MAX - 1 downto 0);
      data_out           : out std_logic_vector(WORD_SIZE*WORD_COUNT - 1 downto 0);
      empty_word_request : out std_logic;
      monitor            : out FEMB_GB_Monitor_t;
      control            : in  FEMB_GB_Control_t);
  end component FEMB_Gearbox;
  
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

  constant DAQ_BYTE_ALIGN_CHAR : std_logic_vector(7 downto 0) := x"5C";
  constant DAQ_COMMA_CHAR      : std_logic_vector(7 downto 0) := x"3C";

  -------------------------------------------------------------------------------
  -- state machine
  -------------------------------------------------------------------------------
  type EB_state_t is (EB_STATE_INIT_WAIT,  -- init state
                      EB_STATE_NEW_FRAME,  -- starting a new frame
                      EB_STATE_SKIP_WORD,  -- skip a word write for the gearbox
                      EB_STATE_SEND_H1,    -- build and send header 1&2
                      EB_STATE_SEND_H3,    -- build and send header 3&4
                      EB_STATE_SEND_H5,    -- build and send header 5&6
                      EB_STATE_SEND_H7,    -- build and send header 7&8
                      EB_STATE_SEND_CDA1_H,     -- Send COLDATA chip 1 header +
                                                -- 1 data word
                      EB_STATE_SEND_CDA1_DATA,  -- Send COLDATA chip 1 data
                      EB_STATE_SEND_CDA2_H,     -- Send COLDATA chip 1 header
                      EB_STATE_SEND_CDA2_DATA,  -- Send COLDATA chip 1 data
                      WAIT_S,              -- CRC Engine's pipeline delay
                      EB_STATE_SEND_CRC1,  -- Send frame CRC
                      EB_STATE_SEND_CRC2   -- Send frame CRC
                      );
  signal EB_state             : EB_state_t                   := EB_STATE_INIT_WAIT;
  constant DAQ_SOF            : std_logic_vector(7 downto 0) := x"BC";
  constant ERR_ALIGNMENT      : integer                      := 8;
  constant ERR_CD_CAPTURE     : integer                      := 0;
  constant ERR_CD_DATA_ERRORS : integer                      := 4;

  -------------------------------------------------------------------------------
  -- counter increment signals
  -------------------------------------------------------------------------------
  signal new_event : std_logic := '0';

  -------------------------------------------------------------------------------
  --remappings
  -------------------------------------------------------------------------------
  signal frame_valid          : std_logic_vector(LINKS_PER_FEMB-1 downto 0) := (others => '0');
  signal stream_convert_count : uint16_array_t(0 to CDAS_PER_FEMB-1)        := (others => (others => '0'));

  -------------------------------------------------------------------------------
  -- other signals
  -------------------------------------------------------------------------------  
  signal data_out_8b         : std_logic_vector(47 downto 0)    := DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                                                                   DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                                                                   DAQ_COMMA_CHAR & DAQ_COMMA_CHAR;
  signal data_out_8b_delay      : std_logic_vector(47 downto 0) := DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                                                                   DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                                                                   DAQ_COMMA_CHAR & DAQ_COMMA_CHAR;
  signal data_k_out_8b          : std_logic_vector(5 downto 0)  := "111111";
  signal data_k_out_8b_delay       : std_logic_vector(5 downto 0)  := "111111";
  signal data_valid          : std_logic_vector(2 downto 0)  := "000";
  signal data_valid_delay       : std_logic_vector(2 downto 0)  := "000";

  signal debug_mode_data : std_logic_vector(31 downto 0) := x"030201bc";
  
  signal crc_input_data : std_logic_vector(31 downto 0) := (others => '0');
  
  signal CD_odd_data_words : std_logic_vector(15 downto 0) := (others => '0');
  
  signal header_errors       : std_logic_vector(15 downto 0) := x"0000";
  signal header_fiber_number : std_logic_vector(1 downto 0)  := "00";
  signal header_slot_number  : std_logic_vector(2 downto 0)  := "000";
  signal header_crate_number : std_logic_vector(4 downto 0)  := "00000";

  constant CDA_FIFO_START : integer range 1 to to_integer(FRAME_SIZE/2) := to_integer(FRAME_SIZE/2);
  signal CDA_send_counter : integer range 1 to to_integer(FRAME_SIZE/2) := CDA_FIFO_START;

  signal empty_line_request : std_logic := '0';
  

--  signal word_32_valid : std_logic := '0';
  signal data_out_32   : std_logic_vector((OUTPUT_BYTE_WIDTH*9) -1 downto 0);

  signal monitor_buffer : FEMB_EB_Monitor_t;

  signal gearbox_input  : std_logic_vector(18 * WORD_COUNT_MAX - 1 downto 0);
  signal gearbox_output : std_logic_vector(18 * WORD_COUNT - 1 downto 0);
  

  -------------------------------------------------------------------------------
  -- spy buffer signals
  -------------------------------------------------------------------------------
  type spy_buffer_state_t is (SPY_BUFFER_STATE_IDLE,
                              SPY_BUFFER_STATE_WAIT,
                              SPY_BUFFER_STATE_CAPTURE);
  signal spy_buffer_state : spy_buffer_state_t := SPY_BUFFER_STATE_IDLE;
  signal spy_buffer_write_enable : std_logic := '0';
  signal spy_buffer_full : std_logic := '0';
  signal spy_buffer_write : std_logic := '0';
  -------------------------------------------------------------------------------
  -- crc signals
  -------------------------------------------------------------------------------
  signal data_crc    : std_logic_vector(31 downto 0) := (others => '1');
  signal crc_reset   : std_logic                     := '1';
  signal crc_process : std_logic                     := '0';
  signal bad_crc     : std_logic                     := '0';
  signal crc_bytes : std_logic_vector(1 downto 0) := "00";
  
  signal convert_info : convert_t;



begin

  monitor_buffer.debug <= control.debug;

  monitor_buffer.fiber_number <= std_logic_vector(to_unsigned(FIBER_ID, 2));
  monitor_buffer.COLDATA_en   <= control.COLDATA_en;

  -- re-mapping variables
  FEMBLinks : for iLink in 0 to LINKS_PER_FEMB-1 generate
    frame_valid(iLink) <= FEMB_stream(iLink).valid;
  end generate FEMBLinks;
  CDLinks : for iCD in 0 to CDAS_PER_FEMB-1 generate
    stream_convert_count(iCD)(7 downto 0)  <= FEMB_stream(iCD*LINKS_PER_CDA + 0).CD_timestamp;
    stream_convert_count(iCD)(15 downto 8) <= FEMB_stream(iCD*LINKS_PER_CDA + 1).CD_timestamp;
  end generate CDLinks;

  -------------------------------------------------------------------------------
  -- Generate the 8b byte stream for the next convert frame
  -------------------------------------------------------------------------------
  EVB : process (clk, reset) is
  begin  -- process EVB
    if reset = '1' then                 -- asynchronous reset (active high)
      data_out_8b   <= DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR;
      data_out_8b_delay <= DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR;
      data_k_out_8b    <= (others => '1');
      data_k_out_8b_delay <= (others => '1');

      crc_reset     <= '1';
      crc_process   <= '0';
      crc_bytes     <= "00";
      EB_state      <= EB_STATE_INIT_WAIT;
      FEMB_read     <= x"0";
--      word_32_valid <= '0';
    elsif clk'event and clk = '1' then  -- rising clock edge
      new_event   <= '0';
      --The default idle pattern for data out.
      data_out_8b   <= DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR &
                       DAQ_COMMA_CHAR & DAQ_COMMA_CHAR;
      data_k_out_8b    <= (others => '1');
      data_valid    <= (others => '0');
      
      data_out_8b_delay <= data_out_8b;
      data_k_out_8b_delay  <= data_k_out_8b;
      data_valid_delay  <= data_valid;
      
      --Don't reset the crc value and don't add current data to the crc
      crc_reset   <= '0';
      crc_process <= '0';
      crc_bytes   <= "00";
      
      --Don't read out any FEMB streams
      FEMB_read <= x"0";

      -- Setup the streaming to 32 for PCS
--      word_32_valid <= not(word_32_valid);
      
      case EB_state is
        -------------------------------------------------------------------------
        -- Wait on a convert signal to begin sending data
        -------------------------------------------------------------------------
        when EB_STATE_INIT_WAIT =>
          -- wait for alignment...                    
          -- we need to check from 
          EB_state <= EB_STATE_NEW_FRAME;
        when EB_STATE_NEW_FRAME =>
          --debug mode reset
          -- DEBUG MODE
          debug_mode_data <= x"030201bc";

          
          ---------------------------------------------------
          -- SPECIAL CODE FOR PIPLELINE DELAYING FOR CRC
          ---------------------------------------------------
          -- As a hold over from the last event, we have to override the
          -- normal delay of data_out -> data_out_delay because the CRC takes
          -- an extra step. We are now overriding the junk we put in data_out
          -- last tick and updating it with the correct crc data.
          data_out_8b_delay(31 downto 0) <=  data_crc(31 downto 0);
--          data_out_8b_delay(31 downto 0) <=  data_crc( 7 downto  0) &
--                                             data_crc(15 downto  8) &
--                                             data_crc(23 downto 16) &
--                                             data_crc(31 downto 24);--data_crc(31 downto 0);
          data_k_out_8b_delay(3 downto 0)  <= x"0";
          data_valid_delay  <= "011";
          ---------------------------------------------------

          --Back to normal state machine processing on non-delayed signals
          
          -- We should have a new convert signal now, so lets go
          if (frame_valid /= control.COLDATA_en) then
          --flag error
          end if;

          
          -- We are now sending an event (increment a counter)
          new_event <= '1';

          --capture WIB info
          header_fiber_number <= std_logic_vector(to_unsigned(FIBER_ID, 2));
          header_slot_number  <= convert.slot_id;
          header_crate_number <= convert.crate_id;         
          -- reset the CRC value
          crc_reset <= '1';
          -- cache the info about this trigger
          convert_info <= convert;
          
          
          -- Check if the gear box wants an empty word
          if empty_line_request = '0' then           
            --Send out idle lock-on pattern
            data_out_8b   <= DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR &
                             DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR &
                             DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR;
            data_k_out_8b <= "111111";
            data_valid    <= "111";            
            EB_state <= EB_STATE_SEND_H1;            
          else
            -- We were asked to skip a word, so send nothing
            -- We are effectively going from the gearbox 7-word state to the
            -- 5-word state.  in EB_STATE_SKIP_WORD, we'll add 2+1 words, so
            -- we'll move to the 6-word state.
            data_valid <= "000";
            EB_state <= EB_STATE_SKIP_WORD;            
          end if;
          
          
          
          
        -------------------------------------------------------------------------
        -- Wait for each COLDATA stream for this FEMB to arrive and then start
        -- sending data header
        -------------------------------------------------------------------------
        when EB_STATE_SKIP_WORD =>
          --Now that we sent a skip word last time, send 3 words of idle pattern
          --Send out idle lock-on pattern
          data_out_8b   <= DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR &
                           DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR &
                           DAQ_BYTE_ALIGN_CHAR & DAQ_COMMA_CHAR;
          data_k_out_8b <= "111111";
          data_valid    <= "111";            
          EB_State <= EB_STATE_SEND_H1;
        when EB_STATE_SEND_H1 =>
          -- Start sending the next event

          --Header word 1
          data_out_8b( 7 downto 0) <= DAQ_SOF;
          data_out_8b(15 downto 8) <= std_logic_vector(convert_info.reset_count(7 downto 0));
          data_k_out_8b ( 1 downto 0) <= "01";
          
          --header word 2
          -- Send remaining bits of reset counter
          data_out_8b(31 downto 16) <= std_logic_vector(convert_info.reset_count(23 downto 8));
          data_k_out_8b ( 3 downto  2) <= "00";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data <= x"07060504";
          end if;

          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";
          
          -- Move to send header word 3 & 4 state
          EB_state <= EB_STATE_SEND_H3;
          
          -- check convert numbers
          for iCD in 0 to CDAS_PER_FEMB-1 loop

            --check convert count for event mismatch
            if stream_convert_count(iCD) = std_logic_vector(convert.convert_count) then
              header_errors(ERR_ALIGNMENT + iCD) <= '0';
            else
              header_errors(ERR_ALIGNMENT + iCD) <= '1';
            end if;

            --check for capture errors
            header_errors(ERR_CD_CAPTURE + LINKS_PER_CDA*iCD + 0)     <= or_reduce(FEMB_stream((iCD*2) + 0).capture_errors);
            header_errors(ERR_CD_CAPTURE + LINKS_PER_CDA*iCD + 1)     <= or_reduce(FEMB_stream((iCD*2) + 1).capture_errors);
            --check for COLDATA ASIC errors
            header_errors(ERR_CD_DATA_ERRORS + LINKS_PER_CDA*iCD + 0) <= or_reduce(FEMB_stream((iCD*2) + 0).capture_errors);
            header_errors(ERR_CD_DATA_ERRORS + LINKS_PER_CDA*iCD + 1) <= or_reduce(FEMB_stream((iCD*2) + 1).capture_errors);

          end loop;  -- iCD            
        -------------------------------------------------------------------------
        when EB_STATE_SEND_H3 =>

          -- Header word 3
          -- Send convert counter
          data_out_8b(15 downto  0) <= std_logic_vector(convert_info.convert_count);
          data_k_out_8b ( 1 downto  0) <= "00";

          -- Header word 4
          -- Error bits
          data_out_8b(31 downto 16) <= header_errors;
          data_k_out_8b ( 3 downto  2) <= "00";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;

          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          -- Move to send header word 5&6 state
          EB_state <= EB_STATE_SEND_H5;
          
        -------------------------------------------------------------------------
        when EB_STATE_SEND_H5 =>
          -- Header words 5 & 6
          -- Timestamp
          data_out_8b  (31 downto 0) <= std_logic_vector(convert_info.time_stamp);
          data_k_out_8b( 3 downto 0)  <= x"0";

          data_valid <= "011";

                    -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;

          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          -- Move to send header word 7 &8 state
          EB_state <= EB_STATE_SEND_H7;

          -- Start the readout pipe-line for COLDATA ASIC 1
          FEMB_read <= x"3";

        -------------------------------------------------------------------------
        when EB_STATE_SEND_H7 =>

          -- Header word 7
          -- FEMB info
          data_out_8b(15 downto 10) <= (others => '0');
          data_out_8b( 9 downto  8)   <= header_fiber_number;
          data_out_8b( 7 downto  5)   <= header_slot_number;
          data_out_8b( 4 downto  0)   <= header_crate_number;
          data_k_out_8b( 1 downto  0)   <= "00";

          --Header word 8
          data_out_8b(31 downto 16) <= (others => '0');
          data_k_out_8b( 3 downto  2) <= "00";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;

          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          -- Move to send header word 3 state
          EB_state <= EB_STATE_SEND_CDA1_H;

        -------------------------------------------------------------------------
        -- Start sending data for COLDATA ASIC 1
        -------------------------------------------------------------------------
        when EB_STATE_SEND_CDA1_H =>
          
          -- CD header word
          data_out_8b(15 downto  8) <= FEMB_stream(1).capture_errors;
          data_out_8b( 7 downto  0) <= FEMB_stream(0).capture_errors;
          data_k_out_8b ( 1 downto  0) <= "00";

          -- First word of CD data
          data_out_8b(31 downto 16) <= FEMB_stream(1).data_out( 7 downto  0) & FEMB_stream(0).data_out( 7 downto  0);
          --cache the next word from each CD chip to be sent next time
          CD_odd_data_words         <= FEMB_stream(1).data_out(15 downto  8) & FEMB_stream(0).data_out(15 downto  8);
          
          data_k_out_8b ( 3 downto  2) <= "00";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;

          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          --Setup counter for the rest of this stream
          CDA_send_counter <= CDA_FIFO_START;
          --Stream out the rest of the data
          EB_state         <= EB_STATE_SEND_CDA1_DATA;
          
        -------------------------------------------------------------------------
        when EB_STATE_SEND_CDA1_DATA =>

          data_out_8b(15 downto  0) <= CD_odd_data_words; --words from last time
          data_out_8b(31 downto 16) <= FEMB_stream(1).data_out( 7 downto  0) & FEMB_stream(0).data_out( 7 downto  0);          
          --cache the next word from each CD chip to be sent next time
          CD_odd_data_words         <= FEMB_stream(1).data_out(15 downto  8) & FEMB_stream(0).data_out(15 downto  8);

          data_k_out_8b(3 downto 0)  <= x"0";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;
          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          --Send data counter
          CDA_send_counter <= CDA_send_counter -1;
          if CDA_send_counter = 2 then
            --Start readout of second COLDATA ASIC
            FEMB_read <= x"C";            
          elsif CDA_send_counter = 1 then
            EB_state  <= EB_STATE_SEND_CDA2_H;
          end if;
        -------------------------------------------------------------------------
        -- Start sending data for COLDATA ASIC 2
        -------------------------------------------------------------------------
        when EB_STATE_SEND_CDA2_H =>
          
          -- CD header word
          data_out_8b(15 downto  8) <= FEMB_stream(3).capture_errors;
          data_out_8b( 7 downto  0) <= FEMB_stream(2).capture_errors;
          data_k_out_8b ( 1 downto  0) <= "00";

          -- First word of CD data
          data_out_8b(31 downto 16) <= FEMB_stream(3).data_out( 7 downto  0) & FEMB_stream(2).data_out( 7 downto  0);
          --cache the next word from each CD chip to be sent next time
          CD_odd_data_words         <= FEMB_stream(3).data_out(15 downto  8) & FEMB_stream(2).data_out(15 downto  8);
          data_k_out_8b ( 3 downto  2) <= "00";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;
          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          --Setup counter for the rest of this stream
          CDA_send_counter <= CDA_FIFO_START;
          --Stream out the rest of the data
          EB_state         <= EB_STATE_SEND_CDA2_DATA;

        -------------------------------------------------------------------------
        when EB_STATE_SEND_CDA2_DATA =>
          data_out_8b(15 downto  0) <= CD_odd_data_words; --words from last time
          data_out_8b(31 downto 16) <= FEMB_stream(3).data_out( 7 downto  0) & FEMB_stream(2).data_out( 7 downto  0);          
          --cache the next word from each CD chip to be sent next time
          CD_odd_data_words         <= FEMB_stream(3).data_out(15 downto  8) & FEMB_stream(2).data_out(15 downto  8);

          data_k_out_8b(3 downto 0)  <= x"0";

          data_valid <= "011";

          -- DEBUG MODE
          if control.debug = '1' then
            data_out_8b(31 downto 0) <= debug_mode_data;
            debug_mode_data( 7 downto  0) <= std_logic_vector(unsigned(debug_mode_data( 7 downto  0)) + 4);
            debug_mode_data(15 downto  8) <= std_logic_vector(unsigned(debug_mode_data(15 downto  8)) + 4);
            debug_mode_data(23 downto 16) <= std_logic_vector(unsigned(debug_mode_data(23 downto 16)) + 4);
            debug_mode_data(31 downto 24) <= std_logic_vector(unsigned(debug_mode_data(31 downto 24)) + 4);
          end if;
          
          -- add data to the CRC
          crc_process <= '1';
          crc_bytes <= "00"; --"11";

          --Send data counter
          CDA_send_counter <= CDA_send_counter -1;
          if CDA_send_counter = 1 then
            EB_state <= WAIT_S;
          end if;

        -------------------------------------------------------------------------
        -- Start sending data for COLDATA ASIC 1
        -------------------------------------------------------------------------
        when WAIT_S =>
          EB_State       <= EB_STATE_NEW_FRAME;
--          EB_State       <= EB_STATE_SEND_CRC1;
        --------------------------------------------------------------------------
--        when EB_STATE_SEND_CRC1 =>
--          data_out_8b_delay(31 downto 0) <= data_crc(31 downto 0);
--          data_k_out_8b_delay(3 downto 0)  <= x"0";
--          data_valid_delay  <= "011";
--          EB_State       <= EB_STATE_NEW_FRAME;

        when others => EB_State <= EB_STATE_INIT_WAIT;
      end case;
    end if;
  end process EVB;

  -------------------------------------------------------------------------------
  -- Generate the CRC for the output data
  -------------------------------------------------------------------------------
--  crc_input_data(31 downto 24) <= data_out_8b( 7 downto  0);
--  crc_input_data(23 downto 16) <= data_out_8b(15 downto  8);
--  crc_input_data(15 downto  8) <= data_out_8b(23 downto 16);
--  crc_input_data( 7 downto  0) <= data_out_8b(31 downto 24);
  
--  crc_input_data(31 downto 24) <= data_out_8b(15 downto  8);
--  crc_input_data(23 downto 16) <= data_out_8b( 7 downto  0);
--  crc_input_data(15 downto  8) <= data_out_8b(31 downto 24);
--  crc_input_data( 7 downto  0) <= data_out_8b(23 downto 16);
  
--  crc_input_data(31 downto 24) <= data_out_8b(23 downto  16);
--  crc_input_data(23 downto 16) <= data_out_8b(31 downto  24);
--  crc_input_data(15 downto  8) <= data_out_8b( 7 downto  0 );
--  crc_input_data( 7 downto  0) <= data_out_8b(15 downto  8 );
  
  crc_input_data(31 downto 24) <= data_out_8b(31 downto  24);
  crc_input_data(23 downto 16) <= data_out_8b(23 downto  16);
  crc_input_data(15 downto  8) <= data_out_8b(15 downto   8);
  crc_input_data( 7 downto  0) <= data_out_8b( 7 downto   0);

  EthernetCRCD32_1 : entity work.EthernetCRCD32
    port map (
      clk     => clk,
      init    => crc_reset,
      ce      => crc_process,
      d       => crc_input_data,
--      d       => data_out_8b(31 downto 0),
      byte_cnt => crc_bytes,
      crc     => data_crc,
      bad_crc => bad_crc);


  gearbox_input <= data_k_out_8b_delay(5) & data_out_8b_delay(47 downto 40) &
                   data_k_out_8b_delay(4) & data_out_8b_delay(39 downto 32) &
                   data_k_out_8b_delay(3) & data_out_8b_delay(31 downto 24) &
                   data_k_out_8b_delay(2) & data_out_8b_delay(23 downto 16) &
                   data_k_out_8b_delay(1) & data_out_8b_delay(15 downto  8) &
                   data_k_out_8b_delay(0) & data_out_8b_delay( 7 downto  0);
  FEMB_Gearbox_2: entity work.FEMB_Gearbox
    generic map (
      WORD_SIZE     => 18,
      DEFAULT_WORDS => "1"&x"3C"&"1"&x"3C")
    port map (
      clk                => clk,
      reset              => reset,
      data_in            => gearbox_input,
      data_in_valid      => data_valid_delay,
      data_out           => gearbox_output,
      empty_word_request => empty_line_request,
      monitor            => monitor_buffer.gearbox,
      control            => control.gearbox);
  data_out   <= gearbox_output(34 downto 27) &
                gearbox_output(25 downto 18) &
                gearbox_output(16 downto  9) &
                gearbox_output( 7 downto  0);
  data_k_out <= gearbox_output(35) &
                gearbox_output(26) &
                gearbox_output(17) &
                gearbox_output( 8);
  
-------------------------------------------------------------------------------
-- Spy buffer
-------------------------------------------------------------------------------
  spy_buffer_control: process (clk, reset) is
  begin  -- process spy_buffer_control
    if reset = '1' then                 -- asynchronous reset (active high)
      spy_buffer_state <= SPY_BUFFER_STATE_IDLE;
      monitor_buffer.spy_buffer_running <= '0';
      spy_buffer_write_enable <= '0';      
    elsif clk'event and clk = '1' then  -- rising clock edge
      --Some state variable defaults
      monitor_buffer.spy_buffer_running <= '1';
      spy_buffer_write_enable <= '0';
      monitor_buffer.spy_buffer_wait_for_trigger <= control.spy_buffer_wait_for_trigger;
      
      if control.spy_buffer_start = '1' then
        --When a 
        spy_buffer_state <= SPY_BUFFER_STATE_WAIT;
      else        
        case spy_buffer_state is          
          when  SPY_BUFFER_STATE_IDLE =>
            --stay in idle state and report the spy buffer isn't running
            spy_buffer_state <= SPY_BUFFER_STATE_IDLE;
            monitor_buffer.spy_buffer_running <= '0';
          when SPY_BUFFER_STATE_WAIT  =>
            -- Wait in this state until something happens
            spy_buffer_state <= SPY_BUFFER_STATE_WAIT;
            if control.spy_buffer_wait_for_trigger = '1' then
              -- if we are waiting for a trigger, wait here until one happens            
              if convert.trigger = '1' then
                spy_buffer_state <= SPY_BUFFER_STATE_CAPTURE;
              end if;
            else
              -- start capturing data right away
              spy_buffer_state <= SPY_BUFFER_STATE_CAPTURE;
            end if;
          when SPY_BUFFER_STATE_CAPTURE =>
            if spy_buffer_full = '1' then
              --We are done capturing, so go back to IDLE
              spy_buffer_state <= SPY_BUFFER_STATE_IDLE;
            else
              spy_buffer_write_enable <= '1';
            end if;
          when others => spy_buffer_state <= SPY_BUFFER_STATE_IDLE;
        end case;
      end if;
      
    end if;
  end process spy_buffer_control;

  --  contorl spy buffer with the same signal that controls the serdes fifo
  RCE_SPY_BUFFER_1: RCE_SPY_BUFFER
    port map (
      data    => gearbox_output,
      rdclk   => clk,
      rdreq   => control.spy_buffer_read,
      wrclk   => clk,
      wrreq   => spy_buffer_write_enable,
      q       => monitor_buffer.spy_buffer_data,
      rdempty => monitor_buffer.spy_buffer_empty,
      wrfull  => spy_buffer_full);
  
-------------------------------------------------------------------------------
-- Counters
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
  delay_counter : process (clk) is
  begin  -- process delay_counter
    if clk'event and clk = '1' then     -- rising clock edge
      monitor <= monitor_buffer;
    end if;
  end process delay_counter;
  counter_1 : entity work.counter
    port map (
      clk         => clk,
      reset_async => reset,
      reset_sync  => control.event_count_reset,
      enable      => '1',
      event       => new_event,
      count       => monitor_buffer.event_count,
      at_max      => open);


end architecture behavioral;
