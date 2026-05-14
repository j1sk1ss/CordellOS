#!/bin/bash
set -euo pipefail

IMAGE="disk.img"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT_DIR"

IMAGE_ABS="$(readlink -f "$IMAGE")"
QEMU_KERNEL_BOOT_MARKER="build/.qemu-kernel-boot"
GRUB_BIOS_DIR="${GRUB_BIOS_DIR:-/usr/lib/grub/i386-pc}"
MOUNT_DIR="${MOUNT_DIR:-/mnt/cordellos}"
SECTOR_SIZE=512
IMAGE_SECTORS=131072
PART_START=2048
PART_SECTORS=$((IMAGE_SECTORS - PART_START))
PART_OFFSET=$((PART_START * SECTOR_SIZE))
PART_SIZE=$((PART_SECTORS * SECTOR_SIZE))
LOOP_DEVICE=""
PART_LOOP_DEVICE=""
SUDO=(sudo)

if [[ "${EUID}" -eq 0 ]]; then
    SUDO=()
fi

ensure_loop_devices() {
    if [[ ! -e /dev/loop-control ]]; then
        "${SUDO[@]}" mknod /dev/loop-control c 10 237 || true
    fi

    for index in $(seq 0 7); do
        if [[ ! -e "/dev/loop$index" ]]; then
            "${SUDO[@]}" mknod "/dev/loop$index" b 7 "$index" || true
        fi
    done
}

has_grub_bios_modules() {
    [[ -f "$GRUB_BIOS_DIR/modinfo.sh" ]]
}

cleanup() {
    set +e
    if mountpoint -q "$MOUNT_DIR"; then
        "${SUDO[@]}" umount "$MOUNT_DIR"
    fi
    if [[ -n "$PART_LOOP_DEVICE" ]]; then
        "${SUDO[@]}" losetup -d "$PART_LOOP_DEVICE"
    fi
    if [[ -n "$LOOP_DEVICE" ]]; then
        "${SUDO[@]}" losetup -d "$LOOP_DEVICE"
    fi
}

unmount_loop_mounts() {
    local dev="$1"
    findmnt -rn -S "$dev" -o TARGET | while read -r mountpoint_path; do
        if [[ -n "$mountpoint_path" ]]; then
            "${SUDO[@]}" umount "$mountpoint_path"
        fi
    done

    for part in "$dev"p*; do
        [[ -e "$part" ]] || continue
        findmnt -rn -S "$part" -o TARGET | while read -r mountpoint_path; do
            if [[ -n "$mountpoint_path" ]]; then
                "${SUDO[@]}" umount "$mountpoint_path"
            fi
        done
    done
}

detach_stale_loops() {
    "${SUDO[@]}" losetup -j "$IMAGE_ABS" | cut -d: -f1 | while read -r dev; do
        if [[ -n "$dev" ]]; then
            unmount_loop_mounts "$dev"
            "${SUDO[@]}" losetup -d "$dev"
        fi
    done
}

trap cleanup EXIT

ensure_loop_devices
detach_stale_loops
rm -f "$QEMU_KERNEL_BOOT_MARKER"

echo "Creating disk image"
dd if=/dev/zero of="$IMAGE" bs="$SECTOR_SIZE" count="$IMAGE_SECTORS"

echo "Creating partition table for disk"
printf 'label: dos\nunit: sectors\n\nstart=%s, size=%s, type=c, bootable\n' "$PART_START" "$PART_SECTORS" | sfdisk "$IMAGE"

echo "Attaching disk image to loop device"
LOOP_DEVICE="$("${SUDO[@]}" losetup --find --show "$IMAGE")"

echo "Attaching FAT partition slice to loop device"
PART_LOOP_DEVICE="$("${SUDO[@]}" losetup --find --show --offset "$PART_OFFSET" --sizelimit "$PART_SIZE" "$IMAGE")"

echo "Formatting partition $PART_LOOP_DEVICE"
"${SUDO[@]}" mkdosfs -F32 -f 2 "$PART_LOOP_DEVICE"

echo "Mounting and copying files"
"${SUDO[@]}" mkdir -p "$MOUNT_DIR"
"${SUDO[@]}" mount "$PART_LOOP_DEVICE" "$MOUNT_DIR"
"${SUDO[@]}" rm -rf "$MOUNT_DIR/boot" "$MOUNT_DIR/home"
"${SUDO[@]}" cp -r build/CordellOS/boot "$MOUNT_DIR/"
"${SUDO[@]}" cp -r build/CordellOS/home "$MOUNT_DIR/"
"${SUDO[@]}" cp src/kernel/font.psf "$MOUNT_DIR/home/shell.psf"

"${SUDO[@]}" mkdir -p "$MOUNT_DIR/boot/grub"
"${SUDO[@]}" tee "$MOUNT_DIR/boot/grub/grub.cfg" >/dev/null <<'EOF'
set timeout=0
set default=0

menuentry "CordellOS" {
    multiboot /boot/kernel/kernel.elf
    boot
}
EOF

echo "Installing GRUB"
if has_grub_bios_modules; then
    GRUB_INSTALL="$(command -v grub2-install || command -v grub-install)"
    "${SUDO[@]}" "$GRUB_INSTALL" \
        --target=i386-pc \
        --directory="$GRUB_BIOS_DIR" \
        --boot-directory="$MOUNT_DIR/boot" \
        --no-floppy \
        --modules="normal part_msdos fat multiboot" \
        --skip-fs-probe \
        --force \
        "$LOOP_DEVICE"
else
    echo "GRUB BIOS modules were not found at $GRUB_BIOS_DIR."
    echo "Skipping GRUB install; QEMU will boot build/CordellOS/boot/kernel/kernel.elf directly."
    mkdir -p "$(dirname "$QEMU_KERNEL_BOOT_MARKER")"
    touch "$QEMU_KERNEL_BOOT_MARKER"
fi

echo "Disk image is ready: $IMAGE"
