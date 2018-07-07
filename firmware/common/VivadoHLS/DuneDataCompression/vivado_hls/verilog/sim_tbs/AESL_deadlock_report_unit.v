`timescale 1 ns / 1 ps

module AESL_deadlock_report_unit #( parameter PROC_NUM = 4 ) (
    input reset,
    input clock,
    input [PROC_NUM - 1:0] dl_in_vec,
    output dl_detect_out,
    output reg [PROC_NUM - 1:0] origin,
    output token_clear);
   
    // FSM states
    localparam ST_IDLE = 2'b0;
    localparam ST_DL_DETECTED = 2'b1;
    localparam ST_DL_REPORT = 2'b10;

    reg [1:0] CS_fsm;
    reg [1:0] NS_fsm;
    reg [PROC_NUM - 1:0] dl_detect_reg;
    reg [PROC_NUM - 1:0] dl_done_reg;
    reg [PROC_NUM - 1:0] origin_reg;
    reg [PROC_NUM - 1:0] dl_in_vec_reg;
    integer i;

    // FSM State machine
    always @ (negedge reset or posedge clock) begin
        if (~reset) begin
            CS_fsm <= ST_IDLE;
        end
        else begin
            CS_fsm <= NS_fsm;
        end
    end
    always @ (CS_fsm or dl_in_vec or dl_detect_reg or dl_done_reg or dl_in_vec or origin_reg) begin
        NS_fsm = CS_fsm;
        case (CS_fsm)
            ST_IDLE : begin
                if (|dl_in_vec) begin
                    NS_fsm = ST_DL_DETECTED;
                end
            end
            ST_DL_DETECTED: begin
                // has unreported deadlock circle
                if (dl_detect_reg != dl_done_reg) begin
                    NS_fsm = ST_DL_REPORT;
                end
            end
            ST_DL_REPORT: begin
                if (|(dl_in_vec & origin_reg)) begin
                    NS_fsm = ST_DL_DETECTED;
                end
            end
        endcase
    end

    // dl_detect_reg record the procs that first detect deadlock
    always @ (negedge reset or posedge clock) begin
        if (~reset) begin
            dl_detect_reg <= 'b0;
        end
        else begin
            if (CS_fsm == ST_IDLE) begin
                dl_detect_reg <= dl_in_vec;
            end
        end
    end

    // dl_detect_out keeps in high after deadlock detected
    assign dl_detect_out = |dl_detect_reg;

    // dl_done_reg record the circles has been reported
    always @ (negedge reset or posedge clock) begin
        if (~reset) begin
            dl_done_reg <= 'b0;
        end
        else begin
            if ((CS_fsm == ST_DL_REPORT) && (|(dl_in_vec & dl_detect_reg) == 'b1)) begin
                dl_done_reg <= dl_done_reg | dl_in_vec;
            end
        end
    end

    // clear token once a circle is done
    assign token_clear = (CS_fsm == ST_DL_REPORT) ? ((|(dl_in_vec & origin_reg)) ? 'b1 : 'b0) : 'b0;

    // origin_reg record the current circle start id
    always @ (negedge reset or posedge clock) begin
        if (~reset) begin
            origin_reg <= 'b0;
        end
        else begin
            if (CS_fsm == ST_DL_DETECTED) begin
                origin_reg <= origin;
            end
        end
    end
   
    // origin will be valid for only one cycle
    always @ (CS_fsm or dl_detect_reg or dl_done_reg) begin
        origin = 'b0;
        if (CS_fsm == ST_DL_DETECTED) begin
            for (i = 0; i < PROC_NUM; i = i + 1) begin
                if (dl_detect_reg[i] & ~dl_done_reg[i] & ~(|origin)) begin
                    origin = 'b1 << i;
                end
            end
        end
    end
    
    // dl_in_vec_reg record the current circle dl_in_vec
    always @ (negedge reset or posedge clock) begin
        if (~reset) begin
            dl_in_vec_reg <= 'b0;
        end
        else begin
            if (CS_fsm == ST_DL_DETECTED) begin
                dl_in_vec_reg <= origin;
            end
            else if (CS_fsm == ST_DL_REPORT) begin
                dl_in_vec_reg <= dl_in_vec;
            end
        end
    end
    
    // get the first valid proc index in dl vector
    function integer proc_index(input [PROC_NUM - 1:0] dl_vec);
        begin
            proc_index = 0;
            for (i = 0; i < PROC_NUM; i = i + 1) begin
                if (dl_vec[i]) begin
                    proc_index = i;
                end
            end
        end
    endfunction

    // get the proc path based on dl vector
    function [1168:0] proc_path(input [PROC_NUM - 1:0] dl_vec);
        integer index;
        begin
            index = proc_index(dl_vec);
            case (index)
                0 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.read196_U0";
                end
                1 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0";
                end
                2 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0";
                end
                3 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0";
                end
                4 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0";
                end
                5 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0";
                end
                6 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0";
                end
                7 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0";
                end
                8 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0";
                end
                9 : begin
                    proc_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0";
                end
                default : begin
                    proc_path = "unknown";
                end
            endcase
        end
    endfunction

    // print the headlines of deadlock detection
    task print_dl_head;
        begin
            $display("\n//////////////////////////////////////////////////////////////////////////////");
            $display("// ERROR!!! DEADLOCK DETECTED at %0t ns! SIMULATION WILL BE STOPPED! //", $time);
            $display("//////////////////////////////////////////////////////////////////////////////");
        end
    endtask

    // print the start of a circle
    task print_circle_start(input reg [1168:0] proc_path, input integer circle_id);
        begin
            $display("/////////////////////////");
            $display("// Dependence circle %0d:", circle_id);
            $display("// (1): Process: %0s", proc_path);
        end
    endtask

    // print the end of deadlock detection
    task print_dl_end(input integer num);
        begin
            $display("////////////////////////////////////////////////////////////////////////");
            $display("// Totally %0d circles detected!", num);
            $display("////////////////////////////////////////////////////////////////////////");
        end
    endtask

    // print one proc component in the circle
    task print_circle_proc_comp(input reg [1168:0] proc_path, input integer circle_comp_id);
        begin
            $display("// (%0d): Process: %0s", circle_comp_id, proc_path);
        end
    endtask

    // print one channel component in the circle
    task print_circle_chan_comp(input [PROC_NUM - 1:0] dl_vec1, input [PROC_NUM - 1:0] dl_vec2);
        reg [1208:0] chan_path;
        integer index1;
        integer index2;
        begin
            index1 = proc_index(dl_vec1);
            index2 = proc_index(dl_vec2);
            case (index1)
                0 : begin
                    case(index2)
                    1: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.read196_U0.ap_done & deadlock_detector.ap_done_reg_1 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.read196_U0.ap_done & deadlock_detector.ap_done_reg_1 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.read196_U0.iframe_assign_c_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                1 : begin
                    case(index2)
                    0: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_0_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_dat_d_1_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_status_V_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_status_V_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_status_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_status_V_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.frame_m_status_V_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.iframe_assign_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.iframe_assign_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    2: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.i_full_n & AESL_inst_DuneDataCompressionCore.handle_packet_U0.acquire_packet_U0.dataflow_in_loop_ACQ_U0.process_frame_U0.ap_done & deadlock_detector.ap_done_reg_0 & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.t_read) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                2 : begin
                    case(index2)
                    1: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_chdx_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_chdx_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_chdx_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_chdx_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_chdx_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_cedx_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_cedx_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_cedx_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_cedx_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_cedx_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_status_V_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_status_V_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_status_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_status_V_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_status_V_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_hdrsBuf_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.pktCtx_m_excsBuf_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg0_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg1_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg2_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_0_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_1_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_2_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_adcs_sg3_3_V_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg0_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg1_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg2_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_s_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_0_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_1_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_2_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_3_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_4_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_5_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_6_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_7_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_16_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_17_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_18_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_19_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_20_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_21_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_22_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_23_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_24_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_25_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_26_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_27_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_28_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_29_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.t_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.i_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.t_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.cmpCtx_hists_sg3_m_b_30_U.i_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                3 : begin
                    case(index2)
                    4: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.adcs0_V_offset_c_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0_ap_ready_count[0]))) begin
                            chan_path = "";
                            if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0_ap_ready_count[0]))) begin
                                $display("//      Deadlocked by sync logic between input processes");
                                $display("//      Please increase channel depth");
                            end
                        end
                    end
                    9: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ichan_c_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.bAxis_m_cur_read_out_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.bAxis_m_idx_read_out_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    5: begin
                        if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0_ap_ready_count[0]))) begin
                            chan_path = "";
                            if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0_ap_ready_count[0]))) begin
                                $display("//      Deadlocked by sync logic between input processes");
                                $display("//      Please increase channel depth");
                            end
                        end
                    end
                    6: begin
                        if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0_ap_ready_count[0]))) begin
                            chan_path = "";
                            if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0_ap_ready_count[0]))) begin
                                $display("//      Deadlocked by sync logic between input processes");
                                $display("//      Please increase channel depth");
                            end
                        end
                    end
                    7: begin
                        if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0_ap_ready_count[0]))) begin
                            chan_path = "";
                            if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0_ap_ready_count[0]))) begin
                                $display("//      Deadlocked by sync logic between input processes");
                                $display("//      Please increase channel depth");
                            end
                        end
                    end
                    8: begin
                        if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0_ap_ready_count[0]))) begin
                            chan_path = "";
                            if (((AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0_ap_ready_count[0]) & AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.write_adcs4_entry205_U0.ap_idle & ~(AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0_ap_ready_count[0]))) begin
                                $display("//      Deadlocked by sync logic between input processes");
                                $display("//      Please increase channel depth");
                            end
                        end
                    end
                    endcase
                end
                4 : begin
                    case(index2)
                    3: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0.hists0_m_omask_V_offset_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.adcs0_V_offset_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    5: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0.hists0_m_omask_V_offset_out_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    6: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0.hists0_m_omask_V_offset_out1_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    7: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0.hists0_m_omask_V_offset_out2_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    8: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.encode4_entry199_U0.hists0_m_omask_V_offset_out3_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                5 : begin
                    case(index2)
                    9: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.etxOut_ha_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.etxOut_ba_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    4: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode141_U0.hist_m_omask_V_offset_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                6 : begin
                    case(index2)
                    9: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.etxOut_ha_m_buf7_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.etxOut_ba_m_buf16_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    4: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode142_U0.hist_m_omask_V_offset_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                7 : begin
                    case(index2)
                    9: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.etxOut_ha_m_buf8_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.etxOut_ba_m_buf17_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    4: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode143_U0.hist_m_omask_V_offset_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                8 : begin
                    case(index2)
                    9: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.etxOut_ha_m_buf9_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.etxOut_ba_m_buf18_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    4: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.APE_encode144_U0.hist_m_omask_V_offset_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.encode4_U0.hists0_m_omask_V_off_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
                9 : begin
                    case(index2)
                    3: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.bAxis_m_cur_read_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_cur_read_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.bAxis_m_idx_read_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.bAxis_m_idx_read_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ichan_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.ichan_c_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    5: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_3_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_3_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_2_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_2_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_0_ha_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_1_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_1_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_s_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_s_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_0_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_0_ba_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    6: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_3_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_3_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_2_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_2_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_1_ha_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_1_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_1_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_s_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_s_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_1_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_1_ba_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    7: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_3_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_3_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_2_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_2_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_2_ha_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_1_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_1_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_s_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_s_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_2_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_2_ba_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    8: begin
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_3_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_3_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_2_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_2_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_2_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_2_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_2_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_3_ha_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ha_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_1_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_1_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_1_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_1_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_1_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_s_U.if_empty_n & (AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_ready | AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.ap_idle) & ~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_s_U.if_write) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_s_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_s_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_3_s_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                        if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.writeN_U0.etx_3_ba_m_buf_blk_n) begin
                            chan_path = "DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U";
                            if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U.if_empty_n) begin
                                $display("//      Channel: %0s, EMPTY", chan_path);
                            end
                            else if (~AESL_inst_DuneDataCompressionCore.handle_packet_U0.process_packet_U0.grp_write_packet_fu_414.grp_write_adcs4_fu_689.container_etxOut_ba_3_U.if_full_n) begin
                                $display("//      Channel: %0s, FULL", chan_path);
                            end
                            else begin
                                $display("//      Channel: %0s", chan_path);
                            end
                        end
                    end
                    endcase
                end
            endcase
        end
    endtask

    // report
    initial begin : report_deadlock
        integer circle_id;
        integer circle_comp_id;
        wait (reset == 1);
        circle_id = 1;
        while (1) begin
            @ (negedge clock);
            case (CS_fsm)
                ST_DL_DETECTED: begin
                    circle_comp_id = 2;
                    if (dl_detect_reg != dl_done_reg) begin
                        if (dl_done_reg == 'b0) begin
                            print_dl_head;
                        end
                        print_circle_start(proc_path(origin), circle_id);
                        circle_id = circle_id + 1;
                    end
                    else begin
                        print_dl_end(circle_id - 1);
                        $finish;
                    end
                end
                ST_DL_REPORT: begin
                    if ((|(dl_in_vec)) & ~(|(dl_in_vec & origin_reg))) begin
                        print_circle_chan_comp(dl_in_vec_reg, dl_in_vec);
                        print_circle_proc_comp(proc_path(dl_in_vec), circle_comp_id);
                        circle_comp_id = circle_comp_id + 1;
                    end
                    else begin
                        print_circle_chan_comp(dl_in_vec_reg, dl_in_vec);
                    end
                end
            endcase
        end
    end
 
endmodule
