#include "wrapper.h"

#include "Moira.h"

#include <cassert>
#include <cstdlib>
#include <exception>
#include <memory>

namespace {

class CPU_Handler : public moira::Moira {
  private:
	cpu_read8_t m_read8 = nullptr;
	cpu_read16_t m_read16 = nullptr;
	cpu_write8_t m_write8 = nullptr;
	cpu_write16_t m_write16 = nullptr;
	void *m_user;

  protected:
	u8 read8(u32 addr) const override {
		if (m_read8)
			return m_read8(m_user, addr);

		return 0;
	}

	u16 read16(u32 addr) const override {
		if (m_read16)
			return m_read16(m_user, addr);

		return 0;
	}

	void write8(u32 addr, u8 val) const override {
		if (m_write8)
			m_write8(m_user, addr, val);
	}

	void write16(u32 addr, u16 val) const override {
		if (m_write16)
			m_write16(m_user, addr, val);
	}

  public:
	CPU_Handler(cpu_read8_t r8, cpu_read16_t r16, cpu_write8_t w8, cpu_write16_t w16)
		: m_read8 { r8 }, m_read16 { r16 }, m_write8 { w8 }, m_write16 { w16 } { };

	void set_userdata(void *user) {
		m_user = user;
	}
};

} // namespace

struct cpu_t {
	std::unique_ptr<CPU_Handler> handler;
};

cpu_t *cpu_create(cpu_read8_t r8, cpu_read16_t r16, cpu_write8_t w8, cpu_write16_t w16) {
	cpu_t *cpu = nullptr;

	try {
		cpu = new cpu_t();
		cpu->handler = std::make_unique<CPU_Handler>(r8, r16, w8, w16);
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

	cpu->handler->set_userdata(userdata);
}
