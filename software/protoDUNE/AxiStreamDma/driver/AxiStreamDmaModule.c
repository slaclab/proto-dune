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
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/mm.h>
#include <asm/io.h>

#define FIFO_OFFSET 0x10000
#define REG_OFFSET  0x00000

#define RX_ENABLE_ADDR     (REG_OFFSET  + 0x000)
#define TX_ENABLE_ADDR     (REG_OFFSET  + 0x004)
#define FIFO_CLEAR_ADDR    (REG_OFFSET  + 0x008)
#define INT_ENABLE_ADDR    (REG_OFFSET  + 0x00C)
#define FIFO_VALID_ADDR    (REG_OFFSET  + 0x010)
#define MAX_RX_SIZE_ADDR   (REG_OFFSET  + 0x014)
#define ENABLE_ONLINE_ADDR (REG_OFFSET  + 0x018)
#define TEST_ID_ADDR       (REG_OFFSET  + 0x01C)
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
   char * testId;
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
   dev->reg.testId       = dev->virtAddr + TEST_ID_ADDR;
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

// Open the device
static int AxiStreamDmaOpen (struct inode *i, struct file *f) {
   return(0);
}

// Close the device
static int AxiStreamDmaClose (struct inode *i, struct file *f) {
   return(0);
}

static ssize_t AxiStreamDmaRead(struct file *f, char __user * buf, size_t len, loff_t * off) {
   dma_addr_t handle;
   uint       status;
   uint       size;
   ssize_t    ret;

   struct AxiStreamDmaDevice * dev;
   struct AxiStreamDmaBuffer * buffer;

   dev = findDevice(f->f_inode);
   if ( dev == NULL ) {
      printk(KERN_INFO "<%s> read: Bad inode. Size %i\n", MODULE_NAME,len);
      return(0);
   }

   // Poll only
   if (len == 1 ) {
      handle = ioread32(dev->reg.fifoValid);
      return(handle);
   }

   // Small size
   if ( len < 8 ) return (0);

   // Get handle;
   handle = ioread32(dev->reg.rxPend);
   if ((handle & 0x80000000) == 0 ) return(0);

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
   status &= 0x3FFFFFF;

   // Overflow
   if ( (size+4) > len ) ret = -1;

   else {
      *((uint *)buf) = status;
      buf += 4;

	   memcpy(buf, buffer->buffAddr, size);
      ret = size+4;
   }

   // Return to free list
   iowrite32(buffer->buffHandle,dev->reg.rxFree);

	return ret;
}

static ssize_t AxiStreamDmaWrite(struct file *f, const char __user * buf, size_t len, loff_t * off) {
   dma_addr_t handle;
   uint       control;
   uint       size;

   struct AxiStreamDmaDevice * dev;
   struct AxiStreamDmaBuffer * buffer;

   if (len > RX_BUFFER_SIZE) {
      printk(KERN_INFO "<%s> write: Bad tx length. %i. Max = %i\n", MODULE_NAME,len,RX_BUFFER_SIZE);
      return(-1);
   }

   dev = findDevice(f->f_inode);

   // Size = 1 is an ack, pulse enabled bit
   if ( len == 1 ) {
      iowrite32(0x1,dev->reg.enableOnline);
      iowrite32(0x3,dev->reg.enableOnline);
      return(1);
   }

   // Get free handle;
   handle = ioread32(dev->reg.txFree);
   if ((handle & 0x80000000) == 0 ) {
	   //printk(KERN_INFO "<%s> write: No free descriptor. Addr 0x%08x Data 0x%08x\n", MODULE_NAME,(uint)dev->reg.txFree,handle);
      return(0);
   }

   // Get buffer
   buffer = findBuffer (dev->txBuffers,TX_BUFFER_COUNT,handle);

   if ( buffer == NULL ) {
	   printk(KERN_INFO "<%s> write: Failed to find buffer for handle 0x%08x\n", MODULE_NAME,handle);
      return(-1);
   }

   // Extract control
   control = *((uint *)buf);
   buf += 4;

   size = len - 4;

   memcpy(buffer->buffAddr, buf, size);

   iowrite32(buffer->buffHandle,dev->reg.txPostA);
   iowrite32(size,dev->reg.txPostB);
   iowrite32(control,dev->reg.txPostC);

   return(len);
}

static struct file_operations fops = {
	.owner   = THIS_MODULE,
   .llseek  = NULL,
	.read    = AxiStreamDmaRead,
	.write   = AxiStreamDmaWrite,
   .readdir = NULL,
   .poll    = NULL,
   .mmap    = NULL,
	.open    = AxiStreamDmaOpen,
	.release = AxiStreamDmaClose,
   .fsync   = NULL,
   .fasync  = NULL,
   .lock    = NULL
};

static int AxiStreamDmaProbe(struct platform_device *pdev) {
   uint x;

	struct AxiStreamDmaDevice *dev;

	dev = (struct AxiStreamDmaDevice *) kmalloc( sizeof(struct AxiStreamDmaDevice), GFP_KERNEL );

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

      //if ( !my_strcmp(dev->devName,pdev->name + 9) ) {
      if ( !strcmp(dev->devName,pdev->name + 9) ) {
         list_del (pos);
         cdev_del(&(dev->charDev));
    		device_destroy(cl, dev->devNum);
    		unregister_chrdev_region(dev->devNum, 1);

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

