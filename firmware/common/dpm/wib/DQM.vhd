library ieee;
use ieee.std_logic_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_misc.all;

use work.WIB_Constants.all;
use work.COLDATA_IO.all;
--
use work.FEMB_IO.all;
use work.Convert_IO.all;
--use work.EB_IO.all;
use work.types.all;
use work.DQM_Packet.all;
use work.DQM_IO.all;

entity DQM is
  port (
    clk_128Mhz : in std_logic;
    reset      : in std_logic;

    convert : in convert_t;

    packet_out  : out DQM_Packet_t;
    packet_free : in  std_logic;

    monitor  : out DQM_Monitor_t;
    control  : in  DQM_Control_t;
    FEMB_DQM : in  FEMB_DQM_t
    );

end entity DQM;

architecture behavioral of DQM is


  -------------------------------------------------------------------------------
  --components
  -------------------------------------------------------------------------------
  component DQM_FIFO is
    port (
      clock : in  std_logic;
      data  : in  std_logic_vector (15 downto 0);
      rdreq : in  std_logic;
      wrreq : in  std_logic;
      empty : out std_logic;
      full  : out std_logic;
      q     : out std_logic_vector (15 downto 0));
  end component DQM_FIFO;
  -------------------------------------------------------------------------------
  -- signals
  -------------------------------------------------------------------------------
  signal fifo_in  : std_logic_vector(15 downto 0) := (others => '0');
  signal fifo_out : std_logic_vector(15 downto 0) := (others => '0');
  signal fifo_rd  : std_logic                     := '0';
  signal fifo_wr  : std_logic                     := '0';

  type DQMS_state_t is (DQMS_S_IDLE, DQMS_S_WAIT_FOR_UDP_PACKET, DQMS_S_WAIT_FOR_TOSEND_PACKET, DQMS_S_STREAM);
  signal DQMS_state : DQMS_state_t := DQMS_S_IDLE;

  type DQMC_state_t is (DQMC_S_IDLE, DQMC_S_WAIT_FOR_EMPTY_PACKET, DQMC_S_START_PACKET,
                        DQMC_S_CD_SINGLESTREAM_START, DQMC_S_CD_SINGLESTREAM_WAIT, DQMC_S_CD_SINGLESTREAM_CAPTURE);
  signal DQMC_state : DQMC_state_t := DQMC_S_IDLE;

  signal write_index : integer range 1 downto 0 := 1;
  signal read_index  : integer range 1 downto 0 := 0;
  signal mem_switch  : std_logic                := '0';

  signal packet_system_status    : uint32_array_t(1 downto 0) := (others => (others => '0'));
  signal packet_header_user_info : uint64_array_t(1 downto 0) := (others => (others => '0'));
  signal packet_size             : uint16_array_t(1 downto 0) := (others => (others => '0'));
  signal size_left               : unsigned(15 downto 0)      := x"0000";

  signal stream_number : integer range LINK_COUNT downto 1;

begin  -- architecture behavioral

  monitor.enable_DQM <= control.enable_DQM;
  monitor.DQM_type   <= control.DQM_type;
  monitor.CD_SS      <= control.CD_SS;

  DQM_FIFO_1 : DQM_FIFO
    port map (
      clock => clk_128Mhz,
      data  => fifo_in,
      rdreq => fifo_rd,
      wrreq => fifo_wr,
      empty => open,
      full  => open,
      q     => fifo_out);

  DQM_send : process (clk_128Mhz, reset) is
  begin  -- process DQM_send
    if reset = '1' then                 -- asynchronous reset (active high)
      packet_out.fifo_wr <= '0';
    elsif clk_128Mhz'event and clk_128Mhz = '1' then  -- rising clock edge
      case DQMS_state is

        when DQMS_S_IDLE =>
          -- Idle state that waits for DQM to be enabled
          DQMS_state         <= DQMS_S_IDLE;
          packet_out.fifo_wr <= '0';

          --Check if we are sending DQM
          if control.enable_DQM = '1' then
            DQMS_state <= DQMS_S_WAIT_FOR_UDP_PACKET;
          end if;

        when DQMS_S_WAIT_FOR_UDP_PACKET =>
          if packet_free = '1' then
            DQMS_state <= DQMS_S_WAIT_FOR_TOSEND_PACKET;
          end if;
        when DQMS_S_WAIT_FOR_TOSEND_PACKET =>
          --Wait for the builder process to hand us a packet
          if mem_switch = '1' then
            --Update packet header info for UDP core 
            packet_out.system_status    <= packet_system_status(read_index);
            packet_out.header_user_info <= packet_header_user_info(read_index);

            --Begin streaming local FIFO to UDP core
            size_left  <= unsigned(packet_size(read_index))-1 ;
            fifo_rd    <= '1';
            DQMS_state <= DQMS_S_STREAM;
          end if;
        when DQMS_S_STREAM =>
          --Stream out data
          packet_out.fifo_data <= fifo_out;
          packet_out.fifo_wr   <= '1';

          --Wait for end of data.
          size_left <= size_left -1;
          if or_reduce(std_logic_vector(size_left)) = '0' then
            fifo_rd    <= '0';
            DQMS_state <= DQMS_S_IDLE;
          end if;
        when others => null;
      end case;
    end if;
  end process DQM_send;




  DQM_caputre_manager : process (clk_128Mhz, reset) is
  begin  -- process DQM_caputre_manager
    if reset = '1' then                 -- asynchronous reset (active high)
      write_index <= 1;
      read_index  <= 0;
    elsif clk_128Mhz'event and clk_128Mhz = '1' then  -- rising clock edge

      -- handle swapping of memories between capture and send
      mem_switch <= '0';
      if (DQMS_state = DQMS_S_WAIT_FOR_TOSEND_PACKET and
          DQMC_state = DQMC_S_WAIT_FOR_EMPTY_PACKET) then
        write_index      <= read_index;
        read_index       <= write_index;
        mem_switch <= '1';
      end if;

    end if;
  end process DQM_caputre_manager;

  DQM_capture : process (clk_128Mhz, reset) is
  begin  -- process DQM_capture
    if reset = '1' then                 -- asynchronous reset (active high)

    elsif clk_128Mhz'event and clk_128Mhz = '1' then  -- rising clock edge
      case DQMC_state is
        when DQMC_S_IDLE =>

          --Check if we are sending DQM
          if control.enable_DQM = '1' then
            DQMC_state <= DQMC_S_WAIT_FOR_EMPTY_PACKET;
          end if;
        when DQMC_S_WAIT_FOR_EMPTY_PACKET =>
          --Wait for the builder process to hand us a packet
          if mem_switch = '1' then
            DQMC_state <= DQMC_S_START_PACKET;
          end if;
        when DQMC_S_START_PACKET =>
          case control.DQM_type is
            when x"0" =>
              DQMC_state <= DQMC_S_CD_SINGLESTREAM_START;
            when others => null;
          end case;



        -------------------------------------------------------------------------
        -- COLDATA single stream capture mode begin
        -------------------------------------------------------------------------
        when DQMC_S_CD_SINGLESTREAM_START =>
          if convert.trigger = '1' then
            packet_system_status(write_index)(4 downto 0)  <= convert.crate_id;
            packet_system_status(write_index)(7 downto 5)  <= convert.slot_id;
            packet_system_status(write_index)(31 downto 8) <= std_logic_vector(convert.time_stamp(23 downto 0));

            packet_header_user_info(write_index)(15 downto 0)  <= std_logic_vector(convert.convert_count);
            packet_header_user_info(write_index)(19 downto 16) <= x"0";
            packet_header_user_info(write_index)(20)           <= control.CD_SS.stream_number;
            packet_header_user_info(write_index)(21)           <= control.CD_SS.CD_number;
            packet_header_user_info(write_index)(23 downto 22) <= control.CD_SS.FEMB_number;
            stream_number                                          <= to_integer(unsigned(control.CD_SS.FEMB_number & control.CD_SS.CD_number & control.CD_SS.stream_number)) + 1;

            DQMC_state <= DQMC_S_CD_SINGLESTREAM_WAIT;
          end if;
        when DQMC_S_CD_SINGLESTREAM_WAIT =>
          if FEMB_DQM.COLDATA_stream(stream_number) = '1'&x"3C" then
            fifo_wr                  <= '1';
            fifo_in                  <= "0000000" & FEMB_DQM.COLDATA_Stream(stream_number)(8 downto 0);
            packet_size(write_index) <= x"01";
            DQMC_state               <= DQMC_S_CD_SINGLESTREAM_CAPTURE;
          end if;
        when DQMC_S_CD_SINGLESTREAM_CAPTURE =>
          fifo_wr                  <= '1';
          fifo_in                  <= "0000000" & FEMB_DQM.COLDATA_Stream(stream_number)(8 downto 0);
          packet_size(write_index) <= std_logic_vector(unsigned(packet_size(write_index)) + 1);
          if (packet_size(write_index) = x"00"&"00"&std_logic_vector(FRAME_SIZE) or
              FEMB_DQM.COLDATA_Stream(stream_number)(8) = '1' ) then
            DQMC_state               <= DQMC_S_IDLE;
            fifo_wr                  <= '0';
            packet_size(write_index) <= packet_size(write_index);
          end if;
        -------------------------------------------------------------------------
        -- COLDATA single stream capture mode end
        -------------------------------------------------------------------------
        when others => null;
      end case;
    end if;
  end process DQM_capture;




end architecture behavioral;
