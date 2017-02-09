//-----------------------------------------------------------------------------
// File          : Gthe3Channel.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 02/16/2016
// Project       : 
//-----------------------------------------------------------------------------
// Description :
//    Device driver for Gthe3Channel
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/16/2016: created
//-----------------------------------------------------------------------------
#include <Gthe3Channel.h>
#include <Register.h>
#include <RegisterLink.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <unistd.h>

using namespace std;

// Constructor
Gthe3Channel::Gthe3Channel ( uint32_t linkConfig, uint32_t baseAddress, uint32_t index, Device *parent, uint32_t addrSize ) : 
                        Device(linkConfig,baseAddress,"Gthe3Channel",index,parent) {
   RegisterLink *rl;
   
   // Description
   desc_ = "Gthe3Channel";
   
   addRegisterLink(rl = new RegisterLink("REG_002", baseAddress_ + 0x002*addrSize, 1, 1,
                                         "CDR_SWAP_MODE_EN", Variable::Configuration, 0, 0x1));// 0
   addRegisterLink(rl = new RegisterLink("REG_003", baseAddress_ + 0x003*addrSize, 1, 4,
                                         "RXBUFRESET_TIME",    Variable::Configuration, 11, 0x1F,  // 0
                                         "EYE_SCAN_SWAP_EN",   Variable::Configuration,  9, 0x1,   // 1
                                         "RX_DATA_WIDTH",      Variable::Configuration,  5, 0xF,   // 2
                                         "RXCDRFREQRESET_TIME",Variable::Configuration,  0, 0x1F));// 3
   addRegisterLink(rl = new RegisterLink("REG_004", baseAddress_ + 0x004*addrSize, 1, 4,
                                         "RXCDRPHRESET_TIME",           Variable::Configuration, 11, 0x1F, // 0
                                         "PCI3_RX_ELECIDLE_H2L_DISABLE",Variable::Configuration,  8, 0x7,  // 1
                                         "RXDFELPMRESET_TIME",          Variable::Configuration,  1, 0x7F, // 2
                                         "RX_FABINT_USRCLK_FLOP",       Variable::Configuration,  0, 0x1));// 3
   addRegisterLink(rl = new RegisterLink("REG_005", baseAddress_ + 0x005*addrSize, 1, 6,
                                         "RXPMARESET_TIME",             Variable::Configuration, 11, 0x1F, // 0
                                         "PCI3_RX_ELECIDLE_LP4_DISABLE",Variable::Configuration, 10, 0x1,  // 1
                                         "PCI3_RX_ELECIDLE_EI2_ENABLE", Variable::Configuration,  9, 0x1,  // 2
                                         "PCI3_RX_FIFO_DISABLE",        Variable::Configuration,  8, 0x1,  // 3
                                         "RXPCSRESET_TIME",             Variable::Configuration,  3, 0x1F, // 4
                                         "RXELECIDLE_CFG",              Variable::Configuration,  0, 0x7));// 5
   addRegisterLink(rl = new RegisterLink("REG_006", baseAddress_ + 0x006*addrSize, 1, 1,
                                         "RXDFE_HB_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_009", baseAddress_ + 0x009*addrSize, 1, 4,
                                         "TXPMARESET_TIME",  Variable::Configuration, 11, 0x1F,  // 0
                                         "RX_PMA_POWER_SAVE",Variable::Configuration, 10, 0x1,   // 1
                                         "TX_PMA_POWER_SAVE",Variable::Configuration,  9, 0x1,   // 2
                                         "TXPCSRESET_TIME",  Variable::Configuration,  3, 0x1F));// 3
   addRegisterLink(rl = new RegisterLink("REG_00B", baseAddress_ + 0x00B*addrSize, 1, 3,
                                         "WB_MODE",              Variable::Configuration, 14, 0x3,  // 0
                                         "RXPMACLK_SEL",         Variable::Configuration,  8, 0x3,  // 1
                                         "TX_FABINT_USRCLK_FLOP",Variable::Configuration,  4, 0x1));// 2
   addRegisterLink(rl = new RegisterLink("REG_00C", baseAddress_ + 0x00C*addrSize, 1, 2,
                                         "TX_PROGCLK_SEL",   Variable::Configuration, 10, 0x3,   // 0
                                         "RXISCANRESET_TIME",Variable::Configuration,  5, 0x1F));// 1
   addRegisterLink(rl = new RegisterLink("REG_00E", baseAddress_ + 0x00E*addrSize, 1, 1,
                                         "RXCDR_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_00F", baseAddress_ + 0x00F*addrSize, 1, 1,
                                         "RXCDR_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_010", baseAddress_ + 0x010*addrSize, 1, 1,
                                         "RXCDR_CFG2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_011", baseAddress_ + 0x011*addrSize, 1, 1,
                                         "RXCDR_CFG3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_012", baseAddress_ + 0x012*addrSize, 1, 1,
                                         "RXCDR_CFG4", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_013", baseAddress_ + 0x013*addrSize, 1, 1,
                                         "RXCDR_LOCK_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_014", baseAddress_ + 0x014*addrSize, 1, 3,
                                         "CHAN_BOND_MAX_SKEW",Variable::Configuration, 12, 0xF,    // 0
                                         "CHAN_BOND_SEQ_LEN", Variable::Configuration, 10, 0x3,    // 1
                                         "CHAN_BOND_SEQ_1_1", Variable::Configuration,  0, 0x3FF));// 2
   addRegisterLink(rl = new RegisterLink("REG_015", baseAddress_ + 0x015*addrSize, 1, 2,
                                         "PCI3_RX_ELECIDLE_HI_COUNT",Variable::Configuration, 10, 0x1F,   // 0
                                         "CHAN_BOND_SEQ_1_3",        Variable::Configuration,  0, 0x3FF));// 1
   addRegisterLink(rl = new RegisterLink("REG_016", baseAddress_ + 0x016*addrSize, 1, 2,
                                         "PCI3_RX_ELECIDLE_H2L_COUNT",Variable::Configuration, 10, 0x1F, // 0
                                         "CHAN_BOND_SEQ_1_4",        Variable::Configuration,  0, 0x3FF));// 1
   addRegisterLink(rl = new RegisterLink("REG_017", baseAddress_ + 0x017*addrSize, 1, 5,
                                         "RX_BUFFER_CFG",         Variable::Configuration, 10, 0x3F, // 0
                                         "RX_DEFER_RESET_BUF_EN", Variable::Configuration,  9, 0x1,  // 1
                                         "OOBDIVCTL",             Variable::Configuration,  7, 0x3,  // 2
                                         "PCI3_AUTO_REALIGN",     Variable::Configuration,  5, 0x3,  // 3
                                         "PCI3_PIPE_RX_ELECIDLE", Variable::Configuration,  4, 0x1));// 4
   addRegisterLink(rl = new RegisterLink("REG_018", baseAddress_ + 0x018*addrSize, 1, 3,
                                         "CHAN_BOND_SEQ_1_ENABLE",   Variable::Configuration, 12, 0xF,    // 0
                                         "PCI3_RX_ASYNC_EBUF_BYPASS",Variable::Configuration, 10, 0x3,    // 1
                                         "CHAN_BOND_SEQ_2_1",        Variable::Configuration,  0, 0x3FF));// 2
   addRegisterLink(rl = new RegisterLink("REG_019", baseAddress_ + 0x019*addrSize, 1, 1,
                                         "CHAN_BOND_SEQ_2_2", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_01A", baseAddress_ + 0x01A*addrSize, 1, 1,
                                         "CHAN_BOND_SEQ_2_3", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_01B", baseAddress_ + 0x01B*addrSize, 1, 1,
                                         "CHAN_BOND_SEQ_2_4", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_01C", baseAddress_ + 0x01C*addrSize, 1, 4,
                                         "CHAN_BOND_SEQ_2_ENABLE",Variable::Configuration, 12, 0xF,   // 0
                                         "CHAN_BOND_SEQ_2_USE",   Variable::Configuration, 11, 0x1,   // 1
                                         "CLK_COR_KEEP_IDLE",     Variable::Configuration,  6, 0x1,   // 2
                                         "CLK_COR_MIN_LAT",       Variable::Configuration,  0, 0x3F));// 3
   addRegisterLink(rl = new RegisterLink("REG_01D", baseAddress_ + 0x01D*addrSize, 1, 5,
                                         "CLK_COR_MAX_LAT",     Variable::Configuration, 10, 0x3F, // 0
                                         "CLK_COR_PRECEDENCE",  Variable::Configuration,  9, 0x1,  // 1
                                         "CLK_COR_REPEAT_WAIT", Variable::Configuration,  4, 0x1F, // 2
                                         "CLK_COR_SEQ_LEN",     Variable::Configuration,  2, 0x3,  // 3
                                         "CHAN_BOND_KEEP_ALIGN",Variable::Configuration,  0, 0x1));// 4
   addRegisterLink(rl = new RegisterLink("REG_01E", baseAddress_ + 0x01E*addrSize, 1, 1,
                                         "CLK_COR_SEQ_1_1", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_01F", baseAddress_ + 0x01F*addrSize, 1, 1,
                                         "CLK_COR_SEQ_1_2", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_020", baseAddress_ + 0x020*addrSize, 1, 1,
                                         "CLK_COR_SEQ_1_3", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_021", baseAddress_ + 0x021*addrSize, 1, 1,
                                         "CLK_COR_SEQ_1_4", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_022", baseAddress_ + 0x022*addrSize, 1, 2,
                                         "CLK_COR_SEQ_1_ENABLE",Variable::Configuration, 12, 0xF,    // 0
                                         "CLK_COR_SEQ_2_1",     Variable::Configuration,  0, 0x3FF));// 1
   addRegisterLink(rl = new RegisterLink("REG_023", baseAddress_ + 0x023*addrSize, 1, 1,
                                         "CLK_COR_SEQ_2_2", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_024", baseAddress_ + 0x024*addrSize, 1, 4,
                                         "CLK_COR_SEQ_2_ENABLE",Variable::Configuration, 12, 0xF,    // 0
                                         "CLK_COR_SEQ_2_USE",   Variable::Configuration, 11, 0x1,    // 1
                                         "CLK_CORRECT_USE",     Variable::Configuration, 10, 0x1,    // 2
                                         "CLK_COR_SEQ_2_3",     Variable::Configuration,  0, 0x3FF));// 3
   addRegisterLink(rl = new RegisterLink("REG_025", baseAddress_ + 0x025*addrSize, 1, 1,
                                         "CLK_COR_SEQ_2_4", Variable::Configuration, 0, 0x3FF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_026", baseAddress_ + 0x026*addrSize, 1, 1,
                                         "RXDFE_HE_CFG0", Variable::Configuration, 0, 0xFFFF));// 0   
   addRegisterLink(rl = new RegisterLink("REG_027", baseAddress_ + 0x027*addrSize, 1, 4,
                                         "ALIGN_COMMA_WORD",  Variable::Configuration, 13, 0x7,    // 0
                                         "ALIGN_COMMA_DOUBLE",Variable::Configuration, 12, 0x1,    // 1
                                         "SHOW_REALIGN_COMMA",Variable::Configuration, 11, 0x1,    // 2
                                         "ALIGN_COMMA_ENABLE",Variable::Configuration,  0, 0x3FF));// 3
   addRegisterLink(rl = new RegisterLink("REG_028", baseAddress_ + 0x028*addrSize, 1, 3,
                                         "CPLL_FBDIV",   Variable::Configuration,  8, 0xFF, // 0
                                         "CPLL_FBDIV_45",Variable::Configuration,  7, 0x1,  // 1
                                         "TXDRVBIAS_N",  Variable::Configuration,  0, 0xF));// 2
   addRegisterLink(rl = new RegisterLink("REG_029", baseAddress_ + 0x029*addrSize, 1, 1,
                                         "CPLL_LOCK_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_02A", baseAddress_ + 0x02A*addrSize, 1, 3,
                                         "CPLL_REFCLK_DIV",Variable::Configuration, 11, 0x1F, // 0
                                         "SATA_CPLL_CFG",  Variable::Configuration,  5, 0x3,  // 1
                                         "TXDRVBIAS_P",    Variable::Configuration,  0, 0xF));// 2
   addRegisterLink(rl = new RegisterLink("REG_02B", baseAddress_ + 0x02B*addrSize, 1, 1,
                                         "CPLL_INIT_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_02C", baseAddress_ + 0x02C*addrSize, 1, 5,
                                         "DEC_PCOMMA_DETECT",Variable::Configuration, 15, 0x1,  // 0
                                         "TX_DIVRESET_TIME", Variable::Configuration,  7, 0x1F, // 1
                                         "RX_DIVRESET_TIME", Variable::Configuration,  2, 0x1F, // 2
                                         "A_TXPROGDIVRESET", Variable::Configuration,  1, 0x1,  // 3
                                         "A_RXPROGDIVRESET", Variable::Configuration,  0, 0x1));// 4
   addRegisterLink(rl = new RegisterLink("REG_02D", baseAddress_ + 0x02D*addrSize, 1, 1,
                                         "RXCDR_LOCK_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_02E", baseAddress_ + 0x02E*addrSize, 1, 1,
                                         "RXCFOK_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_02F", baseAddress_ + 0x02F*addrSize, 1, 1,
                                         "RXDFE_H2_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_030", baseAddress_ + 0x030*addrSize, 1, 1,
                                         "RXDFE_H2_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_031", baseAddress_ + 0x031*addrSize, 1, 1,
                                         "RXCFOK_CFG2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_032", baseAddress_ + 0x032*addrSize, 1, 1,
                                         "RXLPM_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_033", baseAddress_ + 0x033*addrSize, 1, 1,
                                         "RXLPM_KH_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_034", baseAddress_ + 0x034*addrSize, 1, 1,
                                         "RXLPM_KH_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_035", baseAddress_ + 0x035*addrSize, 1, 1,
                                         "RXDFELPM_KL_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_036", baseAddress_ + 0x036*addrSize, 1, 1,
                                         "RXDFELPM_KL_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_037", baseAddress_ + 0x037*addrSize, 1, 1,
                                         "RXLPM_OS_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_038", baseAddress_ + 0x038*addrSize, 1, 1,
                                         "RXLPM_OS_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_039", baseAddress_ + 0x039*addrSize, 1, 1,
                                         "RXLPM_GC_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_03A", baseAddress_ + 0x03A*addrSize, 1, 1,
                                         "DMONITOR_CFG1", Variable::Configuration, 8, 0xFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_03C", baseAddress_ + 0x03C*addrSize, 1, 4,
                                         "ES_CONTROL",    Variable::Configuration, 10, 0x3F,  // 0
                                         "ES_ERRDET_EN",  Variable::Configuration,  9, 0x1,   // 1
                                         "ES_EYE_SCAN_EN",Variable::Configuration,  8, 0x1,   // 2
                                         "ES_PRESCALE",   Variable::Configuration,  0, 0x1F));// 3
   addRegisterLink(rl = new RegisterLink("REG_03D", baseAddress_ + 0x03D*addrSize, 1, 1,
                                         "RXDFE_HC_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_03E", baseAddress_ + 0x03E*addrSize, 1, 1,
                                         "TX_PROGDIV_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_03F", baseAddress_ + 0x03F*addrSize, 1, 1,
                                         "ES_QUALIFIER0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_040", baseAddress_ + 0x040*addrSize, 1, 1,
                                         "ES_QUALIFIER1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_041", baseAddress_ + 0x041*addrSize, 1, 1,
                                         "ES_QUALIFIER2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_042", baseAddress_ + 0x042*addrSize, 1, 1,
                                         "ES_QUALIFIER3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_043", baseAddress_ + 0x043*addrSize, 1, 1,
                                         "ES_QUALIFIER4", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_044", baseAddress_ + 0x044*addrSize, 1, 1,
                                         "ES_QUAL_MASK0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_045", baseAddress_ + 0x045*addrSize, 1, 1,
                                         "ES_QUAL_MASK1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_046", baseAddress_ + 0x046*addrSize, 1, 1,
                                         "ES_QUAL_MASK2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_047", baseAddress_ + 0x047*addrSize, 1, 1,
                                         "ES_QUAL_MASK3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_048", baseAddress_ + 0x048*addrSize, 1, 1,
                                         "ES_QUAL_MASK4", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_049", baseAddress_ + 0x049*addrSize, 1, 1,
                                         "ES_SDATA_MASK0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_04A", baseAddress_ + 0x04A*addrSize, 1, 1,
                                         "ES_SDATA_MASK1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_04B", baseAddress_ + 0x04B*addrSize, 1, 1,
                                         "ES_SDATA_MASK2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_04C", baseAddress_ + 0x04C*addrSize, 1, 1,
                                         "ES_SDATA_MASK3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_04D", baseAddress_ + 0x04D*addrSize, 1, 1,
                                         "ES_SDATA_MASK4", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_04E", baseAddress_ + 0x04E*addrSize, 1, 2,
                                         "FTS_LANE_DESKEW_EN",   Variable::Configuration,  4, 0x1,  // 0
                                         "FTS_DESKEW_SEQ_ENABLE",Variable::Configuration,  0, 0xF));// 1
   addRegisterLink(rl = new RegisterLink("REG_04F", baseAddress_ + 0x04F*addrSize, 1, 2,
                                         "ES_HORZ_OFFSET",     Variable::Configuration,  4, 0xFFF,// 0
                                         "FTS_LANE_DESKEW_CFG",Variable::Configuration,  0, 0xF));// 1
   addRegisterLink(rl = new RegisterLink("REG_050", baseAddress_ + 0x050*addrSize, 1, 1,
                                         "RXDFE_HC_CFG1", Variable::Configuration, 0, 0xFFFF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_051", baseAddress_ + 0x051*addrSize, 1, 1,
                                         "ES_PMA_CFG", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_052", baseAddress_ + 0x052*addrSize, 1, 3,
                                         "RX_EN_HI_LR",    Variable::Configuration, 10, 0x1,  // 0
                                         "RX_DFE_AGC_CFG1",Variable::Configuration,  2, 0x7,  // 1
                                         "RX_DFE_AGC_CFG0",Variable::Configuration,  0, 0x3));// 2
   addRegisterLink(rl = new RegisterLink("REG_053", baseAddress_ + 0x053*addrSize, 1, 1,
                                         "RXDFE_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_054", baseAddress_ + 0x054*addrSize, 1, 1,
                                         "RXDFE_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_055", baseAddress_ + 0x055*addrSize, 1, 4,
                                         "LOCAL_MASTER",      Variable::Configuration, 13, 0x1,    // 0
                                         "PCS_PCIE_EN",       Variable::Configuration, 12, 0x1,    // 1
                                         "ALIGN_MCOMMA_DET",  Variable::Configuration, 10, 0x1,    // 2
                                         "ALIGN_MCOMMA_VALUE",Variable::Configuration,  0, 0x3FF));// 3
   addRegisterLink(rl = new RegisterLink("REG_056", baseAddress_ + 0x056*addrSize, 1, 2,
                                         "ALIGN_PCOMMA_DET",  Variable::Configuration, 10, 0x1,    // 0
                                         "ALIGN_PCOMMA_VALUE",Variable::Configuration,  0, 0x3FF));// 1
   addRegisterLink(rl = new RegisterLink("REG_057", baseAddress_ + 0x057*addrSize, 1, 1,
                                         "TXDLY_LCFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_058", baseAddress_ + 0x058*addrSize, 1, 1,
                                         "RXDFE_OS_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_059", baseAddress_ + 0x059*addrSize, 1, 1,
                                         "RXPHDLY_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05A", baseAddress_ + 0x05A*addrSize, 1, 1,
                                         "RXDFE_OS_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05B", baseAddress_ + 0x05B*addrSize, 1, 1,
                                         "RXDLY_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05C", baseAddress_ + 0x05C*addrSize, 1, 1,
                                         "RXDLY_LCFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05D", baseAddress_ + 0x05D*addrSize, 1, 1,
                                         "RXDFE_HF_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05E", baseAddress_ + 0x05E*addrSize, 1, 1,
                                         "RXDFE_HD_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_05F", baseAddress_ + 0x05F*addrSize, 1, 1,
                                         "RX_BIAS_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_060", baseAddress_ + 0x060*addrSize, 1, 1,
                                         "PCS_RSVD0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_061", baseAddress_ + 0x061*addrSize, 1, 5,
                                         "RXPH_MONITOR_SEL",Variable::Configuration, 11, 0x1F, // 0
                                         "RX_CM_BUF_PD",    Variable::Configuration, 10, 0x1,  // 1
                                         "RX_CM_BUF_CFG",   Variable::Configuration,  6, 0xF,  // 2
                                         "RX_CM_TRIM",      Variable::Configuration,  2, 0xF,  // 3
                                         "RX_CM_SEL",       Variable::Configuration,  0, 0x3));// 4
   addRegisterLink(rl = new RegisterLink("REG_062", baseAddress_ + 0x062*addrSize, 1, 6,
                                         "RX_SUM_DFETAPREP_EN",Variable::Configuration, 14, 0x1,  // 0
                                         "RX_SUM_VCM_OVWR",    Variable::Configuration, 13, 0x1,  // 1
                                         "RX_SUM_IREF_TUNE",   Variable::Configuration,  9, 0xF,  // 2
                                         "RX_SUM_RES_CTRL",    Variable::Configuration,  7, 0x3,  // 3
                                         "RX_SUM_VCMTUNE",     Variable::Configuration,  3, 0xF,  // 4
                                         "RX_SUM_VREF_TUNE",   Variable::Configuration,  0, 0x7));// 5
   addRegisterLink(rl = new RegisterLink("REG_063", baseAddress_ + 0x063*addrSize, 1, 4,
                                         "CBCC_DATA_SOURCE_SEL",Variable::Configuration, 15, 0x1,  // 0
                                         "OOB_PWRUP",           Variable::Configuration, 14, 0x1,  // 1
                                         "RXOOB_CFG",           Variable::Configuration,  5, 0x1FF,// 2
                                         "RXOUT_DIV",           Variable::Configuration,  0, 0x7));// 3
   addRegisterLink(rl = new RegisterLink("REG_064", baseAddress_ + 0x064*addrSize, 1, 7,
                                         "RX_SIG_VALID_DLY",   Variable::Configuration, 11, 0x1F, // 0
                                         "RXSLIDE_MODE",       Variable::Configuration,  9, 0x3,  // 1
                                         "RXPRBS_ERR_LOOPBACK",Variable::Configuration,  8, 0x1,  // 2
                                         "RXSLIDE_AUTO_WAIT",  Variable::Configuration,  4, 0xF,  // 3
                                         "RXBUF_EN",           Variable::Configuration,  3, 0x1,  // 4
                                         "RX_XCLK_SEL",        Variable::Configuration,  1, 0x3,  // 4
                                         "RXGEARBOX_EN",       Variable::Configuration,  0, 0x1));// 5
   addRegisterLink(rl = new RegisterLink("REG_065", baseAddress_ + 0x065*addrSize, 1, 2,
                                         "RXBUF_THRESH_OVFLW",Variable::Configuration, 10, 0x3F,   // 0
                                         "DMONITOR_CFG0",     Variable::Configuration,  0, 0x3FF));// 1
   addRegisterLink(rl = new RegisterLink("REG_066", baseAddress_ + 0x066*addrSize, 1, 10,
                                         "RXBUF_THRESH_OVRD",         Variable::Configuration, 15, 0x1,  // 0
                                         "RXBUF_RESET_ON_COMMAALIGN", Variable::Configuration, 14, 0x1,  // 1
                                         "RXBUF_RESET_ON_RATE_CHANGE",Variable::Configuration, 13, 0x1,  // 2
                                         "RXBUF_RESET_ON_CB_CHANGE",  Variable::Configuration, 12, 0x1,  // 3
                                         "RXBUF_THRESH_UNDFLW",       Variable::Configuration,  6, 0x3F, // 4
                                         "RX_CLKMUX_EN",              Variable::Configuration,  5, 0x1,  // 5
                                         "RX_DISPERR_SEQ_MATCH",      Variable::Configuration,  4, 0x1,  // 6
                                         "RXBUF_ADDR_MODE",           Variable::Configuration,  3, 0x1,  // 7
                                         "RX_WIDEMODE_CDR",           Variable::Configuration,  2, 0x1,  // 8
                                         "RX_INT_DATAWIDTH",          Variable::Configuration,  0, 0x3));// 9
   addRegisterLink(rl = new RegisterLink("REG_067", baseAddress_ + 0x067*addrSize, 1, 7,
                                         "RXBUF_EIDLE_HI_CNT",          Variable::Configuration, 12, 0xF,  // 0
                                         "RXCDR_HOLD_DURING_EIDLE",     Variable::Configuration, 11, 0x1,  // 1
                                         "RX_DFE_LPM_HOLD_DURING_EIDLE",Variable::Configuration, 10, 0x1,  // 2
                                         "RXBUF_EIDLE_LO_CNT",          Variable::Configuration,  4, 0xF,  // 3
                                         "RXBUF_RESET_ON_EIDLE",        Variable::Configuration,  3, 0x1,  // 4
                                         "RXCDR_FR_RESET_ON_EIDLE",     Variable::Configuration,  2, 0x1,  // 5
                                         "RXCDR_PH_RESET_ON_EIDLE",     Variable::Configuration,  1, 0x1));// 6
   addRegisterLink(rl = new RegisterLink("REG_068", baseAddress_ + 0x068*addrSize, 1, 3,
                                         "SATA_BURST_VAL",    Variable::Configuration, 13, 0x7,  // 0
                                         "SATA_BURST_SEQ_LEN",Variable::Configuration,  4, 0xF,  // 1
                                         "SATA_EIDLE_VAL",    Variable::Configuration,  0, 0x3));// 2
   addRegisterLink(rl = new RegisterLink("REG_069", baseAddress_ + 0x069*addrSize, 1, 2,
                                         "SATA_MIN_BURST",Variable::Configuration, 10, 0x3F,  // 0
                                         "SAS_MIN_COM",   Variable::Configuration,  1, 0x3F));// 1
   addRegisterLink(rl = new RegisterLink("REG_06A", baseAddress_ + 0x06A*addrSize, 1, 2,
                                         "SATA_MIN_INIT",Variable::Configuration, 10, 0x3F,  // 0
                                         "SATA_MIN_WAKE",Variable::Configuration,  1, 0x3F));// 1
   addRegisterLink(rl = new RegisterLink("REG_06B", baseAddress_ + 0x06B*addrSize, 1, 2,
                                         "SATA_MAX_BURST",Variable::Configuration, 10, 0x3F,  // 0
                                         "SAS_MAX_COM",   Variable::Configuration,  1, 0x3F));// 1
   addRegisterLink(rl = new RegisterLink("REG_06C", baseAddress_ + 0x06C*addrSize, 1, 2,
                                         "SATA_MAX_INIT",Variable::Configuration, 10, 0x3F,  // 0
                                         "SATA_MAX_WAKE",Variable::Configuration,  1, 0x3F));// 1
   addRegisterLink(rl = new RegisterLink("REG_06D", baseAddress_ + 0x06D*addrSize, 1, 1,
                                         "RX_CLK25_DIV", Variable::Configuration, 3, 0x1F));// 0
   addRegisterLink(rl = new RegisterLink("REG_06E", baseAddress_ + 0x06E*addrSize, 1, 1,
                                         "TXPHDLY_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_06F", baseAddress_ + 0x06F*addrSize, 1, 1,
                                         "TXPHDLY_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_070", baseAddress_ + 0x070*addrSize, 1, 1,
                                         "TXDLY_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_071", baseAddress_ + 0x071*addrSize, 1, 2,
                                         "TXPH_MONITOR_SEL",Variable::Configuration,  2, 0x1F, // 0
                                         "TAPDLY_SET_TX",   Variable::Configuration,  0, 0x3));// 1
   addRegisterLink(rl = new RegisterLink("REG_072", baseAddress_ + 0x072*addrSize, 1, 1,
                                         "RXCDR_LOCK_CFG2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_073", baseAddress_ + 0x073*addrSize, 1, 1,
                                         "TXPH_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_074", baseAddress_ + 0x074*addrSize, 1, 1,
                                         "TERM_RCAL_CFG", Variable::Configuration, 0, 0x7FFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_075", baseAddress_ + 0x075*addrSize, 1, 1,
                                         "RXDFE_HF_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_076", baseAddress_ + 0x076*addrSize, 1, 2,
                                         "PD_TRANS_TIME_FROM_P2",Variable::Configuration,  4, 0xFFF,// 0
                                         "TERM_RCAL_OVRD",       Variable::Configuration,  1, 0x3));// 1
   addRegisterLink(rl = new RegisterLink("REG_077", baseAddress_ + 0x077*addrSize, 1, 2,
                                         "PD_TRANS_TIME_NONE_P2",Variable::Configuration,  8, 0xFF,  // 0
                                         "PD_TRANS_TIME_TO_P2",  Variable::Configuration,  0, 0xFF));// 1
   addRegisterLink(rl = new RegisterLink("REG_078", baseAddress_ + 0x078*addrSize, 1, 1,
                                         "TRANS_TIME_RATE", Variable::Configuration, 8, 0xFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_079", baseAddress_ + 0x079*addrSize, 1, 2,
                                         "TST_RSV0",Variable::Configuration,  8, 0xFF,  // 0
                                         "TST_RSV1",Variable::Configuration,  0, 0xFF));// 1
   addRegisterLink(rl = new RegisterLink("REG_07A", baseAddress_ + 0x07A*addrSize, 1, 3,
                                         "TX_CLK25_DIV", Variable::Configuration, 11, 0x1F, // 0
                                         "TX_XCLK_SEL",  Variable::Configuration, 10, 0x1,  // 1
                                         "TX_DATA_WIDTH",Variable::Configuration,  0, 0xF));// 2
   addRegisterLink(rl = new RegisterLink("REG_07B", baseAddress_ + 0x07B*addrSize, 1, 2,
                                         "TX_DEEMPH0",Variable::Configuration,  8, 0xFF,  // 0
                                         "TX_DEEMPH1",Variable::Configuration,  0, 0xFF));// 1
   addRegisterLink(rl = new RegisterLink("REG_07C", baseAddress_ + 0x07C*addrSize, 1, 7,
                                         "TX_MAINCURSOR_SEL",         Variable::Configuration, 14, 0x1,  // 0
                                         "TXGEARBOX_EN",              Variable::Configuration, 13, 0x1,  // 1
                                         "TXOUT_DIV",                 Variable::Configuration,  8, 0x7,  // 2
                                         "TXBUF_EN",                  Variable::Configuration,  7, 0x1,  // 3
                                         "TXBUF_RESET_ON_RATE_CHANGE",Variable::Configuration,  6, 0x1,  // 4
                                         "TX_RXDETECT_REF",           Variable::Configuration,  3, 0x7,  // 5
                                         "TXFIFO_ADDR_CFG",           Variable::Configuration,  2, 0x1));// 6
   addRegisterLink(rl = new RegisterLink("REG_07D", baseAddress_ + 0x07D*addrSize, 1, 1,
                                         "TX_RXDETECT_CFG", Variable::Configuration, 2, 0x3FFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_07E", baseAddress_ + 0x07E*addrSize, 1, 5,
                                         "TX_CLKMUX_EN",           Variable::Configuration, 15, 0x1,  // 0
                                         "TX_LOOPBACK_DRIVE_HIZ",  Variable::Configuration, 14, 0x1,  // 1
                                         "TX_DRIVE_MODE",          Variable::Configuration,  8, 0x1F, // 2
                                         "TX_EIDLE_ASSERT_DELAY",  Variable::Configuration,  5, 0x7,  // 3
                                         "TX_EIDLE_DEASSERT_DELAY",Variable::Configuration,  2, 0x7));// 4
   addRegisterLink(rl = new RegisterLink("REG_07F", baseAddress_ + 0x07F*addrSize, 1, 2,
                                         "TX_MARGIN_FULL_0",Variable::Configuration,  9, 0x7F,  // 0
                                         "TX_MARGIN_FULL_1",Variable::Configuration,  1, 0x7F));// 1
   addRegisterLink(rl = new RegisterLink("REG_080", baseAddress_ + 0x080*addrSize, 1, 2,
                                         "TX_MARGIN_FULL_2",Variable::Configuration,  9, 0x7F,  // 0
                                         "TX_MARGIN_FULL_3",Variable::Configuration,  1, 0x7F));// 1
   addRegisterLink(rl = new RegisterLink("REG_081", baseAddress_ + 0x081*addrSize, 1, 2,
                                         "TX_MARGIN_FULL_4",Variable::Configuration,  9, 0x7F,  // 0
                                         "TX_MARGIN_LOW_0", Variable::Configuration,  1, 0x7F));// 1
   addRegisterLink(rl = new RegisterLink("REG_082", baseAddress_ + 0x082*addrSize, 1, 2,
                                         "TX_MARGIN_LOW_1",Variable::Configuration,  9, 0x7F,  // 0
                                         "TX_MARGIN_LOW_2",Variable::Configuration,  1, 0x7F));// 1
   addRegisterLink(rl = new RegisterLink("REG_083", baseAddress_ + 0x083*addrSize, 1, 2,
                                         "TX_MARGIN_LOW_3",Variable::Configuration,  9, 0x7F,  // 0
                                         "TX_MARGIN_LOW_4",Variable::Configuration,  1, 0x7F));// 1
   addRegisterLink(rl = new RegisterLink("REG_084", baseAddress_ + 0x084*addrSize, 1, 1,
                                         "RXDFE_HD_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_085", baseAddress_ + 0x085*addrSize, 1, 2,
                                         "TX_QPI_STATUS_EN",Variable::Configuration, 13, 0x1,  // 0
                                         "TX_INT_DATAWIDTH",Variable::Configuration, 10, 0x3));// 1
   addRegisterLink(rl = new RegisterLink("REG_089", baseAddress_ + 0x089*addrSize, 1, 1,
                                         "RXPRBS_LINKACQ_CNT", Variable::Configuration, 0, 0xFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_08A", baseAddress_ + 0x08A*addrSize, 1, 9,
                                         "TX_PMADATA_OPT",   Variable::Configuration, 15, 0x1,   // 0
                                         "RXSYNC_OVRD",      Variable::Configuration, 14, 0x1,   // 1
                                         "TXSYNC_OVRD",      Variable::Configuration, 13, 0x1,   // 2
                                         "TX_IDLE_DATA_ZERO",Variable::Configuration, 12, 0x1,   // 3
                                         "A_RXOSCALRESET",   Variable::Configuration, 11, 0x1,   // 4
                                         "RXOOB_CLK_CFG",    Variable::Configuration, 10, 0x1,   // 5
                                         "TXSYNC_SKIP_DA",   Variable::Configuration,  9, 0x1,   // 6
                                         "RXSYNC_SKIP_DA",   Variable::Configuration,  8, 0x1,   // 7
                                         "RXOSCALRESET_TIME",Variable::Configuration,  0, 0x1F));// 8  
   addRegisterLink(rl = new RegisterLink("REG_08B", baseAddress_ + 0x08B*addrSize, 1, 3,
                                         "TXSYNC_MULTILANE",Variable::Configuration, 10, 0x1,   // 0
                                         "RXSYNC_MULTILANE",Variable::Configuration,  9, 0x1,   // 1
                                         "RX_CTLE3_LPF",    Variable::Configuration,  0, 0xFF));// 2  
   addRegisterLink(rl = new RegisterLink("REG_08C", baseAddress_ + 0x08C*addrSize, 1, 7,
                                         "ACJTAG_MODE",            Variable::Configuration, 15, 0x1,  // 0
                                         "ACJTAG_DEBUG_MODE",      Variable::Configuration, 14, 0x1,  // 1
                                         "ACJTAG_RESET",           Variable::Configuration, 13, 0x1,  // 2
                                         "RESET_POWERSAVE_DISABLE",Variable::Configuration, 12, 0x1,  // 3
                                         "RX_TUNE_AFE_OS",         Variable::Configuration, 10, 0x3,  // 4
                                         "RX_DFE_KL_LPM_KL_CFG0",  Variable::Configuration,  8, 0x3,  // 5
                                         "RX_DFE_KL_LPM_KL_CFG1",  Variable::Configuration,  5, 0x7));// 6
   addRegisterLink(rl = new RegisterLink("REG_08D", baseAddress_ + 0x08D*addrSize, 1, 1,
                                         "RXDFELPM_KL_CFG2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_08E", baseAddress_ + 0x08E*addrSize, 1, 1,
                                         "RXDFE_VP_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_08F", baseAddress_ + 0x08F*addrSize, 1, 1,
                                         "RXDFE_VP_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_090", baseAddress_ + 0x090*addrSize, 1, 1,
                                         "RXDFE_UT_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_091", baseAddress_ + 0x091*addrSize, 1, 1,
                                         "ADAPT_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_092", baseAddress_ + 0x092*addrSize, 1, 1,
                                         "ADAPT_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_093", baseAddress_ + 0x093*addrSize, 1, 1,
                                         "RXCFOK_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_094", baseAddress_ + 0x094*addrSize, 1, 2,
                                         "ES_CLK_PHASE_SEL",     Variable::Configuration, 11, 0x1,  // 0
                                         "USE_PCS_CLK_PHASE_SEL",Variable::Configuration, 10, 0x1));// 1
   addRegisterLink(rl = new RegisterLink("REG_095", baseAddress_ + 0x095*addrSize, 1, 1,
                                         "PMA_RSV1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_097", baseAddress_ + 0x097*addrSize, 1, 6,
                                         "RX_AFE_CM_EN",         Variable::Configuration, 12, 0x1,  // 0
                                         "RX_CAPFF_SARC_ENB",    Variable::Configuration, 11, 0x1,  // 1
                                         "RX_EYESCAN_VS_NEG_DIR",Variable::Configuration, 10, 0x1,  // 2
                                         "RX_EYESCAN_VS_UT_SIGN",Variable::Configuration,  9, 0x1,  // 3
                                         "RX_EYESCAN_VS_CODE",   Variable::Configuration,  2, 0x7F, // 4
                                         "RX_EYESCAN_VS_RANGE",  Variable::Configuration,  0, 0x3));// 5
   addRegisterLink(rl = new RegisterLink("REG_098", baseAddress_ + 0x098*addrSize, 1, 1,
                                         "RXDFE_HE_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_099", baseAddress_ + 0x099*addrSize, 1, 7,
                                         "GEARBOX_MODE",      Variable::Configuration, 11, 0x1F, // 0
                                         "TXPI_SYNFREQ_PPM",  Variable::Configuration,  8, 0x7,  // 1
                                         "TXPI_PPMCLK_SEL",   Variable::Configuration,  7, 0x1,  // 2
                                         "TXPI_INVSTROBE_SEL",Variable::Configuration,  6, 0x1,  // 3
                                         "TXPI_GRAY_SEL",     Variable::Configuration,  5, 0x1,  // 4
                                         "TXPI_LPM",          Variable::Configuration,  3, 0x1,  // 5
                                         "TXPI_VREFSEL",      Variable::Configuration,  2, 0x1));// 6
   addRegisterLink(rl = new RegisterLink("REG_09A", baseAddress_ + 0x09A*addrSize, 1, 1,
                                         "TXPI_PPM_CFG", Variable::Configuration, 0, 0xFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_09B", baseAddress_ + 0x09B*addrSize, 1, 5,
                                         "RX_DFELPM_KLKH_AGC_STUP_EN",Variable::Configuration, 15, 0x1,  // 0
                                         "RX_DFELPM_CFG0",            Variable::Configuration, 11, 0xF,  // 1
                                         "RX_DFELPM_CFG1",            Variable::Configuration, 10, 0x1,  // 2
                                         "RX_DFE_KL_LPM_KH_CFG0",     Variable::Configuration,  8, 0x3,  // 3
                                         "RX_DFE_KL_LPM_KH_CFG1",     Variable::Configuration,  5, 0x7));// 4
   addRegisterLink(rl = new RegisterLink("REG_09C", baseAddress_ + 0x09C*addrSize, 1, 6,
                                         "TXPI_CFG0",Variable::Configuration, 11, 0x3,  // 0
                                         "TXPI_CFG1",Variable::Configuration,  9, 0x3,  // 1
                                         "TXPI_CFG2",Variable::Configuration,  7, 0x3,  // 2
                                         "TXPI_CFG3",Variable::Configuration,  6, 0x1,  // 3
                                         "TXPI_CFG4",Variable::Configuration,  5, 0x1,  // 4
                                         "TXPI_CFG5",Variable::Configuration,  2, 0x7));// 5
   addRegisterLink(rl = new RegisterLink("REG_09D", baseAddress_ + 0x09D*addrSize, 1, 7,
                                         "RXPI_CFG1",Variable::Configuration, 14, 0x3,  // 0
                                         "RXPI_CFG2",Variable::Configuration, 12, 0x3,  // 1
                                         "RXPI_CFG3",Variable::Configuration, 10, 0x3,  // 2
                                         "RXPI_CFG4",Variable::Configuration,  9, 0x1,  // 3
                                         "RXPI_CFG5",Variable::Configuration,  8, 0x1,  // 4
                                         "RXPI_CFG6",Variable::Configuration,  5, 0x7,  // 5
                                         "RXPI_CFG0",Variable::Configuration,  3, 0x3));// 6
   addRegisterLink(rl = new RegisterLink("REG_09E", baseAddress_ + 0x09E*addrSize, 1, 1,
                                         "RXDFE_UT_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_09F", baseAddress_ + 0x09F*addrSize, 1, 1,
                                         "RXDFE_GC_CFG0", Variable::Configuration, 0, 0xFFFF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_0A0", baseAddress_ + 0x0A0*addrSize, 1, 1,
                                         "RXDFE_GC_CFG1", Variable::Configuration, 0, 0xFFFF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_0A1", baseAddress_ + 0x0A1*addrSize, 1, 1,
                                         "RXDFE_GC_CFG2", Variable::Configuration, 0, 0xFFFF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_0A2", baseAddress_ + 0x0A2*addrSize, 1, 1,
                                         "RXCDR_CFG0_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A3", baseAddress_ + 0x0A3*addrSize, 1, 1,
                                         "RXCDR_CFG1_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A4", baseAddress_ + 0x0A4*addrSize, 1, 1,
                                         "RXCDR_CFG2_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A5", baseAddress_ + 0x0A5*addrSize, 1, 1,
                                         "RXCDR_CFG3_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A6", baseAddress_ + 0x0A6*addrSize, 1, 1,
                                         "RXCDR_CFG4_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A7", baseAddress_ + 0x0A7*addrSize, 1, 1,
                                         "RXCDR_CFG5_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0A8", baseAddress_ + 0x0A8*addrSize, 1, 1,
                                         "RXCDR_CFG5", Variable::Configuration, 0, 0xFFFF));// 0 
   addRegisterLink(rl = new RegisterLink("REG_0A9", baseAddress_ + 0x0A9*addrSize, 1, 1,
                                         "PCIE_RXPMA_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0AA", baseAddress_ + 0x0AA*addrSize, 1, 1,
                                         "PCIE_TXPCS_CFG_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0AB", baseAddress_ + 0x0AB*addrSize, 1, 1,
                                         "PCIE_TXPMA_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0AC", baseAddress_ + 0x0AC*addrSize, 1, 2,
                                         "RX_CLK_SLIP_OVRD",Variable::Configuration,  3, 0x1F, // 0
                                         "PCS_RSVD1",       Variable::Configuration,  0, 0x7));// 1
   addRegisterLink(rl = new RegisterLink("REG_0AD", baseAddress_ + 0x0AD*addrSize, 1, 5,
                                         "PLL_SEL_MODE_GEN3", Variable::Configuration, 11, 0x3,  // 0
                                         "PLL_SEL_MODE_GEN12",Variable::Configuration,  9, 0x3,  // 1
                                         "RATE_SW_USE_DRP",   Variable::Configuration,  8, 0x1,  // 2
                                         "RXPI_LPM",          Variable::Configuration,  3, 0x1,  // 3
                                         "RXPI_VREFSEL",      Variable::Configuration,  2, 0x1));// 4
   addRegisterLink(rl = new RegisterLink("REG_0AE", baseAddress_ + 0x0AE*addrSize, 1, 1,
                                         "RXDFE_H3_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0AF", baseAddress_ + 0x0AF*addrSize, 1, 4,
                                         "DFE_D_X_REL_POS",Variable::Configuration, 15, 0x1,    // 0
                                         "DFE_VCM_COMP_EN",Variable::Configuration, 14, 0x1,    // 1
                                         "GM_BIAS_SELECT", Variable::Configuration, 13, 0x1,    // 2
                                         "EVODD_PHI_CFG",  Variable::Configuration,  0, 0x7FF));// 3
   addRegisterLink(rl = new RegisterLink("REG_0B0", baseAddress_ + 0x0B0*addrSize, 1, 1,
                                         "RXDFE_H3_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B1", baseAddress_ + 0x0B1*addrSize, 1, 1,
                                         "RXDFE_H4_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B2", baseAddress_ + 0x0B2*addrSize, 1, 1,
                                         "RXDFE_H4_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B3", baseAddress_ + 0x0B3*addrSize, 1, 1,
                                         "RXDFE_H5_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B4", baseAddress_ + 0x0B4*addrSize, 1, 4,
                                         "PROCESS_PAR",     Variable::Configuration, 13, 0x7,  // 0
                                         "TEMPERATUR_PAR",  Variable::Configuration,  8, 0xF,  // 1
                                         "TX_MODE_SEL",     Variable::Configuration,  5, 0x7,  // 2
                                         "TX_SARC_LPBK_ENB",Variable::Configuration,  4, 0x1));// 3 
   addRegisterLink(rl = new RegisterLink("REG_0B5", baseAddress_ + 0x0B5*addrSize, 1, 1,
                                         "RXDFE_H5_CFG1", Variable::Configuration, 0, 0xFFFF));// 0  
   addRegisterLink(rl = new RegisterLink("REG_0B6", baseAddress_ + 0x0B6*addrSize, 1, 4,
                                         "TX_DCD_CFG",     Variable::Configuration, 10, 0x3F,  // 0
                                         "TX_DCD_EN",      Variable::Configuration,  9, 0x1,   // 1
                                         "TX_EML_PHI_TUNE",Variable::Configuration,  8, 0x1,   // 2
                                         "CPLL_CFG3",      Variable::Configuration,  0, 0x3F));// 3
   addRegisterLink(rl = new RegisterLink("REG_0B7", baseAddress_ + 0x0B7*addrSize, 1, 1,
                                         "RXDFE_H6_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B8", baseAddress_ + 0x0B8*addrSize, 1, 1,
                                         "RXDFE_H6_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0B9", baseAddress_ + 0x0B9*addrSize, 1, 1,
                                         "RXDFE_H7_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0BA", baseAddress_ + 0x0BA*addrSize, 1, 2,
                                         "DDI_REALIGN_WAIT",Variable::Configuration,  2, 0x1F, // 0
                                         "DDI_CTRL",        Variable::Configuration,  0, 0x3));// 1
   addRegisterLink(rl = new RegisterLink("REG_0BB", baseAddress_ + 0x0BB*addrSize, 1, 4,
                                         "TXGBOX_FIFO_INIT_RD_ADDR",Variable::Configuration,  9, 0x7,  // 0
                                         "TX_SAMPLE_PERIOD",        Variable::Configuration,  6, 0x7,  // 1
                                         "RXGBOX_FIFO_INIT_RD_ADDR",Variable::Configuration,  3, 0x7,  // 2
                                         "RX_SAMPLE_PERIOD",        Variable::Configuration,  0, 0x7));// 3
   addRegisterLink(rl = new RegisterLink("REG_0BC", baseAddress_ + 0x0BC*addrSize, 1, 1,
                                         "CPLL_CFG2", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0BD", baseAddress_ + 0x0BD*addrSize, 1, 1,
                                         "RXPHSAMP_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0BE", baseAddress_ + 0x0BE*addrSize, 1, 1,
                                         "RXPHSLIP_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0BF", baseAddress_ + 0x0BF*addrSize, 1, 1,
                                         "RXPHBEACON_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C0", baseAddress_ + 0x0C0*addrSize, 1, 1,
                                         "RXDFE_H7_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C1", baseAddress_ + 0x0C1*addrSize, 1, 1,
                                         "RXDFE_H8_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C2", baseAddress_ + 0x0C2*addrSize, 1, 1,
                                         "RXDFE_H8_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C3", baseAddress_ + 0x0C3*addrSize, 1, 1,
                                         "PCIE_BUFG_DIV_CTRL", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C4", baseAddress_ + 0x0C4*addrSize, 1, 1,
                                         "PCIE_RXPCS_CFG_GEN3", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C5", baseAddress_ + 0x0C5*addrSize, 1, 1,
                                         "RXDFE_H9_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C6", baseAddress_ + 0x0C6*addrSize, 1, 1,
                                         "RX_PROGDIV_CFG", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C7", baseAddress_ + 0x0C7*addrSize, 1, 1,
                                         "RXDFE_H9_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0C8", baseAddress_ + 0x0C8*addrSize, 1, 1,
                                         "RXDFE_HA_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0CA", baseAddress_ + 0x0CA*addrSize, 1, 1,
                                         "CHAN_BOND_SEQ_1_2", Variable::Configuration, 0, 0x3FF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0CB", baseAddress_ + 0x0CB*addrSize, 1, 1,
                                         "CPLL_CFG0", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0CC", baseAddress_ + 0x0CC*addrSize, 1, 1,
                                         "CPLL_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0CD", baseAddress_ + 0x0CD*addrSize, 1, 4,
                                         "CPLL_INIT_CFG1",      Variable::Configuration,  8, 0xFF, // 0
                                         "RX_DDI_SEL",          Variable::Configuration,  2, 0x3F, // 1
                                         "DEC_VALID_COMMA_ONLY",Variable::Configuration,  1, 0x1,  // 2
                                         "DEC_MCOMMA_DETECT",   Variable::Configuration,  0, 0x1));// 3
   addRegisterLink(rl = new RegisterLink("REG_0CE", baseAddress_ + 0x0CE*addrSize, 1, 1,
                                         "RXDFE_HA_CFG1", Variable::Configuration, 0, 0xFFFF));// 0
   addRegisterLink(rl = new RegisterLink("REG_0CF", baseAddress_ + 0x0CF*addrSize, 1, 1,
                                         "RXDFE_HB_CFG0", Variable::Configuration, 0, 0xFFFF));// 0

   // Variables
   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
Gthe3Channel::~Gthe3Channel ( ) { }