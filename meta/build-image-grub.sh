set -e

die() {
    echo "die: $*"
    exit 1
}

if [ "$(id -u)" != 0 ]; then
    exec sudo -E -- "$0" "$@" || die "this script needs to run as root"
else
    : "${SUDO_UID:=0}" "${SUDO_GID:=0}"
fi

grub=$(command -v grub-install 2>/dev/null) || true
if [ -z "$grub" ]; then
    grub=$(command -v grub2-install 2>/dev/null) || true
fi
if [ -z "$grub" ]; then
    echo "can't find a grub-install or grub2-install binary, oh no"
    exit 1
fi
echo "using grub-install at ${grub}"

disk_usage() {
if [ "$(uname -s)" = "Darwin" ]; then
    du -sm "$1" | cut -f1
else
    du -sm --apparent-size "$1" | cut -f1
fi
}

DISK_SIZE=$(($(disk_usage "$SERENITY_SOURCE_DIR/Base") + $(disk_usage Root) + 300))

echo "setting up disk image..."
if [ "$1" = "ebr" ]; then
    DISK_SIZE=
fi
dd if=/dev/zero of=grub_disk_image bs=1M count="${DISK_SIZE:-800}" status=none || die "couldn't create disk image"
chown "$SUDO_UID":"$SUDO_GID" grub_disk_image || die "couldn't adjust permissions on disk image"
echo "done"

printf "creating loopback device... "
dev=$(losetup --find --partscan --show grub_disk_image)
if [ -z "$dev" ]; then
    die "couldn't mount loopback device"
fi
echo "loopback device is at ${dev}"

cleanup() {
    if [ -d mnt ]; then
        printf "unmounting filesystem... "
        umount mnt || ( sleep 1 && sync && umount mnt )
        rmdir mnt
        echo "done"
    fi

    if [ -e "${dev}" ]; then
        printf "cleaning up loopback device... "
        losetup -d "${dev}"
        echo "done"
    fi
}
trap cleanup EXIT

printf "creating partition table... "
if [ "$1" = "mbr" ]; then
    parted -s "${dev}" mklabel msdos mkpart primary ext2 1MiB 100% -a minimal set 1 boot on || die "couldn't partition disk"
    partition_number="p1"
    partition_scheme="mbr"
elif [ "$1" = "gpt" ]; then
    parted -s "${dev}" mklabel gpt mkpart BIOSBOOT ext3 1MiB 8MiB mkpart OS ext2 8MiB 290MiB set 1 bios_grub || die "couldn't partition disk"
    partition_number="p2"
    partition_scheme="gpt"
elif [ "$1" = "ebr" ]; then
    parted -s "${dev}" mklabel msdos mkpart primary 32k 200MiB mkpart primary 200MiB 201MiB mkpart primary 201MiB 202MiB mkpart extended 250MiB 739MiB mkpart logical 372MiB 739MiB -a minimal set 1 boot on || die "couldn't partition disk"
    partition_number="p5"
    partition_scheme="ebr"
else
    parted -s "${dev}" mklabel msdos mkpart primary ext2 1MiB 100% -a minimal set 1 boot on || die "couldn't partition disk"
    partition_number="p1"
    partition_scheme="mbr"
fi

echo "done"