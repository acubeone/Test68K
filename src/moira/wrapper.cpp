#include "wrapper.h"

#include "Moira.h"
#include "MoiraTypes.h"

#include <cassert>
#include <cstdlib>
#include <exception>
#include <memory>

struct cpu_t {
	cpu_read8_t read8 = nullptr;
	cpu_read16_t read16 = nullptr;
	cpu_write8_t write8 = nullptr;
	cpu_write16_t write16 = nullptr;

	void *user;
	moira::Model model;
	u16 exception_vector;
	u16 exception_kind;

	std::unique_ptr<class CPU_Handler> handler;
};

class CPU_Handler : public moira::Moira {
  private:
	struct cpu_t *m_owner;

  protected:
	u8 read8(u32 addr) const override {
		if (m_owner->read8)
			return m_owner->read8(m_owner->user, addr);

		return 0;
	}

	u16 read16(u32 addr) const override {
		if (m_owner->read16)
			return m_owner->read16(m_owner->user, addr);

		return 0;
	}

	void write8(u32 addr, u8 val) const override {
		if (m_owner->write8)
			m_owner->write8(m_owner->user, addr, val);
	}

	void write16(u32 addr, u16 val) const override {
		if (m_owner->write16)
			m_owner->write16(m_owner->user, addr, val);
	}

	void willExecute(moira::M68kException exc, u16 vector) override {
		m_owner->exception_vector = vector;

		using EXC = moira::M68kException;
		switch (exc) {
		case EXC::RESET:			 m_owner->exception_kind = CPU_EXC_OK; break; // Ignore reset
		case EXC::BUS_ERROR:		 m_owner->exception_kind = CPU_EXC_BUS_ERR; break;
		case EXC::ADDRESS_ERROR:	 m_owner->exception_kind = CPU_EXC_ADDR_ERR; break;
		case EXC::ILLEGAL:			 m_owner->exception_kind = CPU_EXC_ILLEGAL; break;
		case EXC::DIVIDE_BY_ZERO:	 m_owner->exception_kind = CPU_EXC_DIVBYZERO; break;
		case EXC::CHK:				 m_owner->exception_kind = CPU_EXC_CHK; break;
		case EXC::TRAPV:			 m_owner->exception_kind = CPU_EXC_TRAPV; break;
		case EXC::PRIVILEGE:		 m_owner->exception_kind = CPU_EXC_PRIVILEGE; break;
		case EXC::TRACE:			 m_owner->exception_kind = CPU_EXC_TRACE; break;
		case EXC::LINEA:			 m_owner->exception_kind = CPU_EXC_LINEA; break;
		case EXC::LINEF:			 m_owner->exception_kind = CPU_EXC_LINEF; break;
		case EXC::FORMAT_ERROR:		 m_owner->exception_kind = CPU_EXC_FORMAT_ERR; break;
		case EXC::IRQ_UNINITIALIZED: m_owner->exception_kind = CPU_EXC_IRQ_UNINIT; break;
		case EXC::IRQ_SPURIOUS:		 m_owner->exception_kind = CPU_EXC_IRQ_SPURIOUS; break;
		case EXC::TRAP:				 m_owner->exception_kind = CPU_EXC_TRAP; break;
		case EXC::BKPT:				 m_owner->exception_kind = CPU_EXC_BKPT; break;
		default:					 m_owner->exception_kind = CPU_EXC_OK; break;
		}
	}

  public:
	CPU_Handler(cpu_t *owner)
		: m_owner { owner } { };
};

cpu_t *cpu_create(cpu_read8_t r8, cpu_read16_t r16, cpu_write8_t w8, cpu_write16_t w16) {
	cpu_t *cpu = nullptr;

	try {
		cpu = new cpu_t();
		cpu->read8 = r8;
		cpu->read16 = r16;
		cpu->write8 = w8;
		cpu->write16 = w16;

		cpu->user = nullptr;
		cpu->exception_vector = -1;
		cpu->exception_kind = CPU_EXC_OK;

		cpu->handler = std::make_unique<CPU_Handler>(cpu);
	} catch (std::exception& e) {
		return nullptr;
	}

	return cpu;
}

void cpu_destroy(cpu_t *cpu) {
	if (!cpu)
		return;

	delete cpu;
}

void cpu_set_userdata(cpu_t *cpu, void *userdata) {
	if (!cpu)
		return;

	cpu->user = userdata;
}

void cpu_reset(cpu_t *cpu, enum cpu_model_t cpu_model) {
	if (!cpu)
		return;

	if (cpu_model == CPU_MODEL_M68000)
		cpu->model = moira::Model::M68000;
	else
		cpu->model = moira::Model::M68010;

	cpu->handler->setModel(cpu->model);

	cpu->exception_vector = -1;
	(*cpu->handler).reset();
}

void cpu_execute(cpu_t *cpu) {
	if (!cpu)
		return;

	cpu->handler->execute();
}

bool cpu_is_instruction_valid(const cpu_t *cpu, u16 opcode, u16 ext) {
	if (!cpu)
		return false;

	moira::InstrInfo instr = cpu->handler->getInstrInfo(opcode);
	return cpu->handler->isAvailable(cpu->model, instr.I, instr.M, instr.S, ext);
}

i32 cpu_disassemble(const cpu_t *cpu, char *str, u32 addr) {
	if (!cpu || !str)
		return 0;

	return cpu->handler->disassemble(str, addr);
}

void cpu_get_registers(const cpu_t *cpu, u32 *out) {
	assert(out);

	if (!cpu)
		return;

	for (i32 i = 0; i <= 7; i += 1) {
		out[i + CPU_REG_D0] = cpu->handler->getD(i);
		out[i + CPU_REG_A0] = cpu->handler->getA(i);
	}

	out[CPU_REG_PC] = cpu->handler->getPC();
	out[CPU_REG_SR] = (u32)cpu->handler->getSR();
	out[CPU_REG_SP] = cpu->handler->getSP();
	out[CPU_REG_USP] = cpu->handler->getUSP();
	out[CPU_REG_ISP] = cpu->handler->getISP();
	out[CPU_REG_VBR] = cpu->handler->getVBR();
	out[CPU_REG_SFC] = cpu->handler->getSFC();
	out[CPU_REG_DFC] = cpu->handler->getDFC();
}

void cpu_set_registers(cpu_t *cpu, const u32 *regs) {
	assert(regs);

	if (!cpu)
		return;

	for (i32 i = 0; i <= 7; i += 1) {
		cpu->handler->setD(i, regs[i + CPU_REG_D0]);
		cpu->handler->setA(i, regs[i + CPU_REG_A0]);
	}

	// cpu->handler->setPC(regs[CPU_REG_PC]);
	// cpu->handler->setSR(regs[CPU_REG_SR]);
	// cpu->handler->setSP(regs[CPU_REG_SP]);
	// cpu->handler->setUSP(regs[CPU_REG_USP]);
	// cpu->handler->setISP(regs[CPU_REG_ISP]);
	// cpu->handler->setVBR(regs[CPU_REG_VBR]);
	// cpu->handler->setSFC(regs[CPU_REG_SFC]);
	// cpu->handler->setDFC(regs[CPU_REG_DFC]);
}

i64 cpu_get_clock(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->handler->getClock();
}

void cpu_set_clock(cpu_t *cpu, i64 cycles) {
	if (!cpu)
		return;

	cpu->handler->setClock(cycles);
}

u32 cpu_get_pc0(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->handler->getPC0();
}

u8 cpu_read_fc(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->handler->readFC();
}

u8 cpu_get_ipl(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->handler->getIPL();
}

u16 cpu_get_exception_vector(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->exception_vector;
}

u16 cpu_get_exception_kind(const cpu_t *cpu) {
	if (!cpu)
		return 0;

	return cpu->exception_kind;
}
