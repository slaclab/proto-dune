# Define target output
target: bit

# Define target part
export PRJ_PART = XC7Z045FFG900-2

# define firmware version
export PRJ_VERSION = 0x00000058

#       DATE    VERSION WHO  DESCRITPTION
# ---------- ---------- ---  -------------------------------------------------
# 2018/05/24 0x00000058 llr  Adding DMA[0].TDEST[3] loopback path
# 2018/05/23 0x00000057 llr  Adding SRPv3 interface to ProtoDuneDpmEth for debugging RSSI
# 2018/05/18 0x00000056 llr  Updating to latest SURF (v1.8.4), which has a RSSI bug fix
# 2018/04/06 0x00000055 llr  Updating to latest SURF (v1.7.4), which has a RSSI bug fix
# 2018/03/20 0x00000054 llr  Hard-coding timing trigger detect to op-code [0x8:0xF] (v4a5 tag)
# 2018/03/15 0x00000053 llr  Updating to timing v4 protocol (v4a5 tag)
# 2018/03/08 0x00000052 llr  Updating to timing v4 protocol (v4a4 tag)
# 2018/02/10 0x00000051 llr  Fix missing SOF to the HLS module (ESDUNE-38)
# 2018/02/09 0x00000050 llr  Reverting the 64B/66B mods in surf/GTX7
# 2018/02/08 0x00000049 llr  Rebuilding with latest version of surf/ruckus
# 2017/07/24 0x00000048 llr  Preventing the timingRst from locking up the CPLL
# 2017/06/05 0x00000047 llr  Integrated timing receiver into data emulator
#                            Now the data emulator will send the actual timestamp
# 2017/06/02 0x00000046 llr  Moved TimingMsg from DMA[CH=2] to DMA[CH=0]
# 2017/05/31 0x00000045 llr  Added TimingRstCnt register
# 2017/05/22 0x00000044 llr  Bug fix in ProtoDuneDpmTimingMsg.vhd
# 2017/05/18 0x00000043 llr  Updating to CERN timing receiver V3C
# 2017/05/11 0x00000042 llr  Added the timing stream to the inbound DMA
# 2017/05/10 0x00000041 jjr  Believe this Ryan's attempt to correcct a bug
#                            which resulted in the length of a packet being
#                            returned 1 byte larger than it should have.
# 2017/05/09 0x00000040 jjr  Ryan's attemtp to fix the drop packet problem. 
#                            This correction addresses the handling of the
#                            cache properties, both on the main DMA data path
#                            and the descriptor path.
# 2017/05/05 0x00000039 jjr  Larry simplified the monitoring done by ILA
#                            (See comment on 0x38.)
# 2017/05/04 0x00000038 jjr  Much more extensive monitoring added by Ryan
#                            -- This failed.  Larry believes it had something
#                            to do with the attempt to monitor both the 125
#                            and 200 MHz clock domains.
# 2017/05/04 0x00000037 jjr  Add ILA for both HLS modules
# 2017/05/04 0x00000036 jjr  Small SsiFifo + ILA 
# 2017/05/04 0x00000035 jjr  Larry thinks that we need a small SsiFifo between
#                            the HLS module and the 'large' fifo to filter out
#                            'garbage' seen at a power cycle.
# 2017/05/03 0x00000034 jjr  Back to using the SsiFifo. This works as 
#                            anticipated, being basically back 0x30
# 2017/05/03 0x00000033 jjr  Back to full output. This was to be an attempt
#                            to use ILA as a monitor, but that failed when
#                            the driver would not load.  Rebuild without ILA.
#                            This also hung the processor. (I did not properly
#                            label the initial build with ILA, it was versioned
#                            as 32. But since it did not work, it will never
#                            be used again.
# 2017/05/03 0x00000032 jjr  Neuter the output to verify that the back-pressure
#                            is coming from the output stream not accepting
#                            data.
# 2017/05/02 0x00000031 jjr  Next attempt. -- Failed, no obvious difference
#                            from the 0x30.
# 2017/05/02 0x00000030 jjr  Using new firmware from Ryan that addresses the
#                            the dropped WibFrame issue.  I think the idea
#                            is that there was contention when the 2 FIFO
#                            that interface directly with the HLS modules
#                            merged into 1 FIFO that feeds the DMA engine.
#                            This fix has been tested in a standalone fashion
#                            by Ryan. 
#
#                            NOTE: Did not work. It dropped frames even with
#                            only one enabled HLS module.  Frames did not
#                            appear to dropped, but corrupted.  In particular
#                            the WIB 64-bit timestamp was in error (sometimes
#                            even 0), but the Convert Counts matched. Had not
#                            seen the pattern till now.
# 2017/04/25 0x0000002F jjr  Larry eliminated the cascading FIFO in the DMA
#                            path.  There was a worry that the way it was
#                            configured may cause small timescale studders.
# 2017/04/25 0x0000002E jjr  Increase the number of DMA Channel 2 buffers from
#                            4 -> 8. This is to see if this cures the sporadic
#                            dropped WIB frames.  (Currently with both HLS
#                            modules enabled, the drop rate is ~4-10 /sec.
#                            Did not bump the PRJ_VERSION, so produced a 
#                            second version 2E.  It was better, but not a
#                            complete success
# 2017/04/21 0x0000002E jjr  Try version of DMA driver with multiple 
#                            independent FIFO's. This is an attempt to up
#                            the thru-put.
# 2017/04/18 0x0000002D jjr  Incorportates Ryan's  V2 rce_stream driver which
#                            uses the ACP rather than HP as the DMA path 
# 2017/03/22 0x0000002C llr  Added the first draft of the CERN Timing receiver
# 2017/03/01 0x0000002B rth  Added register settable cache setting for DMA
# 2017/01/31 0x0000002A llr  Added 1 second linkUP watchdog resets to WIB links
# 2017/01/19 0x00000029 llr  Added OneShotWibCapture command
# 2017/01/18 0x00000028 llr  Fixed a bug in WIB EMU when "SendCntData" = true
# 2017/01/17 0x00000027 llr  Reset rxLinkUp's cnt if rxBufStatus(2) = '1'
# 2017/01/06 0x00000026 llr  Added WIB DbgCrcRcv/DbgCrcCalc registers 
# 2016/12/04 0x00000025 llr  For WIB Error Frame capture module, set undefined to 0xEE
# 2016/12/02 0x00000024 llr  Added WIB SOF detected rate status register
#                            Added WIB Error Frame capture module
# 2016/12/02 0x00000023 llr  Added WIB paket rate (pre-SSI filter) status register
#                            Change GT from LPM mode to DFE mode
#                            Change the EMU TX txPreCursor default from 0xF to 0x0
# 2016/12/01 0x00000022 llr  Added ErrGtRxBuffer status register
#                            Fixed a bug in the GT's clock correction
# 2016/12/01 0x00000021 llr  Added PktLen and MinPktLen status registers
#                            Fixed a bug in WIB RX when EOFE detect and tLast
#                            Added GT reset command in WIB RX
# 2016/11/29 0x00000020 jjr  In both computing the CRC in the emulation path
#                            and checking it in receive path, the WIB CRC
#                            calculations are being used.  This should ensure
#                            the CRC calculation is being both computed and 
#                            checked in the same way.
# 2016/11/29 0x0000001F jjr  Larry added a filtering FIFO so that JJ's HLS
#                            module should always see SOF on the first word,
#                            i.e. it does frame synchronization and filtering
#                            of incorrectly formatted frames.
#                            HLS module now checks and counts that SOF and EOF
#                            are seen only in the proper words.
# 2016/11/28 0x0000001E jjr  Larry increased the size of a buffering FIFO and
#                            added diagnostic counters to check for overflow
#                            of that FIFO
# 2016/11/18 0x0000001D jjr  Add diagnostic counters to see if the input
#                            AXI stream may not be empty on startup. This
#                            would result in the WIB frames not being synched
#                            correctly.
# 2016/11/02 0x0000001B jjr  Try to fix failure to increment the write count,
#                            but failed.  Rebuild with the latest from Larry
# 2016/11/02 0x0000001A llr  Added localMac & localIp status registers to UdpWrapper
# 2016/11/01 0x00000019 llr  Added TxPolarity registers
# 2016/11/01 0x00000018 llr  Added SendCntData, CPllLock, RxBufStatus,
#                            RxPolarity registers
# 2016/11/01 0x00000017 llr  Added the RSSI channel to ART-DAQ board reader
# 2016/10/27 0x00000017 jjr  Fix to DMA engine that should prevent hang-ups
#                            when stopping.  The underlying reason is that
#                            the buffering FIFO hangs on to the memory
#                            bus when it is partially filled and wouldn't let
#                            go until it does fill or last is seen.
#                            -- Somehow got a duplicate ---
# 2016/10/25 0000000016 jjr  A DATAFLOW implementation. This may have a chance.
#                            There are FIFO's between the read and write
#                            methods.
# 2016/10/22 0000000015 jjr  A serial implementation.  This is not 100%
#                            correct, but maybe it will result in something
#                            useable to commission the WIB.
#                            To first order, this works as expected.
# 2016/10/22 0000000014 jjr  Combined read status with temporary buffer.
#                            This adds some flow control to the read/copy
#                            but not all that I expect.
#                            Failed, no data at all was promoted out of the
#                            HLS module
# 2016/10/22 0000000013 jjr  Try promoting some data. The interface does not
#                            look quite right, but try anyway.
#                            Does not keep up
# 2016/10/22 0000000012 jjr  Change sAxis, mAxis to hls::stream. Still dummy
#                            write routine.
#                            Keeps up, data looks okay, although it only one
#                            64-bit word per frame
# 2016/10/21 0x00000011 jjr  Back to dummy write routine
#                            This keeps up, but does nothing but drain the
#                            input stream
# 2016/10/21 0x00000010 jjr  Changed buffering array to an hls_stream.
#                            Failed to keep up
# 2016/10/21 0x0000000F jjr  Added the actual copyFrame back in.  Eliminated
#                            depth parameters on the in/out AXI streams.
#                            Explicitly declared the moduleIdx to be ap_none
#                            so that it will not get inferred as a FIFO, which
#                            we suspect may have been blocking the copyFrame
#                            from running.
#                            This version failed to keep up
# 2016/10/21 0x0000000E jjr  Cutting scope. Kill the output routine. Also
#                            moved the copy of the config block to a separate
#                            method.  Having this done inline may have affected
#                            the dataflow implementation.
#                            Had to add a dummy output method, else Vivado
#                            errors on not using mAxis.
#                            Note: First build failed on timing constraint
# 2016/10/21 0x0000000D jjr  Buffering into DuneDataCompressionCore HLS
#                            module was increased. CASCADE_SIZE_G from 2 to 4.
# 2016/10/20 0x0000000C jjr  Simplified code. Last version resulted in no
#                            data being promoted out of the HLS module. Also
#                            made the ModuleConfig init and mode fields to be
#                            uint32_t to make it easier to set from the
#                            processor.
# 2016/10/20 0x0000000B jjr  Attermpt to fix the thru-put problem.  Previous
#                            version were only running at ~1744 Hz, not 1952.
#                            Don't have a lot of confidence. While the timing
#                            reported during synthesis has change (now 40/40
#                            for latency and iteration, was 74/40) the reports
#                            seem to indicate streaming behaviour between the
#                            readFrame and copyFrame methods which is not right
# 2016/10/19 0x0000000A jjr  Add more debugging (timestamp check) and
#                            configuration/flow control
# 2016/10/18 0x00000009 jjr  Tried to fix the dataflow and updating of the
#                            status registers. 
# 2016/10/13 0x00000008 jjr  Add monitoring status variables.  The is reason
#                            to believe that this version does not correctly
#                            transport data, but will get the monitoring
#                            in place, then attack that problem later.
# 2016/10/12 0x00000007 jjr  Attempt to add a flush mechanism to prevent a
#                            partial packet from hanging the RCE.  This
#                            involved changes to both the HLS code and the
#                            jacketing .vhd code to implement and support
#                            this register.
# 2016/09/17 0x00000006 jjr  Added ability to blowoff either WIB source
#                            Allowing both simoulaneously exceeds the
#                            available bandwidth
# 2016/09/16 0x00000005 jjr  Larry restored filter module in DMA path
# 2016/09/15 0x00000004 jjr  Correct last and user setting in output axis
# 2016/09/13 0x00000003 jjr  Copy WIB frame version
# 2016/09/01 0x00000002 llr  Added ProtoDuneDpmHlsMon
# 07/22/2016 0x00000001 llr  Initial Version
#

# Use top level makefile
include ../../submodules/ruckus/system_vivado.mk
