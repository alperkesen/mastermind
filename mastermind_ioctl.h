#ifndef __SCULL_H
#define __SCULL_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

//Command Number is the number that is assigned to the ioctl. It is used to differentiate the commands from one another.

#define SCULL_IOC_MAGIC  'k' //The Magic Number is a unique number or character that will differentiate our set of ioctl calls from the other ioctl calls.
#define SCULL_IOCRESET    _IO(SCULL_IOC_MAGIC, 0) //Used for a simple ioctl that sends nothing but the type and number, and receives back nothing but an (integer) retval.
#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC,  1, int) //Used for an ioctl that writes data to the device driver.
#define SCULL_IOCSQSET    _IOW(SCULL_IOC_MAGIC,  2, int)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC,   3)
#define SCULL_IOCTQSET    _IO(SCULL_IOC_MAGIC,   4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC,  5, int) //Used for an ioctl that reads data from the device driver. The driver will be allowed to return sizeof(data_type) bytes to the user.
#define SCULL_IOCGQSET    _IOR(SCULL_IOC_MAGIC,  6, int)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC,   7)
#define SCULL_IOCQQSET    _IO(SCULL_IOC_MAGIC,   8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET    _IOWR(SCULL_IOC_MAGIC,10, int)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC,  11)
#define SCULL_IOCHQSET    _IO(SCULL_IOC_MAGIC,  12)
#define SCULL_IOC_MAXNR 12

#endif
