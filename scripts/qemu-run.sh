#!/bin/bash
# Boots the QEMU variant of the kernel (make qemu) in the background, with a
# monitor socket so scripts/qemu-drive.py can press keys and take screenshots.
# Set QEMU to your qemu-system-aarch64 if it is not on PATH.
set -e
cd "$(dirname "$0")/../build/qemu"
rm -f mon.sock qemu.pid
"${QEMU:-qemu-system-aarch64}" -M raspi3b -kernel kernel8.img \
  -drive file=sd.img,if=sd,format=raw -device usb-kbd -display none \
  -monitor unix:mon.sock,server,nowait -serial null -daemonize -pidfile qemu.pid "$@"
