#include <stddef.h>

#include "common/assert.h"
#include "hd44780.hpp"

#include "third_party/FreeRTOS/include/FreeRTOS.h"
#include "third_party/FreeRTOS/include/task.h"

using namespace drv;
using namespace hal;

#define DDRAM1_MIN_ADDR 0
#define DDRAM1_MAX_ADDR 39
#define DDRAM2_MIN_ADDR 64
#define DDRAM2_MAX_ADDR 103

enum cmd_t
{
	CLEAR_DISPLAY                  = 1 << 0,
	RETURN_HOME                    = 1 << 1,
	ENTRY_MODE_SET                 = 1 << 2,
	DISPLAY_ON_OFF_CONTROL         = 1 << 3,
	CURSOR_OR_DISPLAY_SHIFT        = 1 << 4,
	FUNCTION_SET                   = 1 << 5,
	SET_CGRAM_ADDRESS              = 1 << 6,
	SET_DDRAM_ADDRESS              = 1 << 7
	
	/* Read only command or command which doesn't require data lines
	READ_BUSY_FLAG_AND_ADDRESS (RW = 1)
	WRITE_DATA_TO_ADDRESS      (RW = 0)
	READ_DATA_FROM_RAM         (RW = 1, RS = 1)
	*/
};

// Bits for ENTRY_MODE_SET command
enum entry_mode_set_bits_t
{
	I_D_BIT = 1 << 1, // Increment/Decrement DDRAM address (cursor position):
	                  // 0 - decrement, 1 - increment
	S_BIT   = 1 << 0  // Shift the dispaly with each new character
};

// Bits for DISPLAY_ON_OFF_CONTROL command
enum display_on_off_control_bits_t
{
	D_BIT = 1 << 2, // On/off entire display
	C_BIT = 1 << 1, // On/off cursor
	B_BIT = 1 << 0  // On/off blinking cursor position
};

// Bits for CURSOR_OR_DISPLAY_SHIFT command
enum cursor_or_display_shift_bits_t
{
	S_C_BIT = 1 << 3, // Shift display or cursor: 0 - cursor, 1 - display
	R_L_BIT = 1 << 2  // Direction of shift: 0 - to the left, 1 - to the right
};

// Bits for FUNCTION_SET command
enum function_set_bits_t
{
	DL_BIT  = 1 << 4, // Interface data length: 0 - 4 bit, 1 - 8 bit
	N_BIT   = 1 << 3, // Number of display lines: 0 - one line, 1 - two line
	F_BIT   = 1 << 2, // Character font: 0 - 5x8, 1 - 5x10
	FT1_BIT = 1 << 1, // Font table: (FT1:FT0)
	FT0_BIT = 1 << 0, //            00 - ENGLISH_JAPANESE
	                  //            01 - WESTERN EUROPEAN
	                  //            10 - ENGLISH_RUSSIAN
	                  //            11 - N/A
};

hd44780::hd44780(gpio &rs, gpio &rw, gpio &e, gpio &db4, gpio &db5, gpio &db6,
	gpio &db7, tim &tim):
	_rs(rs),
	_rw(rw),
	_e(e),
	_db{&db4, &db5, &db6, &db7},
	_tim(tim),
	_ddram_addr(0)
{
	ASSERT(_e.mode() == gpio::mode::DO);
	ASSERT(_rw.mode() == gpio::mode::DO);
	ASSERT(_rs.mode() == gpio::mode::DO);
	
	for(uint8_t i = 4; (i < sizeof(_db)/sizeof(_db[0])) && _db[i]; i++)
		ASSERT(_db[i]->mode() == gpio::mode::DO);
	
	_lock = xSemaphoreCreateBinary();
	ASSERT(_lock);
}

hd44780::~hd44780()
{
	vSemaphoreDelete(_lock);
}

void hd44780::init()
{
	_rw.set(0);
	_rs.set(0);
	
	write_4bit(FUNCTION_SET >> 4);
	delay(4100);
	write_4bit(FUNCTION_SET >> 4);
	delay(100);
	write_4bit(FUNCTION_SET >> 4);
	
	write_4bit((FUNCTION_SET | N_BIT) >> 4);
	write(CMD, FUNCTION_SET | N_BIT);
	
	write(CMD, DISPLAY_ON_OFF_CONTROL | D_BIT);
	
	write(CMD, CLEAR_DISPLAY);
	delay(6200); // OLED display requires 6,2 ms rather than 1,53 ms
	
	write(CMD, ENTRY_MODE_SET | I_D_BIT);
}

void hd44780::print(const char *str)
{
	while(*str != '\0')
	{
		write(DATA, *str);
		str++;
	}
}

void hd44780::print(char byte)
{
	write(DATA, byte);
}

void hd44780::ddram_addr(uint8_t addr)
{
	ASSERT((addr >= DDRAM1_MIN_ADDR && addr <= DDRAM1_MAX_ADDR) ||
		(addr >= DDRAM2_MIN_ADDR && addr <= DDRAM2_MAX_ADDR));
	
	_ddram_addr = addr;
	
	write(CMD, SET_DDRAM_ADDRESS | _ddram_addr);
}

void hd44780::clear()
{
	write(CMD, CLEAR_DISPLAY);
	delay(6200); // OLED display requires 6,2 ms rather than 1,53 ms
}

uint8_t hd44780::read_bf_and_ddram_addr()
{
	_rw.set(1);
	_rs.set(0);
	
	for(uint8_t i = 0; i < (sizeof(_db) / sizeof(_db[0])); i++)
		_db[i]->mode(gpio::mode::DI);
	
	uint8_t byte = read_4bit() << 4;
	byte |= read_4bit();
	
	for(uint8_t i = 0; i < (sizeof(_db) / sizeof(_db[0])); i++)
		_db[i]->mode(gpio::mode::DO);
	
	return byte;
}

void hd44780::write_4bit(uint8_t half_byte)
{
	for(uint8_t i = 0; i < 4; i++)
		_db[i]->set((half_byte >> i) & 1);
	
	_e.set(1);
	delay(40);
	_e.set(0);
	delay(40);
}

void hd44780::write(write_t type, uint8_t byte)
{
	_rw.set(0);
	_rs.set(type == CMD ? 0 : 1);
	
	write_4bit(byte >> 4);
	write_4bit(byte);
}

uint8_t hd44780::read_4bit()
{
	uint8_t half_byte = 0;
	
	_e.set(1);
	delay(40);
	
	for(uint8_t i = 0; i < 4; i++)
	{
		if(_db[i]->get())
			half_byte |= 1 << i;
	}
	
	_e.set(0);
	delay(40);
	
	return half_byte;
}

static void tim_cb(tim *tim, void *ctx)
{
	SemaphoreHandle_t _lock = (SemaphoreHandle_t)ctx;
	
	BaseType_t hi_task_woken = 0;
	xSemaphoreGiveFromISR(_lock, &hi_task_woken);
	portYIELD_FROM_ISR(hi_task_woken);
}

void hd44780::delay(uint32_t us)
{
	_tim.us(us);
	_tim.start_once(tim_cb, _lock);
	
	xSemaphoreTake(_lock, portMAX_DELAY);
}