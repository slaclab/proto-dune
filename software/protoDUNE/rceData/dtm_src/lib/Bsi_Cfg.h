// -*-Mode: C;-*-
/*
@file Bsi_Cfg.h
@verbatim
                               Copyright 2011
                                    by
                       The Board of Trustees of the
                    Leland Stanford Junior University.
                           All rights reserved.
@endverbatim

@par Facility:


@par Abstract:
This file contains the masks, sizes and offsets for extractign fields
from words in the BSI (BootStrap Interface.)

@author
Gregg Thayer <jgt@slac.stanford.edu>

@par Date created:
2012/01/03

@par Last commit:
\$Date: 2013-10-04 14:45:06 -0700 (Fri, 04 Oct 2013) $ by \$Author: smaldona $.

@par Revision number:
\$Revision: 2165 $

@par Credits:
SLAC
*/

#ifndef BSI_CFG_H
#define BSI_CFG_H

/* This is the size of the BSI block RAM in bytes*/
enum { BSI_CFG_SIZE = 512*4 };

/*
   This is the value that the CE writes to the above addresss
   when boot is successful
*/
enum { BSI_BOOT_RESPONSE_SUCCESS = 0 };

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- *//*

  The layout of the BSI configuration in memory is composed of fixed
  length components. There is a portion which is written by the
  Cluster Element and read by the IPMC, and there is a portion
  written by the IPMC and read by the Cluster Element. There is an
  additional portion that is written by the IPMC only to the
  Cluster Manager.

  This file will describe the layout of the BSI configuration data
  and provide enumerations and macros to assist in the extraction of
  these data.

  This is the portion of the BSI that is written by the Cluster Element.
  It is common to both Cluster Elements and the Cluster Manager.

  ----------------------------------------
  |  Cluster Element BSI Version Number  |
  |  Cluster Element Network PHY type    |
  |  Cluster Element MAC address         |
  |  Cluster Element Interconnect Def    |
  |  Uboot Version Number                |
  |  DAT Version Number                  |
  |  Zynq Device DNA                     |
  |  Zynq eFUSE User code                |
  ----------------------------------------
  |                                      |
  =             Reserved                 =
  |                                      |
  ----------------------------------------

  Here are descriptions of the above:

  The Cluster Element BSI Version Number is a single 32-bit word which
  describes the layout BSI.

  The Cluster Element Network PHY type is a single 32-bit word which
  contains an enumeration of the network configuration needed by
  the switch. It is split into 4 1-byte fields, each of which describes
  the configuration of a single lane of communication with the switch.
  In cases of CMBs with multiple RCEs, one channel (of 4 lanes) is
  shared by Elements 0 and 1, and one lane is shared by Elements 2 and 3.
  Elements which share a channel must have lane definitions which do not
  conflict. This information is collated by the IPMC and given to the
  DTM as a list of configurations for the 24 ports. (See below.)

  The Cluster Element MAC address is a 2 word MAC address. The
  MAC address is laid out as a little endian byte array with the
  two MSB, MBZ.

  The Cluster Element Interconnect Definition is a single 32-bit word
  which describes the use of communication lines between the Cluster
  Element and the Cluster Manager. This information is collected from
  the BSI of the Cluster Elements in a way similar to the Ethernet PHY
  definitions. Precisely how the 32-bits is broken up and shared is
  currently unclear to me.

  The Uboot and DAT Versions are each 32 byte, null terminated  strings.

  The Zynq Device DNA is a 57 bit embedded device identifier permanently 
  programmed by Xilinx into the FPGA. According to Xilinx documentation,
  it is "often unique. However up to 32 devices within the family can
  contain the same DNA value. The number is zero extended to 64 bits.

  The eFUSE User register is a 32-bit number stored in the Zynq. The RCE
  extend it to 64 bits to present to the BSI. 

  There will be reserved space between the Cluster Element written
  portion of the BSI and the IPMC written portion.

  ----------------------------------------
  ----------------------------------------
  ----------------------------------------

  This is the portion of the BSI that is written by the IPMC and is
  common to both Cluster Elements and the Cluster Manager.

  ----------------------------------------
  |  CMB Serial Number                   |
  |  Cluster Address                     |
  |  Cluster Group Name  (8 words)       |
  |  External Interconnect Definition    |
  ----------------------------------------
  |                                      |
  =             Reserved                 =
  |                                      |
  ----------------------------------------

  The CMB Serial Number is two 32-bit words.

  The Cluster Address is a single 32-bit word and contains the fields:
  1) Version        - Version number covering the layout of the
                      cluster address word
  2) Cluster Number
  3) Bay Number
  4) Element Number

  The Cluster Group Name is a null terminated, little endian, character
  array. Meaning that the first character is the LSB of the first word.

  The External Interconnect Definition is a single 32-bit word which
  describes the portion of the RTM that is connected to the Cluster
  Element. In the case of DPMs this will be found in the EEPROM on the
  I2C corner of the RTM. In the case of the DTM, it will be found in
  the EEPROM on the RMB.

  There will be reserved space between the Cluster Element written
  portion of the BSI and the IPMC written portion.

  ----------------------------------------
  ----------------------------------------
  ----------------------------------------

  The BSI structure for the Cluster Manager additionally includes
  the following information.

  ----------------------------------------
  |                                      |
  | Cluster Switch configuration         |
  |                                      |
  ----------------------------------------
  | Reserved                             |
  ----------------------------------------
  |                                      |
  | Shelf IP Information
  |                                      |
  ----------------------------------------
  |  Reserved                            |
  ----------------------------------------
  |                                      |
  | Fabric Map                           |
  |                                      |
  ----------------------------------------
  |  Reserved                            |
  ----------------------------------------

  
  ----------------------------------------
  Cluster Switch Configuration

  The Cluster Switch configuration is currently a 24 word array
  which describe the type of connection which is made to the 24
  ports on the Cluster's switch. Each word is broken into two parts
  the lower part describes the configuration of the port (1G, XAUI, etc.)
  the upper part describes the group of ports each port belongs to. Each
  member of a port group is electrically connected to the same Ethernet
  destination.

  ----------------------------------------
  Shelf IP Information

  This is the IP information that is stored in the Shelf FRU Information.
  It describes the COB (DTM) specific networking configuration needed 
  to operate in the shelf. It is composed of 6 32-bit registers:

  --------------------------------------------
  | Port Membership                          |
  | Port Ingress Policy                      |
  | Group Base Address                       |
  | Group End Address                        |
  | Subnet Mask                              |
  | Gateway                                  |
  | Status                                   |
  --------------------------------------------

  The Port Membership register is made up of 2 fields:
  bit[31]      = Apply
  bits[30..12] = MBZ
  bits[11..0]  = VLAN ID

  The VLAN ID field specifies the VLAN to which the shelf belongs, if and
  only if the Apply bit is set.

  The Port Ingress Policy has the following structure:
  bits[31..3] = MBZ
  bit[2] = Boundary Violations
  bit[1] = Tagged
  bit[0] = Untagged

  When the Boundary Violations bit is set, frames that violate the VLAN 
  boundary are discarded.
  
  When the Tagged bit is set, all arriving VLAN tagged frames are discarded.

  When the Untagged bit is set, all arriving VLAN untagged frames are
  discarded.

  The Group Base address is the base address for the block of addresses
  available to the shelf's DHCP server. It is stored in network order.

  The Group End address is the end address for the block of addresses
  available to the shelf's DHCP server. It is stored in network order.

  The Subnet Mask is the subnet mask for the shelf, it also identifies
  the network assined to the shelf. It is stored in network order.

  The Gateway address is the address of the gateway for the shelf.
  It is stored in network order.

  The Status word indicates that the IPMC has completed filling in this
  block. The contents are:

  bit[31]    = Done
  bit[30]    = Error
  bit[29..0] = MBZ

  Where Done indicates that this block has been completely filled, and
  Error indicates that the block will not be filled due to an error. (Like
  the information retrieved from the Shelf FRU Info could not be parsed.

  ----------------------------------------
  Fabric Map

  --------------------------------------------
  | Slot  1, Channel  1 | Slot  1, Channel 2 |
  | Slot  1, Channel  3 | Slot  1, Channel 4 |
  =                                          =
  | Slot  1, Channel 15 | MBZ                |
  | Slot  2, Channel  1 | Slot  2, Channel 2 |
  =                                          =
  | Slot 16, Channel 15 | MBZ                |
  | Logical Slot                             |
  --------------------------------------------

  The ATCA Fabric Map is contains a digest of the Zone2 Backplane
  Fabric interconnect tables. It is written into the BSI of the Cluster 
  Manager so that it can properly configure the switch in the presence
  of different backplane topologies. (Full-Mesh, Replicated-Mesh, 
  Dual-Star, etc.) The ATCA Fabric allows for a maximum 15 channels
  connecting 16 slots. 

  Each interconnect is encoded in a 16-bit word. The map is an array 
  of 16*16=256 16-bit words as seen below. 
  The map is followed by 1 word which will contains the logical slot
  that the board is in. The logical slot will be the last word written,
  so that when it is non-zero, the map will be complete.
  
  The format of each 16-bit word is as follows:
  bit 15     : Done
  bit 14     : Error
  bit 13     : MBZ
  bits 12..8 : COB switch Port
  bits 7..4  : Destination Channel
  bits 3..0  : Destination Slot

  The Done bit is set when the interconnect has been written. (It ensures
  that a written entry is never zero.) The error bit is set when an 
  inconsistency is found in the interconnect map recieved from the shelf
  manager. 

  ATCA allows the map to be defined from both ends of the interconnect, so
  inconsistency is possible. In the case where the two interconnect
  definitions disagree, the first will be chosen, and the  Error bit set.
  This indicates an error in the Shelf FRU information.
  
  The COB switch port, is not part of ATCA, but it can be COB version
  specific. The IPMC knows this mapping as it is part of the COB. So that
  the DTM does need to know about the different possible mappings, the 
  IPMC will place the Switch Port (0..23) for this interconnect. In the 
  case that the Fabric channel is not connected to the switch, the COB
  switch port will be 0x1F. 

  The Destination Channel (0..14) is the fabric channel on the remote 
  board of this interconnect. 

  The Destination Slot (0..15) is the logical slot number (minus one)
  of the remote board of this interconnect.

  The array of interconnect definitions is aligned as follows where I
  have numbered the slots and channels by their ATCA indicies, in the 
  actual table slots and channel are numbered from zero. In order to
  keep the table aligned the 16th channel entry for each slot is MBZ.

  The logical slot number is the slot number counting from 0 in the 
  lower 31 bits, with the highest order bit a done bit. Bit 30 is a
  bit which signifies that this COB's SFP(s) are serving as the only
  external Ethernet connection for the shelf.

  bit  31    = Done
  bit  30    = Hub
  bits 29..0 = Logical Slot Number
 
  ----------------------------------------
  ----------------------------------------

  ----------------------------------------
  |  Cluster Element                     |
  |  Interconnect Definitions            |
  =  (variable length?)                  =
  |                                      |
  ----------------------------------------

  The Cluster Element Interconnect Definitions is an array of 32-bit
  the Cluster Element Interconnect Definitions which will be collected
  from each Cluster Element. (I don't yet know how they are organized...)
  I presume it will be one 32-bit word per DPM bay, but it may not be. 
  When we have decided how to implement these, we will place them in 
  one of the Reserved areas, the either the Cluster Element area or
  the Cluster Manager area.

  ----------------------------------------
  ----------------------------------------

  ----------------------------------------
  |  RCE Status                          |
  ----------------------------------------

  At the end of the BSI, there will be a status which will be written
  by the RCE and polled by the IPMC. The precise meaning of this status
  value is not specified to the IPMC, save the fact that a value of zero
  indicates that the RCE has completely booted and is not currently in an 
  error state.
									  */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */


/* ---------------------------------------------------------------------- */
/*
  Here are the offsets and the version number which describes the
  Cluster Element written portion of the BSI.
*/
enum {BSI_CLUSTER_CFG_VERSION = 2};

typedef enum _BSI_COMMON_CFG_OFFSETS
{
   BSI_RCE_DATA_OFFSET                      = 0x00,
   BSI_CLUSTER_CFG_VERSION_OFFSET           = 0x00,
   BSI_NETWORK_PHY_TYPE_OFFSET              = 0x01,
   BSI_MAC_ADDR_OFFSET                      = 0x02,
   BSI_INTERCONNECT_OFFSET                  = 0x04,
   BSI_UBOOT_VERSION_OFFSET                 = 0x05,
   BSI_DAT_VERSION_OFFSET                   = 0x0D,
   BSI_DEVICE_DNA_OFFSET                    = 0x15,
   BSI_EFUSE_OFFSET                         = 0x17,
   BSI_RCE_RESERVED_OFFSET                  = 0x19,

   BSI_CLUSTER_DATA_OFFSET                  = 0x50,
   BSI_CMB_SERIAL_NUMBER_OFFSET             = 0x50,
   BSI_CLUSTER_ADDR_OFFSET                  = 0x52,
   BSI_CLUSTER_GROUP_NAME_OFFSET            = 0x53,
   BSI_EXTERNAL_INTERCONNECT_OFFSET         = 0x5B,
   BSI_CLUSTER_RESERVED_OFFSET              = 0x5C,

   BSI_CLUSTER_SWITCH_CONFIGURATION_OFFSET  = 0x90,
   BSI_CLUSTER_SWITCH_RESERVED_OFFSET       = 0xA8,
   BSI_CLUSTER_ELEMENT_INTERCONNECT_OFFSET  = 0xA8,

   BSI_CLUSTER_IP_INFO_OFFSET               = 0xC0,
   BSI_CLUSTER_IP_INFO_RESERVED_OFFSET      = 0xC7,

   BSI_FABRIC_MAP_OFFSET                    = 0x100,
   BSI_FABRIC_MAP_RESERVED_OFFSET           = 0x181,


   BSI_BOOT_RESPONSE_OFFSET                 = 0x1FF,
   BSI_FIFO_OFFSET                          = 0x200,
} BSI_COMMON_CFG_OFFSETS;

/* Sizes of the various arrays */
enum {BSI_GROUP_NAME_SIZE                   = BSI_EXTERNAL_INTERCONNECT_OFFSET        - BSI_CLUSTER_GROUP_NAME_OFFSET};
enum {BSI_DOMAIN_NAME_SIZE                  = 16}; /* Hardcode for now */
enum {BSI_CLUSTER_SWITCH_CONFIGURATION_SIZE = BSI_CLUSTER_SWITCH_RESERVED_OFFSET
      - BSI_CLUSTER_SWITCH_CONFIGURATION_OFFSET};
enum {BSI_UBOOT_VERSION_SIZE                = BSI_DAT_VERSION_OFFSET                  - BSI_UBOOT_VERSION_OFFSET};
enum {BSI_DAT_VERSION_SIZE                  = BSI_DEVICE_DNA_OFFSET
      - BSI_DAT_VERSION_OFFSET}; 
enum {BSI_DEVICE_DNA_SIZE                   = BSI_EFUSE_OFFSET
      - BSI_DEVICE_DNA_OFFSET};
enum {BSI_EFUSE_SIZE                        = BSI_RCE_RESERVED_OFFSET
      - BSI_EFUSE_OFFSET};
enum {BSI_FABRIC_MAP_SIZE                   = BSI_FABRIC_MAP_RESERVED_OFFSET
      - BSI_FABRIC_MAP_OFFSET};

/* ---------------------------------------------------------------------- */
/*
  These are the utility values for extracting fields from the BSI
*/
/* ---------------------------------------------------------------------- */
#define BSI_CLUSTER_INFO_VERSION 1
/* The size of the fields in the cluster address word */
typedef enum _BSI_CLUSTER_ADDR_S
{
   BSI_CLUSTER_ADDR_S_VERSION = 8,
   BSI_CLUSTER_ADDR_S_CLUSTER = 8,
   BSI_CLUSTER_ADDR_S_BAY     = 8,
   BSI_CLUSTER_ADDR_S_ELEMENT = 8,
}  BSI_CLUSTER_ADDR_S;

/* Right justified bit offset of each of the cluster addr word fields */
typedef enum _BSI_CLUSTER_ADDR_V
{
   BSI_CLUSTER_ADDR_V_VERSION = 24,
   BSI_CLUSTER_ADDR_V_CLUSTER = 16,
   BSI_CLUSTER_ADDR_V_BAY     =  8,
   BSI_CLUSTER_ADDR_V_ELEMENT =  0,
}  BSI_CLUSTER_ADDR_V;

/* Masks for the cluster address word */
typedef enum _BSI_CLUSTER_ADDR_M
{
   BSI_CLUSTER_ADDR_M_VERSION = (0xff << BSI_CLUSTER_ADDR_V_VERSION),
   BSI_CLUSTER_ADDR_M_CLUSTER = (0xff << BSI_CLUSTER_ADDR_V_CLUSTER),
   BSI_CLUSTER_ADDR_M_BAY     = (0xff << BSI_CLUSTER_ADDR_V_BAY),
   BSI_CLUSTER_ADDR_M_ELEMENT = (0xff << BSI_CLUSTER_ADDR_V_ELEMENT),
}  BSI_CLUSTER_ADDR_M;

#define BSI_VERSION_FROM_CLUSTER_ADDR(cl_addr) \
   ((cl_addr & BSI_CLUSTER_ADDR_M_VERSION) >> BSI_CLUSTER_ADDR_V_VERSION)

#define BSI_ELEMENT_FROM_CLUSTER_ADDR(cl_addr) \
   ((cl_addr & BSI_CLUSTER_ADDR_M_ELEMENT) >> BSI_CLUSTER_ADDR_V_ELEMENT)

#define BSI_BAY_FROM_CLUSTER_ADDR(cl_addr) \
   ((cl_addr & BSI_CLUSTER_ADDR_M_BAY) >> BSI_CLUSTER_ADDR_V_BAY)

#define BSI_CLUSTER_FROM_CLUSTER_ADDR(cl_addr) \
   ((cl_addr & BSI_CLUSTER_ADDR_M_CLUSTER) >> BSI_CLUSTER_ADDR_V_CLUSTER)

typedef enum _BSI_CLUSTER_SWITCH_PORT_S
{
   BSI_CLUSTER_SWITCH_PORT_S_GROUP = 16,
   BSI_CLUSTER_SWITCH_PORT_S_TYPE  = 16,
}  BSI_CLUSTER_SWITCH_PORT_S;

/* Right justified bit offset of each of the cluster addr word fields */
typedef enum _BSI_CLUSTER_SWITCH_PORT_V
{
   BSI_CLUSTER_SWITCH_PORT_V_GROUP = 16,
   BSI_CLUSTER_SWITCH_PORT_V_TYPE  =  0,
}  BSI_CLUSTER_SWITCH_PORT_V;

/* Masks for the cluster address word */
typedef enum _BSI_CLUSTER_SWITCH_PORT_M
{
   BSI_CLUSTER_SWITCH_PORT_M_GROUP = (0xffff << BSI_CLUSTER_SWITCH_PORT_V_GROUP),
   BSI_CLUSTER_SWITCH_PORT_M_TYPE  = (0xffff << BSI_CLUSTER_SWITCH_PORT_V_TYPE),
}  BSI_CLUSTER_SWITCH_PORT_M;

#define GROUP_FROM_CLUSTER_SWITCH_PORT(port_def) \
   ((port_def & BSI_CLUSTER_SWITCH_PORT_M_GROUP) >> BSI_CLUSTER_SWITCH_PORT_V_GROUP)

#define TYPE_FROM_CLUSTER_SWITCH_PORT(port_def) \
   ((port_def & BSI_CLUSTER_SWITCH_PORT_M_TYPE) >> BSI_CLUSTER_SWITCH_PORT_V_TYPE)

#endif /* BSI_CFG_H */
