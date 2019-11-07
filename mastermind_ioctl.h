#ifndef __MASTERMIND_H
#define __MASTERMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

//Command Number is the number that is assigned to the ioctl. It is used to differentiate the commands from one another.

#define MASTERMIND_IOC_MAGIC  'k' //The Magic Number is a unique number or character that will differentiate our set of ioctl calls from the other ioctl calls.
#define MASTERMIND_IOCRESET    _IO(MASTERMIND_IOC_MAGIC, 0) //Used for a simple ioctl that sends nothing but the type and number, and receives back nothing but an (integer) retval.
#define MASTERMIND_IOCSQUANTUM _IOW(MASTERMIND_IOC_MAGIC,  1, int) //Used for an ioctl that writes data to the device driver.
#define MASTERMIND_IOCSQSET    _IOW(MASTERMIND_IOC_MAGIC,  2, int)
#define MASTERMIND_IOCTQUANTUM _IO(MASTERMIND_IOC_MAGIC,   3)
#define MASTERMIND_IOCTQSET    _IO(MASTERMIND_IOC_MAGIC,   4)
#define MASTERMIND_IOCGQUANTUM _IOR(MASTERMIND_IOC_MAGIC,  5, int) //Used for an ioctl that reads data from the device driver. The driver will be allowed to return sizeof(data_type) bytes to the user.
#define MASTERMIND_IOCGQSET    _IOR(MASTERMIND_IOC_MAGIC,  6, int)
#define MASTERMIND_IOCQQUANTUM _IO(MASTERMIND_IOC_MAGIC,   7)
#define MASTERMIND_IOCQQSET    _IO(MASTERMIND_IOC_MAGIC,   8)
#define MASTERMIND_IOCXQUANTUM _IOWR(MASTERMIND_IOC_MAGIC, 9, int)
#define MASTERMIND_IOCXQSET    _IOWR(MASTERMIND_IOC_MAGIC,10, int)
#define MASTERMIND_IOCHQUANTUM _IO(MASTERMIND_IOC_MAGIC,  11)
#define MASTERMIND_IOCHQSET    _IO(MASTERMIND_IOC_MAGIC,  12)
#define MASTERMIND_IOC_MAXNR 12

#endif
