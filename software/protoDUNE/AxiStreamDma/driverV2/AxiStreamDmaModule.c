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
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mm.h>
#include <asm/io.h>

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
#define ENABLE_ONLINE_ADDR (REG_OFFSET  + 0x018)
#define INT_PENDING_ACK    (REG_OFFSET  + 0x01C)
#define RX_PEND_ADDR       (FIFO_OFFSET + 0x000)
#define TX_FREE_ADDR       (FIFO_OFFSET + 0x004)
#define RX_FREE_ADDR       (FIFO_OFFSET + 0x200)
#define TX_POST_ADDR_A     (FIFO_OFFSET + 0x240)
#define TX_POST_ADDR_B     (FIFO_OFFSET + 0x244)
#define TX_POST_ADDR_C     (FIFO_OFFSET + 0x248)
#define TX_POST_ADDR       (FIFO_OFFSET + 0x24C)

#define TX_BUFFER_COUNT 4
#define RX_BUFFER_COUNT 8

#define TX_BUFFER_SIZE (1024*1024)/2
#define RX_BUFFER_SIZE (1024*1024)/2

#define DRIVER_NAME "axi_stream_dma_drv"
#define MODULE_NAME "axi_stream_dma"

// Global variable for the device class 
static struct class *cl;

// Register structure
struct AxiStreamDmaReg  {
   char * rxEnable;
   char * txEnable;
   char * fifoClear;
   char * intEnable;
   char * fifoValid;
   char * maxRxSize;
   char * enableOnline;
   char * intPendAck;
   char * rxPend;
   char * txFree;
   char * rxFree;
   char * txPostA;
   char * txPostB;
   char * txPostC;
   char * txPass;
};

// Buffer List
struct AxiStreamDmaBuffer {
   uint       buffSize;
   char *     buffAddr;
   dma_addr_t buffHandle;
};

// Tracking Structure
struct AxiStreamDmaDevice {
   phys_addr_t     baseAddr;
   unsigned long   baseSize;
   char          * virtAddr;

	dev_t        devNum;
   const char * devName;

	struct cdev charDev;

   uint irq;
   wait_queue_head_t inq;
   wait_queue_head_t outq;

   atomic_t available;

   struct fasync_struct *async_queue;

   // Buffers
   struct AxiStreamDmaBuffer * txBuffers[TX_BUFFER_COUNT]; // Null terminated list
   struct AxiStreamDmaBuffer * rxBuffers[RX_BUFFER_COUNT]; // Null terminated list

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
   dev->reg.enableOnline = dev->virtAddr + ENABLE_ONLINE_ADDR;
   dev->reg.intPendAck   = dev->virtAddr + INT_PENDING_ACK;
   dev->reg.rxPend       = dev->virtAddr + RX_PEND_ADDR;
   dev->reg.txFree       = dev->virtAddr + TX_FREE_ADDR;
   dev->reg.rxFree       = dev->virtAddr + RX_FREE_ADDR;
   dev->reg.txPostA      = dev->virtAddr + TX_POST_ADDR_A;
   dev->reg.txPostB      = dev->virtAddr + TX_POST_ADDR_B;
   dev->reg.txPostC      = dev->virtAddr + TX_POST_ADDR_C;
   dev->reg.txPass       = dev->virtAddr + TX_POST_ADDR;
}

// Find a buffer from a received handle, don't match MSB of handle
struct AxiStreamDmaBuffer * findBuffer (struct AxiStreamDmaBuffer **list, uint count, dma_addr_t handle ) {
   uint x;

   for (x = 0; x < count; x++) {
      if ( (list[x]->buffHandle & 0x7FFFFFFF) == (handle & 0x7FFFFFFF) ) return(list[x]);
   }

   // Not found
   return(NULL);
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
	//struct AxiStreamDmaDevice * dev;

   //dev = findDevice(f->f_inode);
   //if ( dev == NULL ) {
      //printk(KERN_INFO "<%s> open: Bad inode.\n", MODULE_NAME);
      //return(0);
   //}

   //if (! atomic_dec_and_test (&dev->available)) {
      //atomic_inc(&dev->available);
      //return -EBUSY; /* already open */
   //}

   //else return(0);
   return(0);
}

// Close the device
static int AxiStreamDmaClose (struct inode *i, struct file *f) {
   //struct AxiStreamDmaDevice * dev;

   //dev = findDevice(f->f_inode);
   //if ( dev == NULL ) {
      //printk(KERN_INFO "<%s> close: Bad inode.\n", MODULE_NAME);
      //return(0);
   //}

   //AxiStreamDmaFasync(-1,f,0);

   //atomic_inc(&dev->available);
   return(0);
}

// IRQ Handler
static irqreturn_t AxiStreamDmaIrq(int irq, void *dev_id) {
   uint        stat;
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
static uint AxiStreamDmaPoll(struct file *f, poll_table *wait ) {
   uint mask  = 0;
   uint stat;

   struct AxiStreamDmaDevice * dev;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> poll: Bad inode.\n", MODULE_NAME);
      return(0);
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
   dma_addr_t       handle;
   uint             status;
   uint             size;
   uint             copySize;
   ssize_t          ret;

   struct AxiStreamDmaRx * dmaRx;

   struct AxiStreamDmaDevice * dev;
   struct AxiStreamDmaBuffer * buffer;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> read: Bad inode. Size %i\n", MODULE_NAME,len);
      return(0);
   }

   // Poll only
   if (len == AXIS_DMA_POLL) {
      handle = ioread32(dev->reg.fifoValid);
      return(handle & 0x1);
   }

   // Size must match structure
   if ( len != sizeof(struct AxiStreamDmaRx) ) return (0);
   dmaRx = (struct AxiStreamDmaRx *)buf;

   // Get handle;
   while ( ((handle = ioread32(dev->reg.rxPend)) & 0x80000000) == 0 ) {
      if ( f->f_flags & O_NONBLOCK ) return(0);
      interruptible_sleep_on(&dev->inq);
   }

   // Get buffer
   buffer = findBuffer (dev->rxBuffers,RX_BUFFER_COUNT,handle);

   if ( buffer == NULL ) {
	   printk(KERN_INFO "<%s> read: Failed to find buffer for handle 0x%08x\n", MODULE_NAME,handle);
      return(-1);
   }

   // Read size
   do { 
      size = ioread32(dev->reg.rxPend);
   } while ((size & 0x80000000) == 0);
   size &= 0xFFFFFF;

   // Get status
   do { 
      status = ioread32(dev->reg.rxPend);
   } while ((status & 0x80000000) == 0);

   // Extract Status Fields
   dmaRx->overflow   = (status >> 25) & 0x01;
   dmaRx->writeError = (status >> 24) & 0x01;
   dmaRx->lastUser   = (status >> 16) & 0xFF;
   dmaRx->firstUser  = (status >>  8) & 0xFF;
   dmaRx->dest       = (status      ) & 0xFF;

   // Overflow
   //if ( size > dmaRx->maxSize ) {
	   //printk(KERN_INFO "<%s> read: User buffer too small for handle 0x%08x\n", MODULE_NAME,handle);
      //ret = -1;
   //}
   //else {
	   //memcpy(dmaRx->buffer, buffer->buffAddr, size);
      //ret = size;
   //}
  
   // Header Copy Only
   if ( size > dmaRx->maxSize ) copySize = dmaRx->maxSize;
   else copySize = size;
	copy_to_user(dmaRx->buffer, buffer->buffAddr, copySize);
   ret = size;

   // Return to free list
   iowrite32(buffer->buffHandle,dev->reg.rxFree);

	return ret;
}

static ssize_t AxiStreamDmaWrite(struct file *f, const char __user * buf, size_t len, loff_t * off) {
   dma_addr_t       handle;
   uint             control;

   struct AxiStreamDmaTx * dmaTx;

   struct AxiStreamDmaDevice * dev;
   struct AxiStreamDmaBuffer * buffer;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> write: Bad inode. Size %i\n", MODULE_NAME,len);
      return(0);
   }

   // Ack only
   if (len == AXIS_DMA_ACK) {
      iowrite32(0x1,dev->reg.enableOnline);
      iowrite32(0x3,dev->reg.enableOnline);
      return(1);
   }

   // Size must match structure
   if ( len != sizeof(struct AxiStreamDmaTx) ) return (0);
   dmaTx = (struct AxiStreamDmaTx *)buf;

   if (dmaTx->size > TX_BUFFER_SIZE) {
      printk(KERN_INFO "<%s> write: Bad tx length. %i. Max = %i\n", MODULE_NAME,dmaTx->size,TX_BUFFER_SIZE);
      return(-1);
   }

   // Get handle;
   while ( ((handle = ioread32(dev->reg.txFree)) & 0x80000000) == 0 ) {
      if ( f->f_flags & O_NONBLOCK ) return(0);
      interruptible_sleep_on(&dev->outq);
   }

   // Get buffer
   buffer = findBuffer (dev->txBuffers,TX_BUFFER_COUNT,handle);

   if ( buffer == NULL ) {
	   printk(KERN_INFO "<%s> write: Failed to find buffer for handle 0x%08x\n", MODULE_NAME,handle);
      return(-1);
   }

   // Generate Control
   control  = (dmaTx->dest           ) & 0x000000FF;
   control += (dmaTx->firstUser <<  8) & 0x0000FF00;
   control += (dmaTx->lastUser  << 16) & 0x00FF0000;

   memcpy(buffer->buffAddr, dmaTx->buffer, dmaTx->size);

   iowrite32(buffer->buffHandle,dev->reg.txPostA);
   iowrite32(dmaTx->size,dev->reg.txPostB);
   iowrite32(control,dev->reg.txPostC);

   return(dmaTx->size);
}

static struct file_operations fops = {
	.owner   = THIS_MODULE,
   .llseek  = NULL,
	.read    = AxiStreamDmaRead,
	.write   = AxiStreamDmaWrite,
   .readdir = NULL,
   .poll    = AxiStreamDmaPoll,
   .mmap    = NULL,
	.open    = AxiStreamDmaOpen,
	.release = AxiStreamDmaClose,
   .fsync   = NULL,
   .fasync  = AxiStreamDmaFasync,
   .lock    = NULL
};

static int AxiStreamDmaProbe(struct platform_device *pdev) {
   uint x;

	struct AxiStreamDmaDevice *dev;

	dev = (struct AxiStreamDmaDevice *) kmalloc( sizeof(struct AxiStreamDmaDevice), GFP_KERNEL );

   atomic_set(&dev->available,1);

   dev->baseAddr = pdev->resource[0].start;
   dev->baseSize = (pdev->resource[0].end - pdev->resource[0].start) + 1;

   // Get and map register space
   if (check_mem_region(dev->baseAddr,dev->baseSize) ) return(-1);
   request_mem_region(dev->baseAddr,dev->baseSize,MODULE_NAME);
   dev->virtAddr = (char *) ioremap_nocache(dev->baseAddr,dev->baseSize);
   setRegisterPointers(dev);

	dev->devName = pdev->name + 9;
	
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
                                      
   // Set MAX RX                      
   iowrite32(RX_BUFFER_SIZE,dev->reg. maxRxSize);
                                      
   // Clear FIFOs                     
   iowrite32(0x1,dev->reg.fifoClear); 
   iowrite32(0x0,dev->reg.fifoClear); 

   // Enable rx and tx
   iowrite32(0x1,dev->reg.rxEnable);
   iowrite32(0x1,dev->reg.txEnable);

	printk(KERN_INFO "<%s> init: Creating TX Buffers\n", MODULE_NAME);

   // Allocate TX buffers
   for (x=0; x < TX_BUFFER_COUNT; x++) {
      dev->txBuffers[x] = (struct AxiStreamDmaBuffer *) 
         kmalloc( sizeof(struct AxiStreamDmaBuffer), GFP_KERNEL );

      dev->txBuffers[x]->buffSize = TX_BUFFER_SIZE;
      dev->txBuffers[x]->buffAddr = 
         dma_zalloc_coherent(NULL, TX_BUFFER_SIZE, &(dev->txBuffers[x]->buffHandle), GFP_KERNEL);

	   printk(KERN_INFO "<%s> init: Creating buffer %i. Handle 0x%08x, Address 0x%08x\n", 
         MODULE_NAME,x,dev->txBuffers[x]->buffHandle,(uint)dev->txBuffers[x]->buffAddr);

      if ( dev->txBuffers[x]->buffAddr != NULL )  iowrite32(dev->txBuffers[x]->buffHandle,dev->reg.txPass);
   }
  
	printk(KERN_INFO "<%s> init: Creating RX Buffers\n", MODULE_NAME);

   // Allocate RX buffers
   for (x=0; x < RX_BUFFER_COUNT; x++) {
      dev->rxBuffers[x] = (struct AxiStreamDmaBuffer *) 
         kmalloc( sizeof(struct AxiStreamDmaBuffer), GFP_KERNEL );

      dev->rxBuffers[x]->buffSize = RX_BUFFER_SIZE;
      dev->rxBuffers[x]->buffAddr = 
         dma_zalloc_coherent(NULL, RX_BUFFER_SIZE, &(dev->rxBuffers[x]->buffHandle), GFP_KERNEL);

	   printk(KERN_INFO "<%s> init: Creating buffer %i. Handle 0x%08x, Address 0x%08x\n", 
         MODULE_NAME,x,dev->rxBuffers[x]->buffHandle,(uint)dev->rxBuffers[x]->buffAddr);

      if ( dev->rxBuffers[x]->buffAddr != NULL ) iowrite32(dev->rxBuffers[x]->buffHandle,dev->reg.rxFree);
   }

   // Get IRQ from pci_dev structure. 
   dev->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);;
   printk(KERN_INFO "<%s>: Init: IRQ %d\n", MODULE_NAME, dev->irq);

   // Request IRQ from OS.
   if (request_irq( dev->irq, AxiStreamDmaIrq, 0, MODULE_NAME, (void*)dev) < 0 ) {
      printk(KERN_WARNING"<%s>: Init: Unable to allocate IRQ.",MODULE_NAME);
      return (-1);
   }

   // Init queues
   init_waitqueue_head(&(dev->inq));
   init_waitqueue_head(&(dev->outq));

   // Enable interrupt
   iowrite32(0x1,dev->reg.intPendAck);
   iowrite32(0x1,dev->reg.intEnable);

   // Online bits
   iowrite32(0x3,dev->reg.enableOnline);

	list_add( &dev->devList, &fullDevList );
	return 0;
}

static int AxiStreamDmaRemove(struct platform_device *pdev) {
   struct list_head *pos, *q;
   struct AxiStreamDmaDevice *dev;
   uint x;

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
         iowrite32(0x0,dev->reg.enableOnline);

         // De-Allocate TX buffers
         for (x=0; x < TX_BUFFER_COUNT; x++) {
            if ( dev->txBuffers[x]->buffAddr != NULL )
               dma_free_coherent(NULL, TX_BUFFER_SIZE, dev->txBuffers[x]->buffAddr,dev->txBuffers[x]->buffHandle);
            kfree(dev->txBuffers[x]);
         }
        
         // De-Allocate RX buffers
         for (x=0; x < RX_BUFFER_COUNT; x++) {
            if ( dev->rxBuffers[x]->buffAddr != NULL )
               dma_free_coherent(NULL, RX_BUFFER_SIZE, dev->rxBuffers[x]->buffAddr,dev->rxBuffers[x]->buffHandle);
            kfree(dev->rxBuffers[x]);
         }
   
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

  	if (list_empty(&fullDevList)) {
  		class_destroy(cl);
  	}
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
MODULE_DESCRIPTION("AXI Stream DMA driver");
MODULE_LICENSE("GPL v2");

