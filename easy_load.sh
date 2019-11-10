make
insmod mastermind.ko mmind_number="4283"
mknod /dev/mastermind c 239 0
echo "1234" > /dev/mastermind
cat /dev/mastermind
echo "4513" > /dev/mastermind
cat /dev/mastermind
