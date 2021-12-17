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

#define CTRL_RUN (1 << 0)
#define CTRL_STEP (1 << 1)
#define CTRL_SRAM_ENABLE (1 << 2)

#define SRAM_WRITE_PORT         31
#define SRAM_BASE_ADDR          0x30FFFC00
#define OPENRAM(addr)           (*(uint32_t*)(SRAM_BASE_ADDR + (addr & 0x3fc)))

#define PACK_SPELL(a, b, c, d) ((a) | (b << 8) | (c << 16) | (d << 24))

#define TEST_RESULT_PASS        0x1
#define TEST_RESULT_FAIL_SRAM   0xe
#define TEST_RESULT_FAIL_DFF    0xf

void write_progmem(uint8_t addr, uint8_t opcode) {
    reg_spell_stack_push = opcode;
    reg_spell_stack_push = addr;
    reg_spell_exec = '!';
}

void main() {
	reg_mprj_io_8 =   GPIO_MODE_USER_STD_INPUT_NOPULL;
	reg_mprj_io_9 =   GPIO_MODE_USER_STD_OUTPUT;

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

    // Reset SPELL
    reg_la1_data |= 1;
    reg_la1_data &= ~1;

    // Write a program to the OpenSRAM
    OPENRAM(0) = PACK_SPELL(
        90,
        14,
        '-',
        'z'
    );

    // Note: uncommenting the following line causes test to fail
    // reg_la0_data = (1 << SRAM_WRITE_PORT) | (1 << PROJECT_ID);

    // Start SPELL
    reg_spell_ctrl = CTRL_RUN | CTRL_SRAM_ENABLE;

    // Wait for the SPELL to finish
    while (reg_spell_ctrl & CTRL_RUN);

    // At this point, we should have (90-14) at the top of the stack.
    if (reg_spell_stack_top != 90-14) {
        reg_mprj_datal = TEST_RESULT_FAIL_SRAM << 28;
        return;
    }
    
    reg_mprj_datal = TEST_RESULT_PASS << 28;
}

