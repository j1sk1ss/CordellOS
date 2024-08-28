#!/bin/bash

# Step 1: Create disk image
dd if=/dev/zero of=disk.img bs=512 count=131072

# Step 2: Create new DOS partition table with bootable entry
echo "Creating partition table for disk"
echo -e "n\np\n1\n2048\n\n\na\nw" | fdisk disk.img

# Step 3: Setup loop devices
echo "Attaching disk image to partitions"
LOOP_DEV=$(sudo losetup -Pf --show disk.img)
PART_DEV="${LOOP_DEV}p1"

# Step 4: Format the partition
echo "Formatting partition"
sudo mkdosfs -F32 -f 2 "$PART_DEV"

# Step 5: Mount the newly formatted partition
echo "Mounting and copying files"
sudo mount "$PART_DEV" /mnt
sudo cp -r ../CordellOS/boot /mnt
sudo cp -r ../CordellOS/home /mnt

sudo umount /mnt
sudo losetup -d "$LOOP_DEV"

limine bios-install --force-mbr disk.img

