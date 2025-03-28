# ARM Assembly Language
**I. Fundamentals**

1.  **What is Assembly Language?**

    *   A low-level programming language that uses mnemonic codes (human-readable abbreviations) to represent machine instructions.
    *   Each assembly instruction typically corresponds to a single machine code instruction.
    *   Requires an *assembler* to translate assembly code into machine code (executable binary).

2.  **ARM Architecture: A Brief Overview**

    *   **RISC (Reduced Instruction Set Computing):** ARM is a RISC architecture. This means it emphasizes:
        *   A small, simple instruction set.
        *   Instructions that execute in a single clock cycle (ideally).
        *   A large number of registers.
    *   **Load-Store Architecture:** Data processing operations primarily occur between registers. Data is loaded from memory into registers (using `LDR` instructions) and stored from registers back to memory (using `STR` instructions). Direct operations between memory locations are rare.
    *   **Conditional Execution:** Many ARM instructions can be conditionally executed based on the state of the *condition code flags*.
    *   **Multiple Variants:** ARM architecture has different versions (ARMv6, ARMv7, ARMv8, etc.), each with its own instruction set extensions. We'll focus on a common subset applicable across many ARM architectures.

3.  **ARM Registers**

    ARM has a set of 31 general-purpose registers (R0-R30) and one program counter (PC) register. Some registers have dedicated roles:

    *   **R0-R3:** Used for passing arguments to functions and returning values from functions. R0 is almost always the return value.
    *   **R4-R11:** Used for local variables within functions. These are caller saved, meaning a function must save them on the stack before modifying them, and restore them when returning.
    *   **R12 (IP):** Intra-Procedure call scratch register. Usually used by compilers and assemblers for intermediate temporary values.
    *   **R13 (SP):** Stack Pointer. Points to the top of the stack. Critical for function calls, local variables, and temporary storage.
    *   **R14 (LR):** Link Register. Stores the return address when a function is called using the `BL` (Branch and Link) instruction.
    *   **R15 (PC):** Program Counter. Points to the address of the next instruction to be executed. Directly manipulating the PC is generally discouraged (use branch instructions instead).
    *   **R30 (FP):** Frame Pointer. Points to the beginning of the current stack frame.

    These registers can also be referred to by names `w0`-`w30` for 32-bit architectures and `x0`-`x30` for 64-bit architectures.

    **The CPSR (Current Program Status Register)**

    A special register that holds status information about the processor. Key flags within the CPSR include:

    *   **N (Negative):** Set if the result of an operation is negative.
    *   **Z (Zero):** Set if the result of an operation is zero.
    *   **C (Carry):** Set if an operation resulted in a carry-out.
    *   **V (Overflow):** Set if an operation resulted in an overflow.
    *   **I (Interrupt Disable):** Enables or disables interrupt requests.
    *   **T (Thumb State):** Indicates whether the processor is executing ARM or Thumb instructions. (Important for certain ARM architectures, less relevant for modern AArch64).

4.  **Basic Instruction Syntax**

    ARM assembly instructions typically follow this format:

    ```assembly
    <instruction> <destination>, <operand1>, <operand2>
    ```

    *   `<instruction>`: The mnemonic code for the operation (e.g., `ADD`, `SUB`, `LDR`, `STR`).
    *   `<destination>`: The register where the result will be stored (usually).
    *   `<operand1>, <operand2>`: The operands (registers, immediate values, or memory addresses) that the instruction will operate on.

5.  **Addressing Modes**

    How operands are specified. Crucial for accessing data in memory.

    *   **Register Direct:** Operand is a register (e.g., `R0`, `R1`).  `ADD R0, R1, R2` (R0 = R1 + R2)
    ```assembly
    ADD R0, R1, R2
    ```
    *   **Immediate:** Operand is a constant value (e.g., `#10`, `#0xFF`).  `MOV R0, #10` (R0 = 10)
    ```assembly
    MOV R0, #10
    ```
    *   **Register Indirect:** Operand is a memory address stored in a register.  `LDR R0, [R1]` (R0 = value at memory address stored in R1).
    ```assembly
    LDR R0, [R1]
    ```
    *   **Register Indirect with Offset:** Operand is a memory address calculated by adding an offset to a register.  `LDR R0, [R1, #4]` (R0 = value at memory address (R1 + 4)).
    ```assembly
    LDR R0, [R1, #4]
    ```
    *   **Pre-indexed Addressing:** The base register is updated *before* the memory access. `LDR R0, [R1, #4]!` (R0 = value at memory address (R1 + 4); R1 = R1 + 4).
    ```assembly
    LDR R0, [R1, #4]!
    ```
    *   **Post-indexed Addressing:** The base register is updated *after* the memory access. `LDR R0, [R1], #4` (R0 = value at memory address R1; R1 = R1 + 4).
    ```assembly
    LDR R0, [R1], #4
    ```

**II. Common ARM Instructions**

1.  **Data Transfer Instructions**

    *   `MOV`: Move a value from one register to another or from an immediate value to a register.

        ```assembly
        MOV R0, R1       ; R0 = R1
        MOV R0, #10      ; R0 = 10
        ```
    *   `LDR`: Load a value from memory into a register.

        ```assembly
        LDR R0, [R1]     ; R0 = value at memory address R1
        LDR R0, [R1, #4] ; R0 = value at memory address R1 + 4
        ```
    *   `STR`: Store a value from a register into memory.

        ```assembly
        STR R0, [R1]     ; Memory address R1 = R0
        STR R0, [R1, #4] ; Memory address R1 + 4 = R0
        ```
    *   `PUSH`: Push registers onto the stack. Often used to save registers at the start of a function.

        ```assembly
        PUSH {R4-R7, LR}  ; Push R4, R5, R6, R7, and LR onto the stack
        ```
    *   `POP`: Pop registers from the stack. Used to restore registers before returning from a function.

        ```assembly
        POP {R4-R7, PC}   ; Pop values from the stack into R4, R5, R6, R7, and PC (return from function)
        ```

2.  **Arithmetic and Logical Instructions**

    *   `ADD`: Add two operands.

        ```assembly
        ADD R0, R1, R2   ; R0 = R1 + R2
        ADD R0, R1, #10  ; R0 = R1 + 10
        ```
    *   `SUB`: Subtract two operands.

        ```assembly
        SUB R0, R1, R2   ; R0 = R1 - R2
        ```
    *   `MUL`: Multiply two operands.

        ```assembly
        MUL R0, R1, R2   ; R0 = R1 * R2
        ```
    *   `SDIV`/`UDIV`: Signed/Unsigned division (available on newer ARM architectures).

        ```assembly
        SDIV R0, R1, R2  ; R0 = R1 / R2 (signed)
        UDIV R0, R1, R2  ; R0 = R1 / R2 (unsigned)
        ```
    *   `AND`, `ORR`, `EOR`, `BIC`: Logical AND, OR, Exclusive OR, and Bit Clear (AND with the complement of the second operand).

        ```assembly
        AND R0, R1, R2   ; R0 = R1 & R2
        ORR R0, R1, R2   ; R0 = R1 | R2
        EOR R0, R1, R2   ; R0 = R1 ^ R2
        BIC R0, R1, #0xFF ; R0 = R1 & (~0xFF)  (clear the lower 8 bits)
        ```
    *   `LSL`, `LSR`, `ASR`, `ROR`: Logical Shift Left, Logical Shift Right, Arithmetic Shift Right, Rotate Right.

        ```assembly
        LSL R0, R1, #2   ; R0 = R1 << 2  (shift left by 2 bits)
        LSR R0, R1, #2   ; R0 = R1 >> 2  (shift right by 2 bits, zero-fill)
        ASR R0, R1, #2   ; R0 = R1 >> 2  (shift right by 2 bits, sign-extend)
        ROR R0, R1, #2   ; R0 = R1 rotated right by 2 bits
        ```

3.  **Comparison and Branch Instructions**

    *   `CMP`: Compare two operands. Sets the condition code flags based on the result of the subtraction (operand1 - operand2).

        ```assembly
        CMP R0, R1       ; Compare R0 and R1 (R0 - R1)
        ```
    *   `TST`: Bitwise AND and set flags. A very common idiom for testing a single bit in an operand.

        ```assembly
        TST R0, #0x10    ; Test if bit 4 of R0 is set
        ```
    *   `B`: Branch (unconditional jump).

        ```assembly
        B loop_start    ; Jump to the label 'loop_start'
        ```
    *   `BEQ`, `BNE`, `BGT`, `BLT`, `BGE`, `BLE`: Branch if Equal, Not Equal, Greater Than, Less Than, Greater Than or Equal, Less Than or Equal (conditional jumps). These branches check the condition code flags set by `CMP` or `TST`.

        ```assembly
        CMP R0, R1
        BEQ equal       ; Jump to 'equal' if R0 == R1
        BGT greater     ; Jump to 'greater' if R0 > R1
        ```
    *   `BL`: Branch and Link. Calls a subroutine (function). Saves the return address in the Link Register (LR).

        ```assembly
        BL my_function  ; Call the function 'my_function'
        ```
    *   `BX LR`: Branch Exchange to LR. Returns from a subroutine. Jumps to the address stored in the Link Register.

        ```assembly
        BX LR           ; Return from function
        ```

**III. ARM Assembly Programming: Practical Considerations**

1.  **Function Calls and the Stack**

    *   **The Stack:** A region of memory used for temporary storage, local variables, and passing arguments to functions. The Stack Pointer (SP) points to the top of the stack. The stack grows *downwards* in memory (i.e., pushing onto the stack *decrements* the SP).
    *   **Function Prologue:** The code executed at the *beginning* of a function. Typically involves:
        *   Pushing registers onto the stack to save their values (especially R4-R11 and LR).
        *   Allocating space on the stack for local variables (decrementing the SP).
    *   **Function Epilogue:** The code executed at the *end* of a function, before returning. Typically involves:
        *   Deallocating stack space (incrementing the SP).
        *   Popping registers from the stack to restore their values.
        *   Returning from the function using `BX LR`.

    ```assembly
    my_function:
        PUSH {R4-R7, LR}  ; Save registers
        SUB SP, SP, #16  ; Allocate 16 bytes for local variables
        ; ... function body ...
        ADD SP, SP, #16  ; Deallocate stack space
        POP {R4-R7, PC}   ; Restore registers and return
    ```

2.  **Data Structures and Memory Management**

    *   Assembly language gives you direct control over memory. You can define data structures in memory using directives like `.word`, `.byte`, `.space`.
    *   Dynamic memory allocation (like `malloc` in C) requires interaction with the operating system or a memory management library. This typically involves making system calls.

    ```assembly
    .data
    my_variable: .word 10   ; Allocate a word (4 bytes) and initialize it to 10
    my_string:   .asciz "Hello, world!" ; Allocate a null-terminated string
    buffer:      .space 100  ; Allocate 100 bytes of uninitialized memory
    ```

3.  **Inline Assembly**

    *   Most compilers (like GCC) allow you to embed assembly code directly within your C/C++ code. This can be useful for optimizing critical sections or accessing specific hardware features.
    *   The syntax for inline assembly varies depending on the compiler. A common form is:

    ```c
    int main() {
        int a = 10, b = 20, sum;
        asm (
            "ADD %w[result], %w[in1], %w[in2]"  // Add in1 and in2, store result in result
            : [result] "=r" (sum) // Output operand: sum (modified)
            : [in1] "r" (a), [in2] "r" (b)  // Input operands: a and b
            : // Clobbered registers (if any - registers modified by the inline assembly)
        );
        printf("Sum: %d\n", sum);
        return 0;
    }
    ```

4.  **Debugging**

    *   Use a debugger like GDB (GNU Debugger) to step through your assembly code, inspect registers, and examine memory.
    *   Learn to read disassembly listings generated by compilers or debuggers. This can help you understand how your high-level code is translated into assembly.

5.  **Common Idioms and Best Practices**

    *   **Use meaningful labels:** Give your labels descriptive names to make your code easier to read.
    *   **Comment your code:** Explain what each section of code is doing.
    *   **Follow calling conventions:** Adhere to the standard ARM calling conventions (how arguments are passed, how registers are saved, etc.) to ensure compatibility with other code.
    *   **Optimize for performance:** Consider instruction scheduling, loop unrolling, and other optimization techniques.
    *   **Test thoroughly:** Assembly code can be difficult to debug, so it's important to test your code rigorously.

**IV. A Deeper Look: Advanced Topics**

1.  **SIMD (Single Instruction, Multiple Data): NEON**

    *   ARM's NEON instruction set provides SIMD capabilities, allowing you to perform the same operation on multiple data elements simultaneously. This is crucial for multimedia processing, signal processing, and other computationally intensive tasks.

2.  **Floating-Point Arithmetic**

    *   ARM supports floating-point operations using the VFP (Vector Floating-Point) or the more modern Advanced SIMD (NEON) extensions.

3.  **System Calls (SVC/EABI)**

    *   To interact with the operating system (e.g., for file I/O, memory allocation, process management), you need to make system calls. This typically involves:
        1.  Loading the system call number into a specific register (e.g., `R7`).
        2.  Loading any arguments to the system call into other registers (e.g., `R0-R6`).
        3.  Executing the `SVC` (Supervisor Call) instruction to transfer control to the operating system kernel.
        4.  The kernel performs the requested operation and returns a result code in `R0`.

4.  **Thumb and Thumb-2 Instructions**

    *   Thumb is a 16-bit instruction set that provides higher code density (smaller code size) than the standard 32-bit ARM instruction set. Thumb-2 extends Thumb with 32-bit instructions, offering a good balance between code density and performance.

5.  **Security Considerations**

    *   Buffer overflows, integer overflows, and other vulnerabilities are just as relevant (if not more so) in assembly code. Carefully validate input data and be aware of potential security risks. Return-oriented programming (ROP) is a common exploit technique that relies on chaining together existing code snippets in memory.

**V. Learning Resources**

*   **ARM Architecture Reference Manuals:** The official documentation from ARM. Essential for a deep understanding of the architecture and instruction set.
*   **Online Tutorials:** Numerous websites and tutorials cover ARM assembly programming. Start with introductory tutorials and gradually move to more advanced topics.
*   **Assembler Documentation:** The documentation for your chosen assembler (e.g., GNU Assembler (GAS), Keil Assembler) will provide details on the syntax, directives, and features supported by the assembler.
*   **Example Code:** Study existing ARM assembly code to learn from others and see how different concepts are applied in practice. Look for open-source projects or example code provided by ARM or other developers.
*   **ARM Developer Website:** [https://developer.arm.com/](https://developer.arm.com/)
*   **GCC Inline Assembly Documentation:** [https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html](https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html)

**VI. Hands-on Practice**

The best way to learn ARM assembly is to practice. Here are some suggested exercises:

1.  **Simple Arithmetic:** Write assembly code to add, subtract, multiply, and divide two numbers.
2.  **Factorial:** Write a function to calculate the factorial of a number.
3.  **String Manipulation:** Write code to reverse a string or copy a string from one memory location to another.
4.  **Array Processing:** Write code to find the largest element in an array or sort an array of numbers.
5.  **Blinking LED (if you have an embedded system):** A classic "Hello, world!" program for embedded systems.

**Conclusion**

Learning ARM assembly language is a rewarding endeavor that will give you a deeper appreciation for how computers work and unlock new possibilities for optimization, embedded systems development, and reverse engineering. Start with the fundamentals, practice consistently, and don't be afraid to experiment. Good luck!
