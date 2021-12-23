// SPDX-FileCopyrightText: © 2021 Uri Shaked <uri@wokwi.com>
// SPDX-License-Identifier: MIT


#include "verilog/dv/caravel/defs.h"

#define PROJECT_ID              1

#define reg_spell_pc            (*(volatile uint32_t*)0x30000000)
#define reg_spell_sp            (*(volatile uint32_t*)0x30000004)
#define reg_spell_exec          (*(volatile uint32_t*)0x30000008)
#define reg_spell_ctrl          (*(volatile uint32_t*)0x3000000C)
#define reg_spell_cycles_per_ms (*(volatile uint32_t*)0x30000010)
#define reg_spell_stack_top     (*(volatile uint32_t*)0x30000014)
#define reg_spell_stack_push    (*(volatile uint32_t*)0x30000018)
#define reg_spell_int_enable    (*(volatile uint32_t*)0x30000020)
#define reg_spell_int           (*(volatile uint32_t*)0x30000024)

#define CTRL_RUN                (1 << 0)
#define CTRL_STEP               (1 << 1)
#define CTRL_SRAM_ENABLE        (1 << 2)
#define CTRL_EDGE_INTERRUPTS    (1 << 3)

#define SRAM_WRITE_PORT         31
#define SRAM_BASE_ADDR          0x30FFFC00
#define OPENRAM(addr)           (*(uint32_t*)(SRAM_BASE_ADDR + (addr & 0x3fc)))
#define DATA_START              0x100

#define PACK_SPELL(a, b, c, d) ((a) | (b << 8) | (c << 16) | (d << 24))

#define SPELL_REG_PIN           0x36
#define SPELL_REG_DDR           0x37
#define SPELL_REG_PORT          0x38

#define SPELL_INTR_SLEEP        (1 << 0)
#define SPELL_INTR_STOP         (1 << 1)

#define TEST_RESULT_PASS        0x1
#define TEST_RESULT_FAIL_IRQ    0xc
#define TEST_RESULT_FAIL_SRAM1  0xd
#define TEST_RESULT_FAIL_SRAM2  0xe
#define TEST_RESULT_FAIL_DFF    0xf

volatile int irq_count;

void irq() {
    reg_spell_int = SPELL_INTR_STOP;  // clear the interrupt flag
    irq_count++;
}

void write_progmem(uint8_t addr, uint8_t opcode) {
    reg_spell_stack_push = opcode;
    reg_spell_stack_push = addr;
    reg_spell_exec = '!';
}

void main() {
    // 8 pins for SPELL I/O
	reg_mprj_io_8  = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_9  = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_10 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_11 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_12 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_13 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_14 = GPIO_MODE_USER_STD_BIDIRECTIONAL;
	reg_mprj_io_15 = GPIO_MODE_USER_STD_BIDIRECTIONAL;

    /* Apply configuration */
    reg_mprj_xfer = 1;
    while (reg_mprj_xfer == 1);

    // activate the project with 1st bank of the LA
    reg_la0_iena = 0; // input enable off
    reg_la0_oenb = 0; // output enable bar low (enabled)
    reg_la0_data = 1 << PROJECT_ID;

    // Reset SPELL
    reg_la1_iena = 0;
    reg_la1_oenb = 0;
    reg_la1_data |= 1;
    reg_la1_data &= ~1;

    // Write a quick test program to SPELL's CODE memory
    write_progmem(0, 30);
    write_progmem(1, 25);
    write_progmem(2, '+');
    write_progmem(3, 'z');

    // Start SPELL
    reg_spell_ctrl = CTRL_RUN;

    // Wait for the SPELL to finish
    while (reg_spell_ctrl & CTRL_RUN);

    // At this point, we should have 55 at the top of the stack.
    if (reg_spell_stack_top != 55) {
        reg_mprj_datal = TEST_RESULT_FAIL_DFF << 28;
        return;
    }

    // Test interrupts
    reg_mprj_irq = 0b001; // Enable user IRQ 0
    reg_spell_ctrl = CTRL_EDGE_INTERRUPTS;
    reg_spell_int = 0xff; // Clear any existing interrupt state
    reg_spell_int_enable = SPELL_INTR_STOP;
    irq_count = 0;
    reg_spell_exec = 'z'; // should not generate an IRQ
    if (irq_count != 0) {
        reg_mprj_datal = TEST_RESULT_FAIL_IRQ << 28;
        return;
    }

    reg_spell_exec = 0xff; // should generate an IRQ
    reg_spell_exec = 0xff; // should generate another IRQ
    asm("nop"); // Consumes the IRQ
    if (irq_count != 2) {
        reg_mprj_datal = TEST_RESULT_FAIL_IRQ << 28;
        return;
    }

    // Reset SPELL
    reg_la1_data |= 1;
    reg_la1_data &= ~1;

    // Write a program to the OpenSRAM
    OPENRAM(0) = PACK_SPELL(
        90,
        14,
        '-',
        0xa6
    );
    OPENRAM(4) = PACK_SPELL(
        11,
        'w',
        0x55,
        SPELL_REG_DDR
    );
    OPENRAM(8) = PACK_SPELL(
        'w',
        0x0f,
        SPELL_REG_PORT,
        'w'
    );
    OPENRAM(12) = PACK_SPELL(
        'z',
        0xff,
        0xff,
        0xff
    );

    // Initialize part of the DATA memory
    OPENRAM(DATA_START + 8) = 0; 

    // Let SPELL have R/W access to the OpenSRAM
    reg_la0_data = (1 << SRAM_WRITE_PORT) | (1 << PROJECT_ID);

    // Start SPELL
    reg_spell_ctrl = CTRL_RUN | CTRL_SRAM_ENABLE;

    // Wait for the SPELL to finish
    while (reg_spell_ctrl & CTRL_RUN);

    // At this point, we should have (90-14) at the top of the stack.
    if (reg_spell_stack_top != 90-14) {
        reg_mprj_datal = TEST_RESULT_FAIL_SRAM1 << 28;
        return;
    }

    // Also, project should have written at 0xa6 at data memory address 11 (=8+3)
    if (OPENRAM(DATA_START + 8) != 0xa6000000) {
        reg_mprj_datal = TEST_RESULT_FAIL_SRAM2 << 28;
        return;
    }

    reg_mprj_datal = TEST_RESULT_PASS << 28;
}
