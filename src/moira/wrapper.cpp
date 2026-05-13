#include "wrapper.h"

#include "Moira.h"

#include <cassert>
#include <cstdlib>
#include <memory>

namespace {

class CPU : public moira::Moira {
  private:
	cpu_read8 m_read8 = nullptr;
	cpu_read16 m_read16 = nullptr;
	cpu_write8 m_write8 = nullptr;
	cpu_write16 m_write16 = nullptr;

  protected:
	u8 read8(u32 addr) const override {
		if (m_read8)
			return m_read8(addr);

		return 0;
	}

	u16 read16(u32 addr) const override {
		if (m_read16)
			return m_read16(addr);

		return 0;
	}

	void write8(u32 addr, u8 val) const override {
		if (m_write8)
			m_write8(addr, val);
	}

	void write16(u32 addr, u16 val) const override {
		if (m_write16)
			m_write16(addr, val);
	}

  public:
	CPU() = default;

	friend void ::cpu_set_read8_callback(cpu_read8 func);
	friend void ::cpu_set_read16_callback(cpu_read16 func);
	friend void ::cpu_set_write8_callback(cpu_write8 func);
	friend void ::cpu_set_write16_callback(cpu_write16 func);
};

} // namespace

static std::unique_ptr<CPU> _cpu = nullptr;

void cpu_init() {
	if (_cpu)
		return;

	_cpu = std::make_unique<CPU>();
	if (!_cpu)
		exit(EXIT_FAILURE);
}

void cpu_deinit() {
	if (_cpu)
		_cpu = nullptr;
}

void cpu_set_read8_callback(cpu_read8 func) {
	if (!_cpu)
		return;
	_cpu->m_read8 = func;
}

void cpu_set_read16_callback(cpu_read16 func) {
	if (!_cpu)
		return;
	_cpu->m_read16 = func;
}

void cpu_set_write8_callback(cpu_write8 func) {
	if (!_cpu)
		return;
	_cpu->m_write8 = func;
}

void cpu_set_write16_callback(cpu_write16 func) {
	if (!_cpu)
		return;
	_cpu->m_write16 = func;
}
