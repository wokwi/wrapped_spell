import cocotb
from cocotb.clock import Clock
from cocotb.triggers import RisingEdge, ClockCycles, with_timeout

TEST_RESULT_PASS = 0x1 # Should be in sync with spell_test.c 

@cocotb.test()
async def test_integration(dut):
    clock = Clock(dut.clk, 25, units="ns") # 40M
    cocotb.fork(clock.start())
    
    dut.RSTB <= 0
    dut.power1 <= 0
    dut.power2 <= 0
    dut.power3 <= 0
    dut.power4 <= 0

    await ClockCycles(dut.clk, 8)
    dut.power1 <= 1
    await ClockCycles(dut.clk, 8)
    dut.power2 <= 1
    await ClockCycles(dut.clk, 8)
    dut.power3 <= 1
    await ClockCycles(dut.clk, 8)
    dut.power4 <= 1

    await ClockCycles(dut.clk, 80)
    dut.RSTB <= 1

    # Wait with a timeout for the project to become active
    await with_timeout(RisingEdge(dut.uut.mprj.wrapped_spell_1.active), 180, 'us')

    # Wait
    await ClockCycles(dut.clk, 9200)

    # Check if C program ran succesfully
    assert(((int(dut.uut.soc.mgmt_out_predata) >> 28) & 0xf) == TEST_RESULT_PASS)
