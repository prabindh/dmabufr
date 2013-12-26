/********************************************************************************
*																				*
*																				*
* Dummy kernel module for dmabuf offscreen renderer (dmabufr)
*																				*
*																				*
/********************************************************************************/

#include <linux/dma-buf.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

#include "dmabufr.h"

#define DMABUFR_NUM_DEVICES 1
#define DMABUFR_NAME "dmabufr"

static int dmabufr_open(struct inode *i, struct file *f);
static int dmabufr_release(struct inode *i, struct file *f);
static int dmabufr_mmap(struct file *filp, struct vm_area_struct *vma);
static long dmabufr_ioctl_unlocked(struct file *file, unsigned int cmd, unsigned long arg);
static long dmabufr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static const struct file_operations dmabufr_fops = {
	.owner = THIS_MODULE,
	.open = dmabufr_open,
	.unlocked_ioctl = dmabufr_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dmabufr_compat_ioctl,
#endif
	.release = dmabufr_release
};

static int major;
static int dma_size = 0;
static struct class *dmabufr_class;
static struct device *dmabufr_device;
static DEFINE_MUTEX(dmabufr_io_mutex);
/*
 *	Initialise dmabufr
 */
static int __init dmabufr_init(void)
{
	printk(KERN_INFO "dmabufr offscreen device for SGX\n");
	major = register_chrdev(0, DMABUFR_NAME, &dmabufr_fops);
	if (major <= 0) 
	{
		printk(KERN_WARNING "dmabufr: unable to get major number\n");
		return -1;
	}

	dmabufr_class = class_create(THIS_MODULE, DMABUFR_NAME);
	if (IS_ERR(dmabufr_class)) 
	{
		printk(KERN_WARNING "dmabufr_dev: class_register failed\n");
		return -EIO;	
	}
	dmabufr_device = device_create(dmabufr_class, NULL, MKDEV(major, 0), NULL,
						   DMABUFR_NAME);

	if (IS_ERR(dmabufr_device)) 
	{
		printk(KERN_ERR DMABUFR_NAME ": unable to create device\n");
		class_destroy(dmabufr_class);
		unregister_chrdev(major, DMABUFR_NAME);
		return -1;
	}
	return 0;
}

static void __exit dmabufr_exit(void)
{
	dev_t dev = MKDEV(major, 0);

	class_unregister(dmabufr_class);
	unregister_chrdev_region(dev, DMABUFR_NUM_DEVICES);
}


static int dmabufr_open(struct inode *i, struct file *f)
{
	/* Nothing to do now */
}

static int dmabufr_release(struct inode *i, struct file *f)
{
	/* Nothing to do */
}

/********************************************************************************
*																				*
*																				*
* IOCTL implementation for the device
*																				*
*																				*
/********************************************************************************/
static void dmabufr_dummy_dma_buf_op_handler()
{
}

static struct dma_buf *curr_dma_buf;
static struct dma_buf_attachment *curr_dma_buf_attachment;
static struct sg_table *curr_sg_table;
static struct dma_buf_ops dma_ops = 
{
                 .map_dma_buf = dmabufr_dummy_dma_buf_op_handler,
                 .unmap_dma_buf = dmabufr_dummy_dma_buf_op_handler,
                 .release = dmabufr_dummy_dma_buf_op_handler,
                 .begin_cpu_access = dmabufr_dummy_dma_buf_op_handler,
                 .end_cpu_access = dmabufr_dummy_dma_buf_op_handler,
                 .kmap_atomic = dmabufr_dummy_dma_buf_op_handler,
                 .kunmap_atomic = dmabufr_dummy_dma_buf_op_handler,
                 .kmap = dmabufr_dummy_dma_buf_op_handler,
                 .kunmap = dmabufr_dummy_dma_buf_op_handler,
                 .mmap = dmabufr_dummy_dma_buf_op_handler
};

typedef struct _FD_INFO
{
	int fd;
	void* pBuf;
}FD_INFO;

static FD_INFO fdInfo = {0};

int ioctl_request_fd()
{
/* The buffer exporter announces its wish to export a buffer. In this, it
   connects its own private buffer data, provides implementation for operations
   that can be performed on the exported dma_buf, and flags for the file
   associated with this buffer.

   Interface:
      struct dma_buf *dma_buf_export(void *priv, struct dma_buf_ops *ops,
				     size_t size, int flags)
*/
/*
   Userspace entity requests for a file-descriptor (fd) which is a handle to the
   anonymous file associated with the buffer. It can then share the fd with other
   drivers and/or processes.

   Interface:
      int dma_buf_fd(struct dma_buf *dmabuf)
*/
	int fd;

	curr_dma_buf = dma_buf_export(NULL, &dma_ops, dma_size, 0);	
	fd = dma_buf_fd(curr_dma_buf);
	return fd;
}

int ioctl_connect(int fd)
{
/*
 Each buffer-user 'connects' itself to the buffer

   Each buffer-user now gets a reference to the buffer, using the fd passed to
   it.

   Interface:
      struct dma_buf *dma_buf_get(int fd)

After this, the buffer-user needs to attach its device with the buffer, which
   helps the exporter to know of device buffer constraints.

   Interface:
      struct dma_buf_attachment *dma_buf_attach(struct dma_buf *dmabuf,
                                                struct device *dev)
*/
	struct dma_buf* dma_buf_stored = dma_buf_get(fd); //todo - store this in some context
	curr_dma_buf_attachment = dma_buf_attach(dma_buf_stored, dmabufr_device);
	return 0;
}

int ioctl_use_buffer(int fd)
{
/*
buffer-user requests access to the buffer

   Whenever a buffer-user wants to use the buffer for any DMA, it asks for
   access to the buffer using dma_buf_map_attachment API. At least one attach to
   the buffer must have happened before map_dma_buf can be called.

   Interface:
      struct sg_table * dma_buf_map_attachment(struct dma_buf_attachment *,
                                         enum dma_data_direction);
*/
	curr_sg_table = dma_buf_map_attachment(curr_dma_buf_attachment, DMA_BIDIRECTIONAL);
	/* todo - use these sg buffers to do some operation on the device */
	return 0;
}

int ioctl_end_of_operation(int fd)
{
/*
When finished, the buffer-user notifies end-of-DMA to exporter

   Once the DMA for the current buffer-user is over, it signals 'end-of-DMA' to
   the exporter using the dma_buf_unmap_attachment API.

   Interface:
      void dma_buf_unmap_attachment(struct dma_buf_attachment *,
                                    struct sg_table *);
*/
	/* todo - use fd from userland to get back the contexts */
	dma_buf_unmap_attachment(curr_dma_buf_attachment, curr_sg_table);
	return 0;
}


int ioctl_detach(int fd)
{
/*
when buffer-user is done using this buffer, it 'disconnects' itself from the
   buffer.

   After the buffer-user has no more interest in using this buffer, it should
   disconnect itself from the buffer:

   - it first detaches itself from the buffer.

   Interface:
      void dma_buf_detach(struct dma_buf *dmabuf,
                          struct dma_buf_attachment *dmabuf_attach);

Then, the buffer-user returns the buffer reference to exporter.

   Interface:
     void dma_buf_put(struct dma_buf *dmabuf);
*/

	/* todo - use fd from userland to get back the contexts */
	dma_buf_detach(curr_dma_buf, curr_dma_buf_attachment);
	dma_buf_put(curr_dma_buf);
	return 0;
}

/********************************************************************************
*																				*
*																				*
* IOCTL implementation for the device
*																				*
*																				*
/********************************************************************************/
static long dmabufr_ioctl_unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
        int res;

        mutex_lock(&dmabufr_io_mutex);
        res = dmabufr_ioctl(file, cmd, arg);
        mutex_unlock(&dmabufr_io_mutex);

        return res;
}

static long  dmabufr_ioctl(struct file *file,
                    unsigned int cmd, unsigned long arg)
{
	DMABUFR_IO* dmabufr_io_pointer = (DMABUFR_IO*)arg;
	long ret = 0;
	switch(cmd)
	{
		case DMABUFR_IOCTL_REQUEST_FD:
		{
			int fd;
			DMABUFR_ALLOCATION_INFO info;			
            if (!access_ok(VERIFY_WRITE, arg, sizeof(DMABUFR_IO)))
				return -EFAULT;
			if(copy_from_user(&info, dmabufr_io_pointer->input, sizeof(DMABUFR_ALLOCATION_INFO)))
				return -EFAULT;
			if(info.format != FMT_ARGB) return -EFAULT;
			if(fdInfo.fd != 0) return -EFAULT; //only 1
			dma_size = 4*info.width*info.height;
			fdInfo.pBuf = kmalloc(4*info.width*info.height, GFP_KERNEL);
			fdInfo.fd = ioctl_request_fd();
			dmabufr_io_pointer->output = (void*)fd;
			break;
		}	
		case DMABUFR_IOCTL_CONNECT_FD:
		{
			int fd;
            if (!access_ok(VERIFY_READ, arg, sizeof(DMABUFR_IO)))			
				return -EFAULT;
			fd = dmabufr_io_pointer->input;
			ret = ioctl_connect(fd);
			break;
		}
		case DMABUFR_IOCTL_USE_BUFFER_FD:
		{
			int fd;
            if (!access_ok(VERIFY_READ, arg, sizeof(DMABUFR_IO)))			
				return -EFAULT;
			fd = (int)dmabufr_io_pointer->input;		
			ret = ioctl_use_buffer(fd);
			break;
		}
		case DMABUFR_IOCTL_END_OF_OPERATION_FD:
		{
					int fd;
            if (!access_ok(VERIFY_READ, arg, sizeof(DMABUFR_IO)))			
				return -EFAULT;
			fd = (int)dmabufr_io_pointer->input;
			ret = ioctl_end_of_operation(fd);
			break;
		}
		case DMABUFR_IOCTL_DETACH_FD:
		{
			int fd;
            if (!access_ok(VERIFY_READ, arg, sizeof(DMABUFR_IO)))			
				return -EFAULT;
			fd = (int)dmabufr_io_pointer->input;		
			ret = ioctl_detach(fd);
			break;
		}
		default:
		{
			printk("ERR: Undefined IOCTL cmd %x\n", cmd);
			ret = -EFAULT;
			break;
		}
	}
	return ret;
}

/********************************************************************************
*																				*
*																				*
* End of file
*																				*
*																				*
/********************************************************************************/
late_initcall(dmabufr_init);
module_exit(dmabufr_exit)

MODULE_AUTHOR("Prabindh Sundareson <prabu@ti.com>");
MODULE_DESCRIPTION("Wrapper driver for offscreen dma-buf memory device for SGX");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CHARDEV_MAJOR(DMABUFR_MAJOR);
