# mastermind
Mastermind Character Device Module for System Programming Course

## Getting Started

### Compile the module
In order to compile the Mastermind module, first run the make file and wait until it finishes compiling.

```bash
$ make
```

### Become root
Before continuing, it's better to become root.

```bash
$ sudo su
```
### Insert the module
After compiling the module, insert the module into kernel.

```bash
$ insmod mastermind.ko mmind_number="4283"
```

### Create a device node

Assuming major number is 239, create a device node:

```bash
$ mknod /dev/mastermind c 239 0
```

### Testing the game

Now, we can test the game:

```bash
$ echo "1234" > /dev/mastermind
$ cat /dev/mastermind
1234 1+ 2- 0001
$ echo "4513" > /dev/mastermind
$ cat /dev/mastermind
1234 1+ 2- 0001
4531 1+ 1- 0002
```

### Testing ioctl commands

In order to test ioctl commands, first compile test code.

```bash
$ gcc mmind_ioctl.c -o test
```

Then run the output file with arguments depending on the ioctl command to test.
For MMIND_REMAINING command:

```bash
$ ./test 0
```

For MMIND_NEWGAME command, assuming new secret number is 1509:

```bash
$ ./test 1 1509
```

For MMIND_ENDGAME command:

```bash
$ ./test 2
```

### Removing the module

```bash
$ rmmod mastermind
$ rm /dev/mastermind
```



