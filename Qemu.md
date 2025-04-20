## Using QEMU for Microcontroller Emulation & Debugging

This guide provides an overview and instructions on using QEMU (Quick EMUlator) to emulate microcontroller systems, run firmware, log output, and perform debugging.

## Overview of QEMU for Embedded Systems

QEMU is a powerful open-source machine emulator and virtualizer. For microcontroller development, its **System Emulation** mode is key. In this mode, QEMU emulates:

* **CPU Core:** Executes the instruction set of architectures like ARM (Cortex-M, Cortex-A), RISC-V, MIPS, etc.
* **Memory Map:** Provides simulated memory (RAM, Flash).
* **Basic Peripherals:** Often includes models for UARTs, timers, interrupt controllers (e.g., ARM NVIC), and sometimes basic GPIO, depending on the specific *board* QEMU supports.

### Key Concepts & Caveats

* **Architecture vs. Specific MCU:** QEMU excels at emulating CPU *architectures* and a selection of specific *boards/machines* (e.g., `mps2-an385`, `stellaris`, `virt`).
* **❗ Emulating *Any* Microcontroller is Not Possible Out-of-the-Box:** QEMU **cannot** magically emulate every specific microcontroller model. Vendor-specific peripherals (unique timers, ADCs, communication interfaces like specific I2C/SPI controllers, USB device functions) require dedicated models within QEMU, which often don't exist unless someone has written them.
* **Strengths:** QEMU is excellent for testing CPU-bound code, standard peripheral interactions (UART, timers), interrupt handling, and algorithm verification.
* **Peripheral Limitations:** Emulating complex or custom peripherals is often the biggest challenge.

---

## Steps for Emulation (Supported Hardware)

Here’s the general workflow:

1.  **Install QEMU:**
    * **Debian/Ubuntu:** `sudo apt update && sudo apt install qemu-system-arm qemu-system-misc` (add packages for other architectures like `qemu-system-riscv64` as needed).
    * **Fedora:** `sudo dnf install qemu-system-arm qemu-system-misc`
    * **macOS (Homebrew):** `brew install qemu`
    * **Windows:** Download from the [QEMU website](https://www.qemu.org/download/) or use package managers (Chocolatey, Scoop).

2.  **Obtain/Build Your Executable (ELF File):**
    * You need firmware compiled for the target architecture using a cross-compiler (e.g., `arm-none-eabi-gcc`).
    * The executable (usually an `.elf` file) should contain debug symbols (`-g` flag during compilation) for effective debugging.
    * *Example compilation (ARM Cortex-M):*
        ```bash
        # Ensure you have the appropriate startup code and linker script
        arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb \
                          -specs=nosys.specs -nostartfiles \
                          your_code.c startup_code.s \
                          -T linker_script.ld \
                          -o firmware.elf -g
        ```
    * Your code's memory map assumptions (defined in the linker script) should align with the QEMU machine model.

3.  **Identify QEMU Command and Machine:**
    * Command format: `qemu-system-<arch>` (e.g., `qemu-system-arm`).
    * List available machines for an architecture:
        ```bash
        qemu-system-arm -M ?
        qemu-system-riscv32 -M ?
        ```
    * Choose a machine (`-M <machine_name>`) that matches or is close to your target (e.g., `mps2-an385` for Cortex-M3/M4, `virt` for generic RISC-V).

4.  **Run the Emulation:**
    * Basic command:
        ```bash
        qemu-system-<arch> -M <machine_name> -kernel <path_to_firmware.elf> [options]
        ```
    * *Example (ARM Cortex-M3 on MPS2 board):*
        ```bash
        qemu-system-arm -M mps2-an385 -cpu cortex-m3 -kernel firmware.elf -nographic
        ```
    * *Example (RISC-V 'virt' machine, bare-metal):*
        ```bash
        qemu-system-riscv32 -M virt -kernel firmware.elf -bios none -nographic
        # '-bios none' is often needed for bare-metal to prevent loading a default BIOS/bootloader
        # '-nographic' prevents QEMU from opening an empty display window
        ```

---

## Logging Output via Emulated Serial (UART)

Redirect the emulated microcontroller's UART output to your host terminal or a file.

* **To Terminal (`stdio`):** Easiest method. Firmware `printf` (or equivalent) output appears in the console where QEMU was launched.
    ```bash
    qemu-system-arm [...] -kernel firmware.elf -serial stdio
    ```
* **To Monitor Console (`mon:stdio`):** Multiplexes UART output with the QEMU monitor. Use `Ctrl-A C` to switch views.
    ```bash
    qemu-system-arm [...] -kernel firmware.elf -serial mon:stdio
    ```
* **To File:**
    ```bash
    qemu-system-arm [...] -kernel firmware.elf -serial file:output.log
    ```
* **To Pseudo-terminal (`pty`):** Creates a pseudo-terminal device on the host that another application can connect to.
    ```bash
    qemu-system-arm [...] -kernel firmware.elf -serial pty # QEMU will print the pty device path
    ```

---

## Debugging with GDB

QEMU integrates a GDB server for powerful source-level debugging.

1.  **Start QEMU with GDB Server:**
    * Use `-s`: Shortcut for `-gdb tcp::1234` (listens on port 1234).
    * Use `-S`: Freezes the CPU at startup, waiting for GDB to connect and issue a `continue` command.
    ```bash
    # Example: Wait for GDB connection before starting
    qemu-system-arm -M mps2-an385 -kernel firmware.elf -serial stdio -s -S -nographic
    ```

2.  **Connect GDB:**
    * Use the appropriate GDB for your target (e.g., `arm-none-eabi-gdb`, `riscv64-unknown-elf-gdb`, or `gdb-multiarch`).
    * Launch GDB with the ELF file (contains symbols):
        ```bash
        arm-none-eabi-gdb firmware.elf
        ```
    * Inside GDB, connect to QEMU:
        ```gdb
        (gdb) target remote localhost:1234
        Remote debugging using localhost:1234
        Reset_Handler () at startup_code.s:50
        50          ldr r0, =_estack
        ```

3.  **Debug using GDB Commands:**
    * `b <location>`: Set breakpoint (e.g., `b main`, `b my_func`, `b file.c:42`).
    * `c` or `continue`: Resume execution.
    * `s` or `step`: Step one source line (steps into functions).
    * `n` or `next`: Step one source line (steps over functions).
    * `si` / `ni`: Step one instruction.
    * `p <expr>`: Print variable/expression value (e.g., `p my_variable`, `p/x reg_value`).
    * `info registers`: Show CPU registers.
    * `x/<N><F><S> <addr>`: Examine memory (e.g., `x/10wx 0x20000000`).
    * `layout src` / `layout asm` / `layout regs`: Enable Text User Interface (TUI).
    * `monitor <qemu_cmd>`: Send commands to the QEMU monitor (e.g., `monitor info mem`).
    * `disconnect`: Disconnect from target.
    * `quit`: Exit GDB.

---

## Emulating Peripherals (Sensors, Serial, USB, etc.)

This is where QEMU's capabilities vary significantly.

* **Serial (UART):** Generally well-supported via the `-serial` option (see Logging section).
* **Timers, Interrupt Controllers, Basic GPIO:** Often included in the specific `-M <machine_name>` model. Check QEMU docs for details on your chosen machine.
* **Network:** Some machines support emulated NICs (`-netdev`, `-device`). More common for systems running OSes.
* **USB:**
    * QEMU can emulate USB host controllers (`-usb`).
    * Can attach *emulated* standard devices: `-device usb-mouse`, `usb-kbd`, `usb-serial`, `usb-storage,drive=...`
    * Can pass through *physical* host USB devices: `-device usb-host,vendorid=...,productid=...` (requires permissions).
    * **Limitation:** Emulating *specific* complex USB device logic (e.g., your MCU acting as a custom CDC-ACM device or a vendor-specific device) usually requires writing a custom QEMU device model.
* **Sensors & Custom Peripherals (I2C, SPI, ADC, DAC):**
    * **❗ Major Limitation:** QEMU typically **does not** have built-in models for specific sensors (e.g., BME280, MPU6050) or the exact peripheral implementations (SPI/I2C controllers, ADCs) found on a specific vendor's chip.
    * **Workarounds:**
        1.  **Mocking/Simulation in Firmware:** Use conditional compilation (`#ifdef QEMU_ENV ... #endif`) in your driver/HAL code to return simulated data when running in QEMU, bypassing actual hardware interaction.
        2.  **Write a QEMU Device Model (Very Advanced):** Contribute a C model for the peripheral to the QEMU source code. Requires deep knowledge of QEMU internals (QOM, Memory API, IRQs).
        3.  **External Co-simulation:** Use QEMU's communication features (e.g., `-serial pipe:...`) to talk to a separate host process that simulates the peripheral's behavior.

---

## Summary

* QEMU is a powerful tool for emulating microcontroller **CPU cores** and **standard peripherals** (UART, timers, interrupts) for supported architectures and board models.
* It enables **firmware execution** without hardware and provides excellent **source-level debugging** capabilities via GDB.
* Use `-serial stdio` for easy output logging and `-s -S` for GDB debugging.
* **Be realistic about peripheral support.** QEMU won't automatically emulate every specific sensor or complex on-chip peripheral. Plan to **mock hardware interactions** in your code for QEMU testing or rely on real hardware for full validation.
* Always consult the QEMU documentation for the specific `-M <machine_name>` you are using.


## ------------------   INSTRUCTIONS   -----------------------

### General Steps to Emulate a Microcontroller

Follow these steps to set up and run microcontroller firmware in QEMU:

1.  **Install QEMU**
    * **Linux (Debian/Ubuntu):**
        ```bash
        sudo apt update
        sudo apt install qemu-system-arm qemu-system-misc
        ```
    * **macOS (Homebrew):**
        ```bash
        brew install qemu
        ```
    * **Windows:** Download installers from the [QEMU website](https://www.qemu.org/download/) or use WSL (Windows Subsystem for Linux) and follow Linux instructions.
    * **Verify Installation:**
        ```bash
        qemu-system-arm --version
        ```

2.  **Prepare the Executable**
    * Compile your firmware for the target microcontroller architecture using an appropriate cross-compiler toolchain (e.g., `arm-none-eabi-gcc` for ARM Cortex-M).
    * The output should typically be an ELF file (`.elf`) which includes debug symbols, or a raw binary (`.bin`).
    * *Example compilation for ARM Cortex-M:*
        ```bash
        # Assumes you have main.c and a suitable linker script (linker.ld)
        arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -nostdlib -T linker.ld main.c -o firmware.elf -g
        ```
        *(Note: `-g` adds debug symbols, essential for GDB)*

3.  **Choose a QEMU Machine (`-M`)**
    * QEMU emulates specific hardware boards or "machines". You need to select one that matches or is similar to your target.
    * **List available machines for ARM:**
        ```bash
        qemu-system-arm -M ?
        ```
    * **Commonly used machines:**
        * `stm32f4-discovery`: Emulates aspects of the ST STM32F4 Discovery board (Cortex-M4 based).
        * `mps2-an385`, `mps2-an505`: ARM MPS2/MPS3 boards, good for generic Cortex-M3/M4/M7/M33 testing.
        * `netduino2`: Based on STM32F2.
        * `virt`: A generic virtual ARM machine, often used for OS testing but can be used for bare-metal.

4.  **Run the Executable**
    * Use the `qemu-system-<arch>` command (e.g., `qemu-system-arm`).
    * **Basic command structure:**
        ```bash
        qemu-system-arm -M <machine_name> -kernel <path_to_firmware.elf> [options]
        ```
    * *Example using `stm32f4-discovery`:*
        ```bash
        qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic
        ```
    * **Common Options:**
        * `-M <machine_name>`: **Required.** Specifies the machine model.
        * `-kernel <file>`: **Required.** Loads the ELF/binary executable into memory.
        * `-nographic`: Prevents QEMU from opening a graphical window; useful for headless/serial-only applications. Redirects serial output if `-serial stdio` isn't used.

---

## Debugging with GDB

QEMU has a built-in GDB server for powerful debugging.

1.  **Start QEMU with GDB Server Enabled:**
    * Add the `-S` and `-gdb` options to the QEMU command line.
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -S -gdb tcp::1234
    ```
    * **Options Explained:**
        * `-S`: Freezes the CPU at startup, waiting for a debugger connection.
        * `-gdb tcp::1234`: Starts the GDB server listening on TCP port 1234. (`-s` is a shortcut for `-gdb tcp::1234`).

2.  **Connect GDB:**
    * Use the GDB client from your cross-compiler toolchain (e.g., `arm-none-eabi-gdb`).
    * Launch GDB with your ELF file (which contains debug symbols):
        ```bash
        arm-none-eabi-gdb firmware.elf
        ```
    * Inside GDB, connect to the QEMU GDB server:
        ```gdb
        (gdb) target remote localhost:1234
        ```
    * Set breakpoints and start execution:
        ```gdb
        (gdb) break main  # Set a breakpoint at the main function
        (gdb) continue    # Start execution
        ```

---

## Logging to Terminal (Serial Output)

Redirect the emulated microcontroller's UART output.

* **Redirect to Host Terminal (`stdio`):**
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial stdio
    ```
    *(Firmware output directed to the UART will appear in the terminal where you launched QEMU)*

* **Log to a File:**
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial file:output.log
    ```

---

## Emulating Peripherals

QEMU's support varies depending on the machine model (`-M`). Basic peripherals are often included.

* **Serial (UART):** Generally well-supported via `-serial`.
    * *Example using a pseudo-terminal (pty):*
        ```bash
        # QEMU will print the path to the PTY device (e.g., /dev/pts/0)
        qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial pty
        ```
    * Connect a terminal emulator (like `minicom`, `screen`, `picocom`) to the PTY device on the host:
        ```bash
        minicom -D /dev/pts/0 # Replace /dev/pts/0 with the actual device path
        ```
* **SPI, I2C, GPIO:** Support is **highly dependent** on the chosen machine model (`-M`). Check QEMU documentation for the specific machine. Generic models might exist, but they may not perfectly match your target chip's peripheral behavior.
* **USB:** Emulation is complex and often limited for microcontroller device roles. Basic host controller emulation and attaching standard USB device models (like HID, mass storage) is possible, especially with the `virt` machine. USB tracing can be enabled with `-trace usb_*`.
    ```bash
    # Example: Attaching an emulated HID device to the 'virt' machine
    qemu-system-arm -M virt -kernel firmware.elf -usb -device usb-hid
    ```
* **Sensors (via I2C/SPI):** QEMU typically **does not** have built-in models for specific sensors. You usually need to:
    1.  **Simulate/Mock:** Modify your firmware to provide simulated sensor data when running under QEMU.
    2.  **Extend QEMU:** Write a custom device model in C (Advanced).

---

## Custom Hardware Emulation

If your specific hardware isn't directly supported:

1.  **Use a Similar Machine:** Choose an existing `-M` option that is architecturally similar (e.g., same CPU core) or use a generic board like `virt` (ARM) or `mps2-an*`.
2.  **Extend QEMU (Advanced):**
    * Clone the QEMU source code:
        ```bash
        git clone [https://github.com/qemu/qemu.git](https://github.com/qemu/qemu.git)
        cd qemu
        ```
    * Configure and build QEMU for your target architecture:
        ```bash
        # Example for ARM
        ./configure --target-list=arm-softmmu
        make -j$(nproc) # Build using multiple cores
        ```
    * Add custom device models (written in C) to the appropriate subdirectories (e.g., `hw/arm/`, `hw/i2c/`, `hw/spi/`). This requires understanding QEMU's internal APIs (QOM, memory regions, IRQs).

---

## Specific Scenarios & Examples

### 1. Emulating STM32F4

* **Target:** Run firmware on an emulated STM32F4-Discovery board and log UART output.
* **Compile (`main.c`, `stm32f4.ld`):**
    ```bash
    arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -nostdlib -T stm32f4.ld main.c -o firmware.elf -g
    ```
* **Run & Log:**
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial stdio
    ```
* **Debug:**
    ```bash
    # Terminal 1: Start QEMU waiting for GDB
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -S -gdb tcp::1234

    # Terminal 2: Connect GDB
    arm-none-eabi-gdb firmware.elf
    (gdb) target remote localhost:1234
    (gdb) # Debug commands...
    ```

### 2. Emulating Serial Connection (PTY)

* **Goal:** Simulate a UART connection accessible from the host.
* **Run QEMU:**
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial pty
    # Note the PTY device path printed by QEMU (e.g., /dev/pts/1)
    ```
* **Connect from Host:**
    ```bash
    minicom -D /dev/pts/1 # Or screen /dev/pts/1, picocom /dev/pts/1, etc.
    ```

### 3. Emulating USB (Basic Example)

* **Goal:** Attach a simple emulated USB device (limited microcontroller relevance).
* **Run QEMU (`virt` machine often better for USB):**
    ```bash
    qemu-system-arm -M virt -kernel firmware.elf -usb -device usb-hid -nographic
    ```
* **Enable USB Trace Events:**
    ```bash
    qemu-system-arm -M virt -kernel firmware.elf -usb -device usb-hid -trace "usb_*" -nographic
    ```

### 4. Emulating Sensors (I2C/SPI) - Conceptual

* **Challenge:** Direct sensor emulation is usually not feasible out-of-the-box.
* **Approach:** Mocking in firmware or using generic device models if available for the machine.
    ```bash
    # Example using a generic I2C device model (if supported by the machine)
    # The firmware needs to know how to interact with this generic device
    qemu-system-arm -M mps2-an385 -kernel firmware.elf -nographic -device i2c-dev [...]
    ```

### Example: Simple STM32F4 UART Output

* **`main.c`:**
    ```c
    #include <stdint.h>

    // Define UART Data Register address for STM32F4 (USART1 DR)
    #define USART1_DR (*(volatile uint32_t *)0x40011004)

    // Basic delay function (replace with proper timer/SysTick later)
    void simple_delay(volatile uint32_t count) {
        while(count--);
    }

    int main() {
        // NOTE: In a real application, you MUST initialize the UART peripheral
        // (clocks, GPIO pins, baud rate, etc.) before writing to the data register.
        // This example skips initialization for brevity, relying on QEMU's
        // default state for the stm32f4-discovery machine's UART, which might work.

        USART1_DR = 'H'; // Send 'H'
        simple_delay(10000); // Small delay
        USART1_DR = 'i'; // Send 'i'
        simple_delay(10000); // Small delay

        while (1) {
            // Loop forever
        }
        return 0; // Should not be reached
    }
    ```
* **Compile (assuming `stm32f4.ld` linker script):**
    ```bash
    arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -nostdlib -T stm32f4.ld main.c -o firmware.elf -g -O0
    # -O0 disables optimizations, often helpful for initial debugging
    ```
* **Run:**
    ```bash
    qemu-system-arm -M stm32f4-discovery -kernel firmware.elf -nographic -serial stdio
    ```
* **Expected Output:** `Hi` (or potentially just `H` or nothing if UART defaults aren't sufficient - proper initialization is key).

---

## Tips and Further Resources

* **Consult Documentation:** Refer to the official [QEMU Documentation](https://www.qemu.org/docs/master/) for detailed information on machines, devices, and options.
* **Architecture Match:** Ensure your compiled executable (`-kernel`) matches the target architecture of `qemu-system-<arch>` and the chosen machine (`-M`).
* **Trace Execution:** Use `-d <option>` for detailed tracing (e.g., `-d cpu,exec,in_asm` shows CPU state, executed instructions). Use `-d help` to list trace options.
* **Start Simple:** Begin with well-supported boards like `stm32f4-discovery` or `mps2-an*` before attempting more obscure hardware.
* **Community:** Check Stack Overflow, Reddit (r/qemu), and the QEMU mailing lists for specific issues.
* **Be Specific:** When asking for help, provide details about your target microcontroller, the QEMU machine you're using, the command line, and the specific problem.

