// SPDX-FileCopyrightText: © 2021 Uri Shaked <uri@wokwi.com>
// SPDX-License-Identifier: MIT

#define PROJECT_ID              1

#define reg_spell_pc            (*(volatile uint32_t*)0x30000000)
#define reg_spell_sp            (*(volatile uint32_t*)0x30000004)
#define reg_spell_exec          (*(volatile uint32_t*)0x30000008)
#define reg_spell_run           (*(volatile uint32_t*)0x3000000C)
#define reg_spell_cycles_per_ms (*(volatile uint32_t*)0x30000010)
#define reg_spell_stack_top     (*(volatile uint32_t*)0x30000014)
#define reg_spell_stack_push    (*(volatile uint32_t*)0x30000018)

#define TEST_RESULT_PASS        0x1
#define TEST_RESULT_FAIL        0xf

#include "verilog/dv/caravel/defs.h"

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
    reg_spell_run = 1;

    // Wait for the SPELL to finish
    while (reg_spell_run & 0x1);

    // At this point, we should have 55 at the top of the stack.
    if (reg_spell_stack_top == 55) {
        reg_mprj_datal = TEST_RESULT_PASS << 28;
    } else {
        reg_mprj_datal = TEST_RESULT_FAIL << 28;
    }
}

