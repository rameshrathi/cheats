# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies required to build QEMU
# Added python3-venv and python3-pip to satisfy QEMU's new build system requirements
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    python3 \
    python3-venv \
    python3-pip \
    ninja-build \
    pkg-config \
    libglib2.0-dev \
    libpixman-1-dev \
    flex \
    bison \
    libfdt-dev \
    libslirp-dev \
    libgcrypt20-dev \
    telnet \
    device-tree-compiler \
    && rm -rf /var/lib/apt/lists/*

# Set working directory for the build
WORKDIR /opt/build

# Clone the official Xilinx QEMU repository
# RUN git clone --depth 1 https://github.com/Xilinx/qemu.git
RUN git clone --depth 1 -b xilinx_v2024.2 https://github.com/Xilinx/qemu.git

WORKDIR /opt/build/qemu

# Configure QEMU build for ARM and AARCH64
# We only need the softmmu targets for ZynqMP emulation
RUN ./configure \
    --target-list="aarch64-softmmu,arm-softmmu" \
    --disable-kvm \
    --enable-fdt \
    --enable-slirp

# Build and install QEMU
RUN make -j$(nproc) && make install

# Create a workspace directory where you will mount your Linux images and R5 firmware
WORKDIR /workspace

# Expose ports for Serial Connections (A53 and R5)
EXPOSE 4444 5555

# Copy the startup script into the container
COPY run_qemu.sh /usr/local/bin/run_qemu.sh
RUN chmod +x /usr/local/bin/run_qemu.sh

# Set the entrypoint to our script
ENTRYPOINT ["/usr/local/bin/run_qemu.sh"]
