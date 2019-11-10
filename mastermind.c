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

#define MASTERMIND_MAJOR      0
#define MASTERMIND_NR_DEVS    4
#define MMIND_NUMBER          "4283"
#define MMIND_MAX_GUESSES     10
#define MMIND_GUESS           16      // quantum
#define MMIND_NUM_GUESS       256     // qset
#define MMIND_DIGITS          4

int mastermind_major = MASTERMIND_MAJOR;
int mastermind_minor = 0;
int mastermind_nr_devs = MASTERMIND_NR_DEVS;
char *mmind_number = MMIND_NUMBER;
int mmind_max_guesses = MMIND_MAX_GUESSES;
int mmind_guess = MMIND_GUESS;
int mmind_num_guess = MMIND_NUM_GUESS;

module_param(mastermind_major, int, S_IRUGO);
module_param(mmind_number, charp, S_IRUGO | S_IWUSR);
module_param(mmind_max_guesses, int, S_IRUGO);

MODULE_AUTHOR("Group 28");
MODULE_LICENSE("MIT License");

struct mastermind_dev {
    char **data;
    int guess; // quantum
    int num_guess; // qset
    int current_guess;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct mastermind_dev *mastermind_devices;


void write_mmind_number(char *buffer, char *mmind_number, char *number, int num_guess)
{
  int i;
  int m = 0;
  int n = 0;
  char temp_number[MMIND_DIGITS+1];
  memcpy(temp_number, number, MMIND_DIGITS + 1);

  for (i = 0; i < MMIND_DIGITS; i++) {
    if (number[i] == mmind_number[i]) {
        temp_number[i] = '+';
        m++;
    }
  }
  for (i = 0; i < MMIND_DIGITS; i++) {
    if (temp_number[i] != '+') {
        char* found_char = strchr(temp_number, mmind_number[i]);
        if(found_char) {
            found_char[0] = '-';
            n++;
        }
    }
  }

  number[MMIND_DIGITS] = '\0';
  snprintf(buffer, MMIND_GUESS, "%s %d+%d- %04d\n", number, m, n, num_guess);
}


int mastermind_trim(struct mastermind_dev *dev)
{
    int i;

    if (dev->data) {
        for (i = 0; i < dev->num_guess; i++) {
            if (dev->data[i])
                kfree(dev->data[i]);
        }
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->guess = mmind_guess;
    dev->num_guess = mmind_num_guess;
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
        // mastermind_trim(dev);
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
    int guess = dev->guess;
    int s_pos;
    int i;
    char *report;
    ssize_t retval = 0;

    count = guess * dev->current_guess;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->size)
        goto out;

    if (dev->data == NULL || ! dev->data[0])
        goto out;

    report = kmalloc(count, GFP_KERNEL);

    for (i = 0; i < dev->current_guess; i++) {
        memcpy(report + i * guess, dev->data[i], guess);
    }

    if (copy_to_user(buf, report, count)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;
    retval = count;
    kfree(report);
  out:
    up(&dev->sem);
    return retval;
}


ssize_t mastermind_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct mastermind_dev *dev = filp->private_data;
    int guess = dev->guess, num_guess = dev->num_guess;
    int s_pos;
    char *number;
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= guess * num_guess) {
        retval = 0;
        goto out;
    }

    s_pos = (long) *f_pos / guess;
    s_pos = (long) dev->current_guess;

    if (!dev->data) {
        dev->data = kmalloc(num_guess * sizeof(char *), GFP_KERNEL);
        if (!dev->data)
            goto out;
        memset(dev->data, 0, num_guess * sizeof(char *));
    }
    if (!dev->data[s_pos]) {
        dev->data[s_pos] = kmalloc(guess, GFP_KERNEL);
        if (!dev->data[s_pos])
            goto out;
    }

    number = (char *)kmalloc((MMIND_DIGITS + 1) * sizeof(char), GFP_KERNEL);

    if (copy_from_user(number, buf, MMIND_DIGITS + 1)) {
        retval = -EFAULT;
        goto out;
    }

    dev->current_guess++;

    if (dev->current_guess > mmind_max_guesses) {
        printk(KERN_WARNING "mastermind: can't add guess");
	dev->current_guess--;
        retval = -EDQUOT;
        goto out;
    }

    count = guess;

    write_mmind_number(dev->data[s_pos], mmind_number, number,
		       dev->current_guess);
    *f_pos += count;
    retval = count;

    if (dev->size < *f_pos)
        dev->size = *f_pos;

  out:
    up(&dev->sem);
    return retval;
}

long mastermind_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        struct mastermind_dev *dev = filp->private_data;
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
          case MMIND_REMAINING:
	    if (!capable(CAP_SYS_ADMIN))
	      return -EPERM;
	    retval = mmind_max_guesses - dev->current_guess;
	    break;
          case MMIND_NEWGAME:
	    if (!capable(CAP_SYS_ADMIN))
	      return -EPERM;
	    mastermind_trim(dev);
	    mmind_number = arg;
	    break;
	  case MMIND_ENDGAME:
	    if (!capable(CAP_SYS_ADMIN))
	      return -EPERM;
	    mastermind_trim(dev);
	    break;
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
        dev->guess = mmind_guess;
        dev->num_guess = mmind_num_guess;
	dev->current_guess = 0;
        sema_init(&dev->sem,1);
        devno = MKDEV(mastermind_major, mastermind_minor + i);
        cdev_init(&dev->cdev, &mastermind_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &mastermind_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding mastermind%d", err, i);
    }

    printk(KERN_INFO "Loading hello module...\n");
    return 0; /* succeed */

  fail:
    mastermind_cleanup_module();
    return result;
}

module_init(mastermind_init_module);
module_exit(mastermind_cleanup_module);
