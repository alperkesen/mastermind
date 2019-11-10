#ifndef __MASTERMIND_H
#define __MASTERMIND_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

//Command Number is the number that is assigned to the ioctl. It is used to differentiate the commands from one another.

#define MASTERMIND_IOC_MAGIC  'k' //The Magic Number is a unique number or character that will differentiate our set of ioctl calls from the other ioctl calls.
#define MMIND_REMAINING _IO(MASTERMIND_IOC_MAGIC, 0)
#define MMIND_NEWGAME _IO(MASTERMIND_IOC_MAGIC,  1)
#define MMIND_ENDGAME _IO(MASTERMIND_IOC_MAGIC,  2)
#define MASTERMIND_IOC_MAXNR 3

#endif
