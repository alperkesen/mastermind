#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/switch_to.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */


//  check if linux/uaccess.h is required for copy_*_user
//instead of asm/uaccess
//required after linux kernel 4.1+ ?
#ifndef __ASM_ASM_UACCESS_H
    #include <linux/uaccess.h>
#endif


#include "mastermind_ioctl.h"

#define MASTERMIND_MAJOR 0
#define MASTERMIND_NR_DEVS 4
#define MASTERMIND_QUANTUM 4000
#define MASTERMIND_QSET 1000

int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = 0;
int mastermind_nr_devs = MASTERMIND_NR_DEVS;
int mastermind_quantum = MASTERMIND_QUANTUM;
int mastermind_qset = MASTERMIND_QSET;

module_param(mastermind_major, int, S_IRUGO);
module_param(mastermind_minor, int, S_IRUGO);
module_param(mastermind_nr_devs, int, S_IRUGO);
module_param(mastermind_quantum, int, S_IRUGO);
module_param(mastermind_qset, int, S_IRUGO);

MODULE_AUTHOR("Group 28");
MODULE_LICENSE("MIT License");

struct mastermind_dev {
    char **data;
    int quantum;
    int qset;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct mastermind_dev *mastermind_devices;


int mastermind_trim(struct mastermind_dev *dev)
{
    int i;

    if (dev->data) {
        for (i = 0; i < dev->qset; i++) {
            if (dev->data[i])
                kfree(dev->data[i]);
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->quantum = mastermind_quantum;
    dev->qset = mastermind_qset;
    dev->size = 0;
    return 0;
}


int mastermind_open(struct inode *inode, struct file *filp)
{
    struct mastermind_dev *dev;

    dev = container_of(inode->i_cdev, struct mastermind_dev, cdev);
    filp->private_data = dev;

    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        mastermind_trim(dev);
        up(&dev->sem);
    }
    return 0;
}


int mastermind_release(struct inode *inode, struct file *filp)
{
    return 0;
}


ssize_t mastermind_read(struct file *filp, char __user *buf, size_t count,
			loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    int quantum = dev->quantum;
    int s_pos, q_pos;
    ssize_t retval = 0;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;
    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (dev->data == NULL || ! dev->data[s_pos])
        goto out;

    /* read only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_to_user(buf, dev->data[s_pos] + q_pos, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

  out:
    up(&dev->sem);
    return retval;
}


ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    int quantum = dev->quantum, qset = dev->qset;
    int s_pos, q_pos;
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= quantum * qset) {
        retval = 0;
        goto out;
    }

    s_pos = (long) *f_pos / quantum;
    q_pos = (long) *f_pos % quantum;

    if (!dev->data) {
        dev->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
        if (!dev->data)
            goto out;
        memset(dev->data, 0, qset * sizeof(char *));
    }
    if (!dev->data[s_pos]) {
        dev->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dev->data[s_pos])
            goto out;
    }
    /* write only up to the end of this quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_from_user(dev->data[s_pos] + q_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

  out:
    up(&dev->sem);
    return retval;
}

long mastermind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0, tmp;
	int retval = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != MASTERMIND_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > MASTERMIND_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	  case MASTERMIND_IOCRESET:
		mastermind_quantum = MASTERMIND_QUANTUM;
		mastermind_qset = MASTERMIND_QSET;
		break;

	  case MASTERMIND_IOCSQUANTUM: /* Set: arg points to the value */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mastermind_quantum, (int __user *)arg);
		break;

	  case MASTERMIND_IOCTQUANTUM: /* Tell: arg is the value */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mastermind_quantum = arg;
		break;

	  case MASTERMIND_IOCGQUANTUM: /* Get: arg is pointer to result */
		retval = __put_user(mastermind_quantum, (int __user *)arg);
		break;

	  case MASTERMIND_IOCQQUANTUM: /* Query: return it (it's positive) */
		return mastermind_quantum;

	  case MASTERMIND_IOCXQUANTUM: /* eXchange: use arg as pointer */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_quantum;
		retval = __get_user(mastermind_quantum, (int __user *)arg);
		if (retval == 0)
			retval = __put_user(tmp, (int __user *)arg);
		break;

	  case MASTERMIND_IOCHQUANTUM: /* sHift: like Tell + Query */
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_quantum;
		mastermind_quantum = arg;
		return tmp;

	  case MASTERMIND_IOCSQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(mastermind_qset, (int __user *)arg);
		break;

	  case MASTERMIND_IOCTQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		mastermind_qset = arg;
		break;

	  case MASTERMIND_IOCGQSET:
		retval = __put_user(mastermind_qset, (int __user *)arg);
		break;

	  case MASTERMIND_IOCQQSET:
		return mastermind_qset;

	  case MASTERMIND_IOCXQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_qset;
		retval = __get_user(mastermind_qset, (int __user *)arg);
		if (retval == 0)
			retval = put_user(tmp, (int __user *)arg);
		break;

	  case MASTERMIND_IOCHQSET:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
		tmp = mastermind_qset;
		mastermind_qset = arg;
		return tmp;

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t mastermind_llseek(struct file *filp, loff_t off, int whence)
{
    struct mastermind_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations mastermind_fops = {
    .owner =    THIS_MODULE,
    .llseek =   mastermind_llseek,
    .read =     mastermind_read,
    .write =    mastermind_write,
    .unlocked_ioctl =  mastermind_ioctl,
    .open =     mastermind_open,
    .release =  mastermind_release,
};


void mastermind_cleanup_module(void)
{
    int i;
    dev_t devno = MKDEV(mastermind_major, mastermind_minor);

    if (mastermind_devices) {
        for (i = 0; i < mastermind_nr_devs; i++) {
            mastermind_trim(mastermind_devices + i);
            cdev_del(&mastermind_devices[i].cdev);
        }
    kfree(mastermind_devices);
    }

    unregister_chrdev_region(devno, mastermind_nr_devs);
}


int mastermind_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct mastermind_dev *dev;

    if (mastermind_major) {
        devno = MKDEV(mastermind_major, mastermind_minor);
        result = register_chrdev_region(devno, mastermind_nr_devs, "mastermind");
    } else {
        result = alloc_chrdev_region(&devno, mastermind_minor, mastermind_nr_devs,
                                     "mastermind");
        mastermind_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "mastermind: can't get major %d\n", mastermind_major);
        return result;
    }

    mastermind_devices = kmalloc(mastermind_nr_devs * sizeof(struct mastermind_dev),
				 GFP_KERNEL);
    if (!mastermind_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(mastermind_devices, 0, mastermind_nr_devs * sizeof(struct mastermind_dev));

    /* Initialize each device. */
    for (i = 0; i < mastermind_nr_devs; i++) {
        dev = &mastermind_devices[i];
        dev->quantum = mastermind_quantum;
        dev->qset = mastermind_qset;
        sema_init(&dev->sem,1);
        devno = MKDEV(mastermind_major, mastermind_minor + i);
        cdev_init(&dev->cdev, &mastermind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &mastermind_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding mastermind%d", err, i);
    }

    return 0; /* succeed */

  fail:
    mastermind_cleanup_module();
    return result;
}

module_init(mastermind_init_module);
module_exit(mastermind_cleanup_module);
