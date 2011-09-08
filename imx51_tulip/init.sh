#!/system/bin/sh

TARGET=`getprop ro.product.device`
echo "Enter device $TARGET init shell"

# mount debugfs
mount -t debugfs none /sys/kernel/debug
