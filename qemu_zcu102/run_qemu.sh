#!/bin/bash
# run_qemu.sh - Script to launch ZCU102 emulation with A53 Linux and R5 Firmware

# Default file names (You can override these by mounting your own files into /workspace)
LINUX_KERNEL=${LINUX_KERNEL:-"Image"}
LINUX_DTB=${LINUX_DTB:-"system.dtb"}
LINUX_ROOTFS=${LINUX_ROOTFS:-"rootfs.cpio.gz.u-boot"}
R5_FIRMWARE=${R5_FIRMWARE:-"npc_firmware.elf"}

echo "=========================================================="
echo " Starting Xilinx ZCU102 QEMU Emulation"
echo "=========================================================="
echo " Expected Files in /workspace:"
echo "  - Linux Kernel : $LINUX_KERNEL"
echo "  - Device Tree  : $LINUX_DTB"
echo "  - RootFS       : $LINUX_ROOTFS"
echo "  - R5 Firmware  : $R5_FIRMWARE"
echo ""
echo " Serial Connections:"
echo "  - A53 (Linux)  : telnet localhost 4444"
echo "  - R5  (Bare)   : telnet localhost 5555"
echo "=========================================================="

# Check if at least the R5 firmware exists (since that's your custom work)
if [ ! -f "/workspace/$R5_FIRMWARE" ]; then
    echo "WARNING: R5 firmware '$R5_FIRMWARE' not found in /workspace."
    echo "QEMU will start, but the R5 core will have nothing to execute."
fi

# QEMU Command Breakdown:
# -M xlnx-zcu102             : Emulate the ZCU102 board
# -m 4G                      : 4GB RAM (Standard for ZCU102)
# -nographic                 : Disable graphical window
# -serial telnet:...         : UART0 (A53) mapped to port 4444
# -serial telnet:...         : UART1 (R5) mapped to port 5555
# -kernel, -dtb, -initrd     : Boot A53 Linux (cpu-num 0-3)
# -append                    : Linux boot arguments routing console to ttyPS0
# -device loader,...         : Load your custom baremetal ELF into the R5 core. 
#                              In ZynqMP, cpu-num=4 targets the first Cortex-R5 core.

qemu-system-aarch64 \
    -M xlnx-zcu102 \
    -m 4G \
    -nographic \
    -serial telnet:0.0.0.0:4444,server,nowait \
    -serial telnet:0.0.0.0:5555,server,nowait \
    -kernel "/workspace/$LINUX_KERNEL" \
    -dtb "/workspace/$LINUX_DTB" \
    -initrd "/workspace/$LINUX_ROOTFS" \
    -append "console=ttyPS0,115200 root=/dev/ram rw earlycon" \
    -device loader,file="/workspace/$R5_FIRMWARE",cpu-num=4
