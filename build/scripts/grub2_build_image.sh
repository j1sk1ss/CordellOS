#!/bin/bash

# Step 1: Create disk image
dd if=/dev/zero of=disk.img bs=512 count=131072

# Step 2: Create new DOS partition table with bootable entry
echo "Creating partition table for disk"
echo -e "n\p\1\2048\131071\a\w" | fdisk disk.img

OFFSET=$(fdisk -l disk.img | grep -oP '(?<=start=)\s*\d+')
echo "Partition offset: $OFFSET sectors"

# Step 3: Setup loop devices
echo "Attaching disk image to partitions"
sudo losetup /dev/loop0 disk.img

# Step 4: Format the partition
echo "Formating partitions"
sudo mkdosfs -F32 -f 2 /dev/loop0

# Step 5: Mount the newly formatted partition
echo "Mounting and copy files"
sudo mount /dev/loop0 /mnt
sudo cp -r ../CordellOS/boot /mnt
sudo cp -r ../CordellOS/home /mnt

# Step 6: Install GRUB using grub-install
sudo grub2-install --boot-directory=/mnt/boot --root-directory=/mnt --no-floppy --target=i386-pc --modules="normal part_msdos multiboot" /dev/loop0 --force

sudo umount /mnt
sudo losetup -d /dev/loop0
