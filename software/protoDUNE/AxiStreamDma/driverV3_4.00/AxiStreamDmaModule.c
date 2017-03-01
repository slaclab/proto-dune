//////////////////////////////////////////////////////////////////////////////
// This file is part of 'AXI Stream DMA Core'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'AXI Stream DMA Core', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/sort.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/memory.h>

#define AXIS_IN_KERNEL 1
#include "AxiStreamDma.h"

#define FIFO_OFFSET 0x10000
#define REG_OFFSET  0x00000

#define RX_ENABLE_ADDR     (REG_OFFSET  + 0x000)
#define TX_ENABLE_ADDR     (REG_OFFSET  + 0x004)
#define FIFO_CLEAR_ADDR    (REG_OFFSET  + 0x008)
#define INT_ENABLE_ADDR    (REG_OFFSET  + 0x00C)
#define FIFO_VALID_ADDR    (REG_OFFSET  + 0x010)
#define MAX_RX_SIZE_ADDR   (REG_OFFSET  + 0x014)
#define ONLINE_ACK_ADDR    (REG_OFFSET  + 0x018)
#define INT_PENDING_ACK    (REG_OFFSET  + 0x01C)
#define RX_PEND_ADDR       (FIFO_OFFSET + 0x000)
#define TX_FREE_ADDR       (FIFO_OFFSET + 0x004)
#define RX_FREE_ADDR       (FIFO_OFFSET + 0x200)
#define TX_POST_ADDR_A     (FIFO_OFFSET + 0x240)
#define TX_POST_ADDR_B     (FIFO_OFFSET + 0x244)
#define TX_POST_ADDR_C     (FIFO_OFFSET + 0x248)
#define TX_POST_ADDR       (FIFO_OFFSET + 0x24C)

// Module Configuration
__u32 cfgTxCount[4] = {0,4,0,0};
__u32 cfgTxSize[4]  = {4096,4096,4096,4096};
__u32 cfgRxCount[4] = {1000,8,0,0};
__u32 cfgRxSize[4]  = {4096*4,4096,4096,4096};
__u32 cfgRxAcp[4]   = {0,0,0,0};

#define DRIVER_NAME "axi_stream_dma_drv"
#define MODULE_NAME "axi_stream_dma"

// Global variable for the device class 
static struct class *cl;

// Register structure
struct AxiStreamDmaReg  {
   __u8 * rxEnable;
   __u8 * txEnable;
   __u8 * fifoClear;
   __u8 * intEnable;
   __u8 * fifoValid;
   __u8 * maxRxSize;
   __u8 * onlineAck;
   __u8 * intPendAck;
   __u8 * rxPend;
   __u8 * txFree;
   __u8 * rxFree;
   __u8 * txPostA;
   __u8 * txPostB;
   __u8 * txPostC;
   __u8 * txPass;
};

// Buffer List
struct AxiStreamDmaBuffer {
   __u32       index;
   __u32       count;
   __u32       userHas;
   void      * buffAddr;
   dma_addr_t  buffHandle;
};

// Tracking Structure
struct AxiStreamDmaDevice {
   phys_addr_t   baseAddr;
   __u32         baseSize;
   __u8        * virtAddr;

	dev_t        devNum;
   const char * devName;

	struct cdev     charDev;
	struct device * device;

   __u32 idx;
   __u32 irq;
   wait_queue_head_t inq;
   wait_queue_head_t outq;

   struct fasync_struct *async_queue;

   // Buffers
   struct AxiStreamDmaBuffer ** txBuffers;
   struct AxiStreamDmaBuffer ** txSorted;
   struct AxiStreamDmaBuffer ** rxBuffers;
   struct AxiStreamDmaBuffer ** rxSorted;

   __u32 rxAcp;
   __u32 rxSize;
   __u32 rxCount;

   __u32 txSize;
   __u32 txCount;

   // Debug
   __u32 writeCount;
   __u32 readCount; 
   __u32 ackCount; 
   __u32 readSearchErrors;
   __u32 descErrors;
   __u32 axiWriteErrors;
   __u32 readSizeErrors;
   __u32 writeSizeErrors;
   __u32 writeSearchErrors;
   __u32 overFlows;
   __u32 badWriteCommands;
   __u32 badReadCommands;
   __u32 readPopCount;
   __u32 readPushCount;


   // Locks
   struct semaphore writeSem;
   struct semaphore readSem;

   // Register Space
   struct AxiStreamDmaReg reg; 

   // Linked List
	struct list_head devList;
 
};

// Create list
LIST_HEAD( fullDevList );

// Setup register pointers
void setRegisterPointers ( struct AxiStreamDmaDevice * dev ) {
   dev->reg.rxEnable     = dev->virtAddr + RX_ENABLE_ADDR;
   dev->reg.txEnable     = dev->virtAddr + TX_ENABLE_ADDR;
   dev->reg.fifoClear    = dev->virtAddr + FIFO_CLEAR_ADDR;
   dev->reg.intEnable    = dev->virtAddr + INT_ENABLE_ADDR;
   dev->reg.fifoValid    = dev->virtAddr + FIFO_VALID_ADDR;
   dev->reg.maxRxSize    = dev->virtAddr + MAX_RX_SIZE_ADDR;
   dev->reg.onlineAck    = dev->virtAddr + ONLINE_ACK_ADDR;
   dev->reg.intPendAck   = dev->virtAddr + INT_PENDING_ACK;
   dev->reg.rxPend       = dev->virtAddr + RX_PEND_ADDR;
   dev->reg.txFree       = dev->virtAddr + TX_FREE_ADDR;
   dev->reg.rxFree       = dev->virtAddr + RX_FREE_ADDR;
   dev->reg.txPostA      = dev->virtAddr + TX_POST_ADDR_A;
   dev->reg.txPostB      = dev->virtAddr + TX_POST_ADDR_B;
   dev->reg.txPostC      = dev->virtAddr + TX_POST_ADDR_C;
   dev->reg.txPass       = dev->virtAddr + TX_POST_ADDR;
}

// Binary Search
void *bsearch(const void *key, const void *base, size_t num, size_t size,
	      int (*cmp)(const void *key, const void *elt))
{
	int start = 0, end = num - 1, mid, result;
	if (num == 0)
		return NULL;

	while (start <= end) {
		mid = (start + end) / 2;
		result = cmp(key, base + mid * size);
		if (result < 0)
			end = mid - 1;
		else if (result > 0)
			start = mid + 1;
		else
			return (void *)base + mid * size;
	}

	return NULL;
}

// Buffer comparison for sort
__s32 bufferSortComp (const void *p1, const void *p2) {
   struct AxiStreamDmaBuffer ** b1 = (struct AxiStreamDmaBuffer **)p1;
   struct AxiStreamDmaBuffer ** b2 = (struct AxiStreamDmaBuffer **)p2;

   if ( (*b1)->buffHandle > (*b2)->buffHandle ) return 1;
   if ( (*b1)->buffHandle < (*b2)->buffHandle ) return -1;
   return(0);
}

// Buffer comparison for search
__s32 bufferSearchComp (const void *key, const void *element) {
   struct AxiStreamDmaBuffer ** buff = (struct AxiStreamDmaBuffer **)element;

   dma_addr_t value = *((dma_addr_t *)key);

   if ( (*buff)->buffHandle < value ) return 1;
   if ( (*buff)->buffHandle > value ) return -1;
   return(0);
}

// Find a buffer from a received handle
__s32 findBuffer (struct AxiStreamDmaDevice *dev, __u32 tx, dma_addr_t handle ) {
   __u32 count;

   struct AxiStreamDmaBuffer ** list;
   struct AxiStreamDmaBuffer ** result;

   if ( tx == 1 ) {
      list  = dev->txSorted;
      count = dev->txCount;
   } else {
      list  = dev->rxSorted;
      count = dev->rxCount;
   }

   result = (struct AxiStreamDmaBuffer **) 
      bsearch(&handle, list, count, sizeof(struct AxiStreamDmaBuffer *), bufferSearchComp);

   if ( result == NULL ) return(-1);
   return((*result)->index);
}

// Find device in the list that matches current inode
static struct AxiStreamDmaDevice *findDevice(struct inode *i) {
	struct list_head          * pos;
	struct AxiStreamDmaDevice * dev = NULL;
	struct AxiStreamDmaDevice * tmp = NULL;

	list_for_each( pos, &fullDevList ) {

    	tmp = list_entry( pos, struct AxiStreamDmaDevice, devList );

    	if (tmp->devNum == i->i_rdev) {
    		dev = tmp;
    		break;
    	}
  	}
  	return dev;	
}

static int AxiStreamDmaFasync(int fd, struct file *f, int mode) {
   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> fasync: Bad inode. \n", MODULE_NAME);
      return(0);
   }

    return fasync_helper(fd, f, mode, &dev->async_queue);
}

// Open the device
static int AxiStreamDmaOpen (struct inode *i, struct file *f) {
   return(0);
}

// Close the device
static int AxiStreamDmaClose (struct inode *i, struct file *f) {
   __u32 x;

   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> poll: Bad inode.\n", MODULE_NAME);
      return(ERR_DRIVER);
   }

   if ( down_interruptible(&dev->readSem)) return -ERESTARTSYS; 

   // Cleanup any buffers that the user did not return
   for (x=0; x < dev->rxCount; x++) {
      if ( dev->rxBuffers[x]->userHas ) {
         dev->rxBuffers[x]->userHas = 0;
         dev->readPushCount++;
         iowrite32(dev->rxBuffers[x]->buffHandle,dev->reg.rxFree);
      }
   }

   if (dev->async_queue) AxiStreamDmaFasync(-1,f,0);

   up(&dev->readSem);

   return(0);
}

// IRQ Handler
static irqreturn_t AxiStreamDmaIrq(int irq, void *dev_id) {
   __u32       stat;
   irqreturn_t ret;

   struct AxiStreamDmaDevice * dev = (struct AxiStreamDmaDevice *)dev_id;

   // Read IRQ Status
   stat = ioread32(dev->reg.intPendAck);

   // Is this the source
   if ( stat != 0 ) {

      // Wake up any readers
      if ( (stat & 0x1) != 0 ) {
         wake_up_interruptible(&(dev->inq));
         if (dev->async_queue) kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
      }

      // Wake up any writers
      if ( (stat & 0x2) != 0 ) wake_up_interruptible(&(dev->outq));

      // Ack Interrupt
      iowrite32(0x1,dev->reg.intPendAck);

      ret = IRQ_HANDLED;
   }
   else ret = IRQ_NONE;

   return(ret);
}

// Poll/Select
static __u32 AxiStreamDmaPoll(struct file *f, poll_table *wait ) {
   __u32 mask = 0;
   __u32 stat;

   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> poll: Bad inode.\n", MODULE_NAME);
      return(ERR_DRIVER);
   }

   poll_wait(f,&(dev->inq),wait);
   poll_wait(f,&(dev->outq),wait);

   // Read FIFO status
   stat = ioread32(dev->reg.fifoValid);

   // Readable?
   if ( (stat & 0x1) != 0 ) mask |= POLLIN | POLLRDNORM;

   // Writable??
   if ( (stat & 0x2) != 0 ) mask |= POLLOUT | POLLWRNORM;

   return(mask);
}

static ssize_t AxiStreamDmaRead(struct file *f, char __user * buf, size_t len, loff_t * off) {
   dma_addr_t handle;
   __u32      status;
   __u32      size;
   __s32      idx;
   ssize_t    ret = 0;

   struct AxiStreamDmaRead   * entry;
   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> read: Bad inode. Size %i\n", MODULE_NAME,len);
      return(ERR_DRIVER);
   }

   if ( down_interruptible(&dev->readSem)) return -ERESTARTSYS; 

   // Size must match structure
   if ( len != sizeof(struct AxiStreamDmaRead) ) {
      dev->badReadCommands++;
      up(&dev->readSem);
      return (ERR_DRIVER);
   }
   entry = (struct AxiStreamDmaRead *)buf;

   // Decode Command
   switch (entry->command) {

      // Get buffer size
      case CMD_GET_BSIZE :
         ret = dev->rxSize;
         break;

      // Get buffer count
      case CMD_GET_BCOUNT :
         ret = dev->rxCount;
         break;

      // Check if read is ready
      case CMD_READ_READY :
         handle = ioread32(dev->reg.fifoValid);
         ret = (handle & 0x1);
         break;

      // Ack Read, pulse ack bit
      case CMD_READ_ACK :
         iowrite32(0x3,dev->reg.onlineAck);
         iowrite32(0x1,dev->reg.onlineAck);
         dev->ackCount++;
         ret = 0;
         break;

      // Read Data
      case CMD_READ_COPY :
      case CMD_READ_USER :

         // Receive queue entry
         while ( ((handle = ioread32(dev->reg.rxPend)) & 0x80000000) == 0 ) {
            if ( f->f_flags & O_NONBLOCK ) {
               up(&dev->readSem);
               return(0);
            }
            wait_event_interruptible(dev->inq, ( (ioread32(dev->reg.rxPend) & 0x80000000) != 0 ));
         }
         handle &= 0x7FFFFFFF;

         // Read size
         do { 
            size = ioread32(dev->reg.rxPend);
         } while ((size & 0x80000000) == 0);

         // Bad marker
         if ( (size & 0xFF000000) != 0xE0000000 ) {
            printk(KERN_INFO "<%s> read: Bad FIFO size marker 0x%.8x\n", dev->devName,size);
            dev->descErrors++;
            return(ERR_DRIVER);
         }
         size &= 0xFFFFFF;

         // Get status
         do { 
            status = ioread32(dev->reg.rxPend);
         } while ((status & 0x80000000) == 0);

         // Bad marker
         if ( (status & 0xF0000000) != 0xF0000000 ) {
            printk(KERN_INFO "<%s> read: Bad FIFO status marker 0x%.8x\n", dev->devName,size);
            dev->descErrors++;
            return(ERR_DRIVER);
         }

         // Find Buffer
         if ( (idx = findBuffer (dev,0,handle)) < 0 ) {
            printk(KERN_INFO "<%s> read: Error finding receive buffer 0x%.8x\n", dev->devName,handle);
            dev->readSearchErrors++;
            up(&dev->readSem);
            return(ERR_DRIVER);
         }
         dev->rxBuffers[idx]->count++;
         dev->readPopCount++;
         ret = size;

         // Update record
         entry->fUser = (status >>  8) & 0xFF;
         entry->lUser = (status >> 16) & 0xFF;
         entry->dest  = (status      ) & 0xFF;

         // Check for errors
         if ( (status & 0x01000000) != 0 ) {
            ret = ERR_AXI_WRITE;
            dev->axiWriteErrors++;
         }
         if ( (status & 0x02000000) != 0 ) {
            ret = ERR_DMA_OFLOW;
            dev->overFlows++;
         }

         // Copy data and return descriptor if a read command
         if ( entry->command == CMD_READ_COPY ) {
            if ( entry->size < size ) {
               ret = ERR_BUFF_OFLOW;
               dev->readSizeErrors++;
            }
            else {
               copy_to_user(entry->buffer, dev->rxBuffers[idx]->buffAddr, size);
               dev->readPushCount++;
               iowrite32(dev->rxBuffers[idx]->buffHandle,dev->reg.rxFree);
            }
         }

         // Return user space index
         else {
            dev->rxBuffers[idx]->userHas = 1;
            entry->index = idx;
         }
         dev->readCount++;
         break;

      // Post entry
      case CMD_POST_USER :
         if ( (idx = entry->index) >= dev->rxCount ) {
            printk(KERN_INFO "<%s> read: Invalid index posted: %i\n", dev->devName,entry->index);
            dev->readSearchErrors++;
            up(&dev->readSem);
            return(ERR_DRIVER);
         }
         if ( dev->rxBuffers[idx]->userHas ) {
            iowrite32(dev->rxBuffers[idx]->buffHandle,dev->reg.rxFree);
            dev->readPushCount++;
         } else {
            printk(KERN_INFO "<%s> read: User posted entry that was already released: %i\n", dev->devName,entry->index);
         }
         dev->rxBuffers[idx]->userHas = 0;
         ret = 0;
         break;

      default :
         dev->badReadCommands++;
         ret = ERR_DRIVER;
         break;
   }
   up(&dev->readSem);
   return(ret);
}

static ssize_t AxiStreamDmaWrite(struct file *f, const char __user * buf, size_t len, loff_t * off) {
   dma_addr_t handle;
   __u32      control;
   __s32      idx;
   ssize_t    ret = 0;

   struct AxiStreamDmaWrite  * entry;
   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> write: Bad inode. Size %i\n", MODULE_NAME,len);
      return(ERR_DRIVER);
   }

   if ( down_interruptible(&dev->writeSem)) return -ERESTARTSYS; 

   // Size must match structure
   if ( len != sizeof(struct AxiStreamDmaWrite) ) {
      dev->badWriteCommands++;
      up(&dev->writeSem);
      return(ERR_DRIVER);
   }
   entry = (struct AxiStreamDmaWrite *)buf;

   // Decode Command
   switch (entry->command) {

      // Get buffer size
      case CMD_GET_BSIZE :
         ret = dev->txSize;
         break;

      // Get buffer count
      case CMD_GET_BCOUNT :
         ret = dev->txCount;
         break;

      // Write Data
      case CMD_WRITE_DATA  :

         if (entry->size > dev->txSize) {
            printk(KERN_INFO "<%s> write: Bad tx length. %i. Max = %i\n", dev->devName,entry->size,dev->txSize);
            dev->writeSizeErrors++;
            up(&dev->writeSem);
            return(ERR_DRIVER);
         }

         // Get handle;
         while ( ((handle = ioread32(dev->reg.txFree)) & 0x80000000) == 0 ) {
            if ( f->f_flags & O_NONBLOCK ) {
               up(&dev->writeSem);
               return(0);
            }
            wait_event_interruptible(dev->outq, ( (ioread32(dev->reg.txFree) & 0x80000000) != 0 ));
         }
         handle &= 0x7FFFFFFF;

         // Get buffer
         if ( (idx = findBuffer (dev,1,handle)) < 0 ) {
            printk(KERN_INFO "<%s> write: Error finding transmit buffer 0x%.8x\n", dev->devName,handle);
            dev->writeSearchErrors++;
            up(&dev->writeSem);
            return(ERR_DRIVER);
         }
         dev->txBuffers[idx]->count++;

         // Generate Control
         control  = (entry->dest       ) & 0x000000FF;
         control += (entry->fUser <<  8) & 0x0000FF00;
         control += (entry->lUser << 16) & 0x00FF0000;

         copy_from_user(dev->txBuffers[idx]->buffAddr, entry->buffer, entry->size);

         iowrite32(dev->txBuffers[idx]->buffHandle,dev->reg.txPostA);
         iowrite32(entry->size,dev->reg.txPostB);
         iowrite32(control,dev->reg.txPostC);

         dev->writeCount++;
         ret = entry->size;
         break;

      default :
         dev->badWriteCommands++;
         ret = ERR_DRIVER;
         break;
   }

   up(&dev->writeSem);
   return(ret);
}

// Memory map
__s32 AxiStreamDmaMap(struct file *filp, struct vm_area_struct *vma) {
   struct AxiStreamDmaDevice * dev;

   __u32 offset;
   __u32 vsize;
   __u32 idx;
   __s32 ret;

   dev = findDevice(filp->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> map: Bad inode.\n", MODULE_NAME);
      return(-1);
   }

   // Figure out offset and size
   offset = vma->vm_pgoff << PAGE_SHIFT;
   vsize  = vma->vm_end - vma->vm_start;

   // After we use the offset to figure out the index, we must zero it out so
   // the map call will map to the start of our space from dma_alloc_coherent()
   vma->vm_pgoff = 0;

   // Size must match the receive buffer size and offset must be size aligned
   if ( vsize != dev->rxSize || (offset % dev->rxSize) != 0 ) {
      printk(KERN_INFO "<%s> map: Invalid map size (%i) and offset (%i) \n", 
            dev->devName,vsize,offset);
      return(ERR_DRIVER);
   }

   // Compute and check index
   if ( (idx = offset / dev->rxSize) > dev->rxCount ) {
      printk(KERN_INFO "<%s> map: Computed index (%i) out of range\n", 
            dev->devName,idx);
      return(ERR_DRIVER);
   }

   // Map
   if ( dev->rxAcp ) {
      ret = remap_pfn_range(vma, 
                            vma->vm_start, 
                            virt_to_phys((void *)dev->rxBuffers[idx]->buffAddr) >> PAGE_SHIFT,
                            vsize,
                            vma->vm_page_prot);
   } else {
      ret = dma_mmap_coherent(dev->device,vma, dev->rxBuffers[idx]->buffAddr,
                              dev->rxBuffers[idx]->buffHandle,dev->rxSize);
   }

   if ( ret < 0 )
      printk(KERN_INFO "<%s> map: Failed to map. start 0x%.8x, end 0x%.8x, offset %i, size %i, index %i. Ret=%i\n", 
         dev->devName,(__u32)vma->vm_start,(__u32)vma->vm_end,offset,vsize,idx,ret);

   return (ret);
}

static struct file_operations fops = {
	.owner   = THIS_MODULE,
   .llseek  = NULL,
	.read    = AxiStreamDmaRead,
	.write   = AxiStreamDmaWrite,
//   .readdir = NULL,
   .poll    = AxiStreamDmaPoll,
   .mmap    = AxiStreamDmaMap,
	.open    = AxiStreamDmaOpen,
	.release = AxiStreamDmaClose,
   .fsync   = NULL,
   .fasync  = AxiStreamDmaFasync,
   .lock    = NULL
};

int AxiStreamDmaProcRead(struct file *file, char __user *buf, size_t size, loff_t *ppos)  {
   __s32 len;
   __s32 x;
   __s32 cnt;
   __s32 max;
   __s32 min;
   __s32 sum;
   __s32 avg;
   static bool sent = false;

   struct AxiStreamDmaDevice *dev = PDE_DATA(file_inode(file));

   len = 0;

   len += sprintf(buf+len,"\nStatus of device %s. Driver Version 3\n",dev->devName);
   len += sprintf(buf+len,"\n-------------- Read Buffers ---------------\n\n");
   len += sprintf(buf+len,"    Read Buffer Count : %i\n",dev->rxCount);
   len += sprintf(buf+len,"     Read Buffer Size : %i\n",dev->rxSize);

   cnt = 0;
   max = 0;
   min = 0xFFFFFFFF;
   sum = 0;
   for (x=0; x < dev->rxCount; x++) {
      if ( dev->rxBuffers[x]->count > max ) max = dev->rxBuffers[x]->count;
      if ( dev->rxBuffers[x]->count < min ) min = dev->rxBuffers[x]->count;
      if ( dev->rxBuffers[x]->userHas ) cnt++;
      sum += dev->rxBuffers[x]->count;
   }
   if ( dev->rxCount == 0 ) {
      min = 0;
      avg = 0;
   }
   else avg = sum/dev->rxCount;

   len += sprintf(buf+len,"       Read Pop Count : %i\n",dev->readPopCount);
   len += sprintf(buf+len,"      Read Push Count : %i\n",dev->readPushCount);
   len += sprintf(buf+len," Read Buffers In User : %i\n",cnt);
   len += sprintf(buf+len,"  Min Read Buffer Use : %i\n",min);
   len += sprintf(buf+len,"  Max Read Buffer Use : %i\n",max);
   len += sprintf(buf+len,"  Avg Read Buffer Use : %i\n",avg);
   len += sprintf(buf+len,"  Tot Read Buffer Use : %i\n",sum);

   len += sprintf(buf+len,"\n-------------- Read Counters --------------\n\n");
   len += sprintf(buf+len,"           Read Count : %i\n",dev->readCount);
   len += sprintf(buf+len,"            Ack Count : %i\n",dev->ackCount);
   len += sprintf(buf+len,"    Descriptor Errors : %i\n",dev->descErrors);
   len += sprintf(buf+len,"   Read Search Errors : %i\n",dev->readSearchErrors);
   len += sprintf(buf+len,"     Read Size Errors : %i\n",dev->readSizeErrors);
   len += sprintf(buf+len,"     AXI Write Errors : %i\n",dev->axiWriteErrors);
   len += sprintf(buf+len,"    Inbound Overflows : %i\n",dev->overFlows);
   len += sprintf(buf+len,"    Bad Read Commands : %i\n",dev->badReadCommands);

   len += sprintf(buf+len,"\n-------------- Write Buffers --------------\n\n");
   len += sprintf(buf+len,"   Write Buffer Count : %i\n",dev->txCount);
   len += sprintf(buf+len,"    Write Buffer Size : %i\n",dev->txSize);

   max = 0;
   min = 0xFFFFFFFF;
   sum = 0;
   for (x=0; x < dev->txCount; x++) {
      if ( dev->txBuffers[x]->count > max ) max = dev->txBuffers[x]->count;
      if ( dev->txBuffers[x]->count < min ) min = dev->txBuffers[x]->count;
      sum += dev->txBuffers[x]->count;
   }
   if ( dev->txCount == 0 ) {
      min = 0;
      avg = 0;
   }
   else avg = sum/dev->txCount;

   len += sprintf(buf+len," Min Write Buffer Use : %i\n",min);
   len += sprintf(buf+len," Max Write Buffer Use : %i\n",max);
   len += sprintf(buf+len," Avg Write Buffer Use : %i\n",avg);
   len += sprintf(buf+len," Tot Write Buffer Use : %i\n",sum);

   len += sprintf(buf+len,"\n-------------- Write Counters -------------\n\n");
   len += sprintf(buf+len,"          Write Count : %i\n",dev->writeCount);
   len += sprintf(buf+len,"  Write Search Errors : %i\n",dev->writeSearchErrors);
   len += sprintf(buf+len,"    Write Size Errors : %i\n",dev->writeSizeErrors);
   len += sprintf(buf+len,"   Bad Write Commands : %i\n",dev->badWriteCommands);

   len += sprintf(buf+len,"\n-------------- Generic --------------------\n\n");
   len += sprintf(buf+len,"             Writable : %i\n",((ioread32(dev->reg.fifoValid) >> 1) & 0x1));
   len += sprintf(buf+len,"             Readable : %i\n",(ioread32(dev->reg.fifoValid) & 0x1));
   len += sprintf(buf+len,"     Write Int Status : %i\n",((ioread32(dev->reg.intPendAck) >> 1) & 0x1));
   len += sprintf(buf+len,"      Read Int Status : %i\n",(ioread32(dev->reg.intPendAck) & 0x1));
   len += sprintf(buf+len,"             Read Sem : %i\n",dev->readSem.count);
   len += sprintf(buf+len,"            Write Sem : %i\n",dev->writeSem.count);

   len += sprintf(buf+len,"\n");

   for (x=0; x < dev->rxCount; x++) {
      len += sprintf(buf+len,"Buffer idx %i, kaddr=0x%x, paddr=0x%x\n",x,dev->rxBuffers[x]->buffAddr,dev->rxBuffers[x]->buffHandle);
   }

   // Emulate the legacy "*eof=1;" functionality 
   if(!sent){
      sent = true;
      return(len);
   } else {
      sent = false;
      return(0);      
   }
}

struct file_operations proc_fops = {
   read:  AxiStreamDmaProcRead
};

static int AxiStreamDmaProbe(struct platform_device *pdev) {
   __u32 x;
   __u32 count;

	struct AxiStreamDmaDevice *dev;
	dev = (struct AxiStreamDmaDevice *) kmalloc( sizeof(struct AxiStreamDmaDevice), GFP_KERNEL );

   // Init locks
   sema_init(&dev->writeSem,1);
   sema_init(&dev->readSem,1);

   dev->writeCount = 0;
   dev->readCount = 0; 
   dev->ackCount = 0;
   dev->descErrors = 0;
   dev->readSearchErrors = 0;
   dev->axiWriteErrors = 0;
   dev->readSizeErrors = 0;
   dev->writeSizeErrors = 0;
   dev->writeSearchErrors = 0;
   dev->overFlows = 0;
   dev->badReadCommands = 0;
   dev->badWriteCommands = 0;
   dev->readPopCount = 0;
   dev->readPushCount = 0;

   dev->baseAddr = pdev->resource[0].start;
   dev->baseSize = (pdev->resource[0].end - pdev->resource[0].start) + 1;
	dev->devName  = pdev->name + 9;
   dev->device   = &pdev->dev;

   // Determine idx
   if ( strcmp(dev->devName,"axi_stream_dma_0") == 0 ) dev->idx = 0;
   else if ( strcmp(dev->devName,"axi_stream_dma_1") == 0 ) dev->idx = 1;
   else if ( strcmp(dev->devName,"axi_stream_dma_2") == 0 ) dev->idx = 2;
   else dev->idx = 3;

   // Get and map register space
   // ---------------------------------------------------------------
   // -- 2017.02.14 -- jjr
   // --------------------
   // Commented out the following line per Sergio's instructions
   // This causes compilation warnings which causes the build to fail
   // ---------------------------------------------------------------
   //// !!!! if (check_mem_region(dev->baseAddr,dev->baseSize) ) return(-1);
   if ( request_mem_region(dev->baseAddr,dev->baseSize,MODULE_NAME) == NULL ) return(-1);
   dev->virtAddr = (char *) ioremap_nocache(dev->baseAddr,dev->baseSize);
   setRegisterPointers(dev);

	if (alloc_chrdev_region(&(dev->devNum), 0, 1, dev->devName) < 0) {
		return -1;
	}

	if (cl == NULL && (cl = class_create(THIS_MODULE, "chardrv")) == NULL) {
		unregister_chrdev_region(dev->devNum, 1);
		return -1;
	}

	if (device_create(cl, NULL, dev->devNum, NULL, dev->devName) == NULL) {
		class_destroy(cl);
		unregister_chrdev_region(dev->devNum, 1);
		return -1;
	}

	cdev_init(&(dev->charDev), &fops);

	if (cdev_add(&(dev->charDev), dev->devNum, 1) == -1) {
		device_destroy(cl, dev->devNum) ;
		class_destroy(cl);              
		unregister_chrdev_region(dev->devNum, 1);
		return -1;                     
	}                                  

   // Configure Sizes
   dev->rxAcp   = cfgRxAcp[dev->idx];
   dev->rxCount = cfgRxCount[dev->idx];
   dev->rxSize  = cfgRxSize[dev->idx];
   dev->txCount = cfgTxCount[dev->idx];
   dev->txSize  = cfgTxSize[dev->idx];

   // Set MAX RX                      
   iowrite32(dev->rxSize,dev->reg.maxRxSize);
                                      
   // Clear FIFOs                     
   iowrite32(0x1,dev->reg.fifoClear); 
   iowrite32(0x0,dev->reg.fifoClear); 

   // Enable rx and tx
   iowrite32(0x1,dev->reg.rxEnable);
   iowrite32(0x1,dev->reg.txEnable);

	printk(KERN_INFO "<%s> init: Creating %i RX Buffers. Size=%i Bytes\n", dev->devName,dev->rxCount,dev->rxSize);
   if ( dev->rxCount > 0 ) {
      dev->rxBuffers = (struct AxiStreamDmaBuffer **) 
         kmalloc( (sizeof(struct AxiStreamDmaBuffer *) * dev->rxCount), GFP_KERNEL );
      dev->rxSorted = (struct AxiStreamDmaBuffer **) 
         kmalloc( (sizeof(struct AxiStreamDmaBuffer *) * dev->rxCount), GFP_KERNEL );
   }

   // Allocate RX buffers
   count = 0;
   for (x=0; x < dev->rxCount; x++) {
      dev->rxBuffers[x] = (struct AxiStreamDmaBuffer *) 
         kmalloc( sizeof(struct AxiStreamDmaBuffer), GFP_KERNEL );

      if ( dev->rxAcp ) {
         if ( (dev->rxBuffers[x]->buffAddr = kmalloc(dev->rxSize, GFP_DMA | GFP_KERNEL)) != NULL)
            dev->rxBuffers[x]->buffHandle = virt_to_phys(dev->rxBuffers[x]->buffAddr);
      } else {
         dev->rxBuffers[x]->buffAddr = 
            dma_alloc_coherent(dev->device, dev->rxSize, &(dev->rxBuffers[x]->buffHandle), GFP_KERNEL);
      }

      if ( dev->rxBuffers[x]->buffAddr == NULL ) break;

      dev->rxBuffers[x]->userHas  = 0;
      dev->rxBuffers[x]->count    = 0;
      dev->rxBuffers[x]->index    = x;

      dev->rxSorted[x] = dev->rxBuffers[x];

      iowrite32(dev->rxBuffers[x]->buffHandle,dev->reg.rxFree);
      count++;
   }

   // Sort the buffers
   if ( count > 0 ) sort(dev->rxSorted,count,sizeof(struct AxiStreamDmaBuffer *),bufferSortComp,NULL);

	printk(KERN_INFO "<%s> init: Created  %i out of %i RX Buffers. %i Bytes\n", 
         dev->devName,count,dev->rxCount,(count*dev->rxSize));
   dev->rxCount = count;

	printk(KERN_INFO "<%s> init: Creating %i TX Buffers. Size=%i Bytes\n", dev->devName,dev->txCount,dev->txSize);
   if ( dev->txCount > 0 ) {
      dev->txBuffers = (struct AxiStreamDmaBuffer **) 
         kmalloc( (sizeof(struct AxiStreamDmaBuffer *) * dev->txCount), GFP_KERNEL );
      dev->txSorted = (struct AxiStreamDmaBuffer **) 
         kmalloc( (sizeof(struct AxiStreamDmaBuffer *) * dev->txCount), GFP_KERNEL );
   }

   // Allocate TX buffers
   count = 0;
   for (x=0; x < dev->txCount; x++) {
      dev->txBuffers[x] = (struct AxiStreamDmaBuffer *) 
         kmalloc( sizeof(struct AxiStreamDmaBuffer), GFP_KERNEL );

      dev->txBuffers[x]->buffAddr = 
         dma_alloc_coherent(dev->device, dev->txSize, &(dev->txBuffers[x]->buffHandle), GFP_KERNEL);

      if ( dev->txBuffers[x]->buffAddr == NULL ) break;

      dev->txBuffers[x]->userHas  = 0;
      dev->txBuffers[x]->count    = 0;
      dev->txBuffers[x]->index    = x;

      dev->txSorted[x] = dev->txBuffers[x];

      iowrite32(dev->txBuffers[x]->buffHandle,dev->reg.txPass);
      count++;
   }

   // Sort the buffers
   if ( count > 0 ) sort(dev->txSorted,dev->txCount,sizeof(struct AxiStreamDmaBuffer *),bufferSortComp,NULL);

	printk(KERN_INFO "<%s> init: Created  %i out of %i TX Buffers. %i Bytes\n", 
         dev->devName,count,dev->txCount,(count*dev->txSize));
   dev->txCount = count;

   // Get IRQ from pci_dev structure. 
   dev->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);;
   printk(KERN_INFO "<%s> Init: IRQ %d\n", dev->devName, dev->irq);

   // Request IRQ from OS.
   if (request_irq( dev->irq, AxiStreamDmaIrq, 0, MODULE_NAME, (void*)dev) < 0 ) {
      printk(KERN_WARNING"<%s> Init: Unable to allocate IRQ.",dev->devName);
      return (-1);
   }



   /* 
    | 2017.02.17 -- jjr
    | -----------------
    | The following was added per Sergio's request. This deals
    | with a coherency problem in the ACP
   */
   if(dev->rxAcp || of_dma_is_coherent(pdev->dev.of_node)) {
       set_dma_ops(&pdev->dev,&arm_coherent_dma_ops);
   }

   // Setup /proc
   proc_create_data(dev->devName,0444,NULL,&proc_fops,dev);

   // Init queues
   init_waitqueue_head(&(dev->inq));
   init_waitqueue_head(&(dev->outq));
   dev->async_queue = NULL;

   // Enable interrupt
   iowrite32(0x1,dev->reg.intPendAck);
   iowrite32(0x1,dev->reg.intEnable);

   // Online bits = 1, Ack bit = 0
   iowrite32(0x1,dev->reg.onlineAck);

	list_add( &dev->devList, &fullDevList );
	return 0;
}

static int AxiStreamDmaRemove(struct platform_device *pdev) {
   struct list_head *pos, *q;
   struct AxiStreamDmaDevice *dev;
   __u32 x;

   list_for_each_safe(pos, q, &fullDevList) {

      dev = list_entry(pos,struct AxiStreamDmaDevice,devList);

      if ( !strcmp(dev->devName,pdev->name + 9) ) {
         list_del (pos);
         cdev_del(&(dev->charDev));
    		device_destroy(cl, dev->devNum);
    		unregister_chrdev_region(dev->devNum, 1);

         // Disable interrupt
         iowrite32(0x0,dev->reg.intEnable);

         // Clear FIFOs
         iowrite32(0x1,dev->reg.fifoClear);

         // Disable rx and tx
         iowrite32(0x0,dev->reg.rxEnable);
         iowrite32(0x0,dev->reg.txEnable);
         iowrite32(0x0,dev->reg.onlineAck);

         // De-Allocate TX buffers
         for (x=0; x < dev->txCount; x++) {
            if ( dev->txBuffers[x]->buffAddr != NULL )
               dma_free_coherent(dev->device, dev->txSize, 
                     dev->txBuffers[x]->buffAddr,dev->txBuffers[x]->buffHandle);
            kfree(dev->txBuffers[x]);
         }
         if ( dev->txCount > 0 ) {
            kfree(dev->txBuffers);
            kfree(dev->txSorted);
         }
        
         // De-Allocate RX buffers
         for (x=0; x < dev->rxCount; x++) {
            if ( dev->rxBuffers[x]->buffAddr != NULL ) {
               if ( dev->rxAcp ) {
                  kfree(dev->rxBuffers[x]->buffAddr);
               } else {
                  dma_free_coherent(dev->device, dev->rxSize, 
                        dev->rxBuffers[x]->buffAddr,dev->rxBuffers[x]->buffHandle);
               }
            }
            kfree(dev->rxBuffers[x]);
         }
         if ( dev->rxCount > 0 ) {
            kfree(dev->rxBuffers);
            kfree(dev->rxSorted);
         }

         // Cleanup proc
         remove_proc_entry(dev->devName,NULL);

         // Release register space
         iounmap(dev->virtAddr);
         release_mem_region(dev->baseAddr,dev->baseSize);

         // Release IRQ
         free_irq(dev->irq, dev);

         // Free device area
         kfree(dev);
    		break;
    	}
  	}

  	if (list_empty(&fullDevList)) class_destroy(cl);
	printk(KERN_INFO "<%s> exit: unregistered\n", MODULE_NAME);
	return 0;
}

static int AxiStreamDmaRuntimeNop(struct device *dev) {
   return 0;
}

static const struct dev_pm_ops AxiStreamDmaOps = {
   .runtime_suspend = AxiStreamDmaRuntimeNop,
   .runtime_resume = AxiStreamDmaRuntimeNop,
};

static struct of_device_id AxiStreamDmaMatch[] = {
   { .compatible = MODULE_NAME, },
   { /* This is filled with module_parm */ },
   { /* Sentinel */ },
};

// Parameters
module_param_array(cfgTxCount,uint,0,0);
MODULE_PARM_DESC(cfgTxCount, "TX buffer count");

module_param_array(cfgTxSize,uint,0,0);
MODULE_PARM_DESC(cfgTxSize, "TX buffer size");

module_param_array(cfgRxCount,uint,0,0);
MODULE_PARM_DESC(cfgRxCount, "RX buffer count");

module_param_array(cfgRxSize,uint,0,0);
MODULE_PARM_DESC(cfgRxSize, "RX buffer size");

module_param_array(cfgRxAcp,uint,0,0);
MODULE_PARM_DESC(cfgRxAcp, "RX use ACP");

MODULE_DEVICE_TABLE(of, AxiStreamDmaMatch);
module_param_string(of_id, AxiStreamDmaMatch[1].compatible, 128, 0);
MODULE_PARM_DESC(of_id, "Id of the device to be handled by driver");

static struct platform_driver AxiStreamDmaPdrv = {
   .probe  = AxiStreamDmaProbe,
   .remove = AxiStreamDmaRemove,
   .driver = {
      .name = DRIVER_NAME,
      .owner = THIS_MODULE,
      .pm = &AxiStreamDmaOps,
      .of_match_table = of_match_ptr(AxiStreamDmaMatch),
   },
};

module_platform_driver(AxiStreamDmaPdrv);

MODULE_AUTHOR("Ryan Herbst");
MODULE_DESCRIPTION("AXI Stream DMA driver. V3");
MODULE_LICENSE("GPL v2");

