/*
 * SPDX-FileCopyrightText: 2020 Efabless Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * SPDX-License-Identifier: Apache-2.0
 */

/*-----------------------------------------------*/
/* Start code that enables and handles IRQs 	 */
/*-----------------------------------------------*/
/* Code modified specifically for this testbench */
/*-----------------------------------------------*/

#undef ENABLE_FASTIRQ

#include "custom_ops.S"

.section .text
.global irq

reset_vec:
	j start

.balign 16
# this interrupt handler adapted from Claire's here: 
# https://raw.githubusercontent.com/cliffordwolf/picorv32/master/firmware/start.S
irq_vec:
    /* save registers */

    sw gp,   0*4+0x200(zero)
    sw x1,   1*4+0x200(zero)
    sw x2,   2*4+0x200(zero)
    sw x5,   5*4+0x200(zero)
    sw x6,   6*4+0x200(zero)
    sw x7,   7*4+0x200(zero)
    sw x10, 10*4+0x200(zero)
    sw x11, 11*4+0x200(zero)
    sw x12, 12*4+0x200(zero)
    sw x13, 13*4+0x200(zero)
    sw x14, 14*4+0x200(zero)
    sw x15, 15*4+0x200(zero)
    sw x16, 16*4+0x200(zero)
    sw x17, 17*4+0x200(zero)
    sw x28, 28*4+0x200(zero)
    sw x29, 29*4+0x200(zero)
    sw x30, 30*4+0x200(zero)
    sw x31, 31*4+0x200(zero)

    // call to C function
    jal ra, irq

    /* restore registers */
    lw gp,   0*4+0x200(zero)
    lw x1,   1*4+0x200(zero)
    lw x2,   2*4+0x200(zero)
    lw x5,   5*4+0x200(zero)
    lw x6,   6*4+0x200(zero)
    lw x7,   7*4+0x200(zero)
    lw x10, 10*4+0x200(zero)
    lw x11, 11*4+0x200(zero)
    lw x12, 12*4+0x200(zero)
    lw x13, 13*4+0x200(zero)
    lw x14, 14*4+0x200(zero)
    lw x15, 15*4+0x200(zero)
    lw x16, 16*4+0x200(zero)
    lw x17, 17*4+0x200(zero)
    lw x28, 28*4+0x200(zero)
    lw x29, 29*4+0x200(zero)
    lw x30, 30*4+0x200(zero)
    lw x31, 31*4+0x200(zero)
    
    // retire interrupt
    picorv32_retirq_insn()

.balign 0x200
irq_regs:
    // registers are saved to this memory region during interrupt handling
    // the program counter is saved as register 0
    .fill 32,4


start:

# zero-initialize register file
addi x1, zero, 0
# x2 (sp) is initialized by reset
addi x3, zero, 0
addi x4, zero, 0
addi x5, zero, 0
addi x6, zero, 0
addi x7, zero, 0
addi x8, zero, 0
addi x9, zero, 0
addi x10, zero, 0
addi x11, zero, 0
addi x12, zero, 0
addi x13, zero, 0
addi x14, zero, 0
addi x15, zero, 0
addi x16, zero, 0
addi x17, zero, 0
addi x18, zero, 0
addi x19, zero, 0
addi x20, zero, 0
addi x21, zero, 0
addi x22, zero, 0
addi x23, zero, 0
addi x24, zero, 0
addi x25, zero, 0
addi x26, zero, 0
addi x27, zero, 0
addi x28, zero, 0
addi x29, zero, 0
addi x30, zero, 0
addi x31, zero, 0

# zero initialize scratchpad memory
# setmemloop:
# sw zero, 0(x1)
# addi x1, x1, 4
# blt x1, sp, setmemloop

# This part copied from Tim's demo. 
# Write these instructions to memory location zero and following:
# lui t4, 0x10000	= 10000eb7
# addi t4, t4, 4	= 0e91
# jalr t4, 0		= 000e80e7
#
# These three instructions jump to 0x10000004, which is the location
# of the interrupt handler.  For a fast interrupt handler, the whole
# handler should be moved into SRAM.
li  t4, 0x10000eb7
sw  t4, 0(zero)
li  t4, 0x80e70e91
sw  t4, 4(zero)
li  t4, 0x000e
sw  t4, 8(zero)

# Enable the user interrupts only on irq[14:12]
# needs to be 0 to allow the interrupt
# https://github.com/efabless/caravel-lite/blob/13f2590e4b3a74b910dac56a6b757f5a66fd5212/verilog/rtl/mgmt_soc.v#L360
li   t4, 0x8FFF 
picorv32_maskirq_insn(t4, t4)

# call main
call main
loop:
j loop

.global flashio_worker_begin
.global flashio_worker_end

flashio_worker_begin:
# a0 ... data pointer
# a1 ... data length
# a2 ... optional WREN cmd (0 = disable)

# address of SPI ctrl reg
li   t0, 0x02000000

# Set CS high, IO0 is output
li   t1, 0x120
sh   t1, 0(t0)

# Enable Manual SPI Ctrl
sb   zero, 3(t0)

# Send optional WREN cmd
beqz a2, flashio_worker_L1
li   t5, 8
andi t2, a2, 0xff
flashio_worker_L4:
srli t4, t2, 7
sb   t4, 0(t0)
ori  t4, t4, 0x10
sb   t4, 0(t0)
slli t2, t2, 1
andi t2, t2, 0xff
addi t5, t5, -1
bnez t5, flashio_worker_L4
sb   t1, 0(t0)

# SPI transfer
flashio_worker_L1:
beqz a1, flashio_worker_L3
li   t5, 8
lbu  t2, 0(a0)
flashio_worker_L2:
srli t4, t2, 7
sb   t4, 0(t0)
ori  t4, t4, 0x10
sb   t4, 0(t0)
lbu  t4, 0(t0)
andi t4, t4, 2
srli t4, t4, 1
slli t2, t2, 1
or   t2, t2, t4
andi t2, t2, 0xff
addi t5, t5, -1
bnez t5, flashio_worker_L2
sb   t2, 0(a0)
addi a0, a0, 1
addi a1, a1, -1
j    flashio_worker_L1
flashio_worker_L3:

# Back to MEMIO mode
li   t1, 0x80
sb   t1, 3(t0)

ret
flashio_worker_end:

