#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common/assert.h"
#include "spi.hpp"
#include "rcc/rcc.hpp"
#include "dma/dma.hpp"
#include "gpio/gpio.hpp"
#include "CMSIS/device-support/include/stm32f4xx.h"
#include "CMSIS/core-support/core_cm4.h"
#include "FreeRTOS.h"
#include "semphr.h"

using namespace hal;

#define IRQ_PRIORITY 3

static SPI_TypeDef *const spi_list[spi::SPI_END] =
{
	SPI1,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F410Cx) || defined(STM32F410Rx) || \
	defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
	defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
	defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	SPI2,
#else
	NULL,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	SPI3,
#else
	NULL,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	SPI4,
#else
	NULL,
#endif
#if defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	SPI5,
#else
	NULL,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	SPI6
#else
	NULL
#endif
};

static IRQn_Type const irq_list[spi::SPI_END] =
{
	SPI1_IRQn,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F410Cx) || defined(STM32F410Rx) || \
	defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
	defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
	defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	SPI2_IRQn,
#else
	static_cast<IRQn_Type>(0),
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	SPI3_IRQn,
#else
	static_cast<IRQn_Type>(0),
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	SPI4_IRQn,
#else
	static_cast<IRQn_Type>(0),
#endif
#if defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	SPI5_IRQn,
#else
	static_cast<IRQn_Type>(0),
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	SPI6_IRQn
#else
	static_cast<IRQn_Type>(0)
#endif
};

static uint32_t const rcc_list[spi::SPI_END] =
{
	RCC_APB2ENR_SPI1EN,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F410Cx) || defined(STM32F410Rx) || \
	defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
	defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
	defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	RCC_APB1ENR_SPI2EN,
#else
	0,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB1ENR_SPI3EN,
#else
	0,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	RCC_APB2ENR_SPI4EN,
#else
	0,
#endif
#if defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB2ENR_SPI5EN,
#else
	0,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB2ENR_SPI6EN
#else
	0
#endif
};

static uint32_t const reset_list[spi::SPI_END] =
{
	RCC_APB2RSTR_SPI1RST,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F410Cx) || defined(STM32F410Rx) || \
	defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
	defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
	defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	RCC_APB1RSTR_SPI2RST,
#else
	0,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB1RSTR_SPI3RST,
#else
	0,
#endif
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	RCC_APB2RSTR_SPI4RST,
#else
	0,
#endif
#if defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB2RSTR_SPI5RST,
#else
	0,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	RCC_APB2RSTR_SPI6RST
#else
	0
#endif
};

static volatile uint32_t *rcc_addr_list[spi::SPI_END] =
{
	&RCC->APB2ENR,
	&RCC->APB1ENR,
	&RCC->APB1ENR,
	&RCC->APB2ENR,
	&RCC->APB2ENR,
	&RCC->APB2ENR
};

static volatile uint32_t *reset_addr_list[spi::SPI_END] =
{
	&RCC->APB2RSTR,
	&RCC->APB1RSTR,
	&RCC->APB1RSTR,
	&RCC->APB2RSTR,
	&RCC->APB2RSTR,
	&RCC->APB2RSTR
};

static rcc_src_t const rcc_src_list[spi::SPI_END] =
{
	RCC_SRC_APB2,
	RCC_SRC_APB1,
	RCC_SRC_APB1,
	RCC_SRC_APB2,
	RCC_SRC_APB2,
	RCC_SRC_APB2
};

static uint8_t const gpio_af_list[spi::SPI_END] =
{
	0x05,
	0x05,
	0x06,
	0x05,
	0x05,
	0x05
};

static GPIO_TypeDef *const gpio_list[PORT_QTY] =
{
	GPIOA, GPIOB, GPIOC,
#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	GPIOD, GPIOE,
#else
	NULL, NULL,
#endif
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
	GPIOF, GPIOG,
#else
	NULL, NULL,
#endif
	GPIOH,
#if defined(STM32F405xx) || defined(STM32F407xx) || defined(STM32F415xx) || \
	defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
	GPIOI,
#else
	NULL,
#endif
#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
	GPIOJ, GPIOK
#else
	NULL, NULL
#endif
};

static spi *obj_list[spi::SPI_END];

static void gpio_af_init(spi::spi_t spi, gpio &gpio);
static uint8_t calc_presc(spi::spi_t spi, uint32_t baud);

#if configUSE_TRACE_FACILITY
static traceHandle isr_dma_tx, isr_dma_rx, isr_spi;
#endif

spi::spi(spi_t spi, uint32_t baud, cpol_t cpol, cpha_t cpha,
	bit_order_t bit_order, dma &dma_tx, dma &dma_rx, gpio &mosi,
	gpio &miso, gpio &clk):
	_spi(spi),
	_baud(baud),
	_cpol(cpol),
	_cpha(cpha),
	_bit_order(bit_order),
	api_lock(NULL),
	irq_lock(NULL),
	irq_res(RES_OK),
	_mosi(mosi),
	_miso(miso),
	_clk(clk),
	_cs(NULL),
	tx_dma(dma_tx),
	tx_buff(NULL),
	rx_dma(dma_rx),
	rx_buff(NULL),
	rx_irq_res(RES_OK)
{
	ASSERT(_spi < SPI_END && spi_list[_spi]);
	ASSERT(_baud > 0);
	ASSERT(_cpol <= CPOL_1);
	ASSERT(_cpha <= CPHA_1);
	ASSERT(tx_dma.dir() == dma::DIR_MEM_TO_PERIPH);
	ASSERT(tx_dma.inc_size() == dma::INC_SIZE_8);
	ASSERT(rx_dma.dir() == dma::DIR_PERIPH_TO_MEM);
	ASSERT(rx_dma.inc_size() == dma::INC_SIZE_8);
	ASSERT(_mosi.mode() == gpio::MODE_AF);
	ASSERT(_miso.mode() == gpio::MODE_AF);
	ASSERT(_clk.mode() == gpio::MODE_AF);
	
	api_lock = xSemaphoreCreateMutex();
	ASSERT(api_lock);
	irq_lock = xSemaphoreCreateBinary();
	ASSERT(irq_lock);
	
#if configUSE_TRACE_FACILITY
	vTraceSetMutexName((void *)api_lock, "spi_api_lock");
	vTraceSetSemaphoreName((void *)irq_lock, "spi_irq_lock");
	isr_dma_tx = xTraceSetISRProperties("ISR_dma_spi_tx", 1);
	isr_dma_rx = xTraceSetISRProperties("ISR_dma_spi_rx", 1);
	isr_spi = xTraceSetISRProperties("ISR_spi", 1);
#endif
	
	obj_list[_spi] = this;
	
	*rcc_addr_list[_spi] |= rcc_list[_spi];
	*reset_addr_list[_spi] |= reset_list[_spi];
	*reset_addr_list[_spi] &= ~reset_list[_spi];
	
	gpio_af_init(_spi, _mosi);
	gpio_af_init(_spi, _miso);
	gpio_af_init(_spi, _clk);
	
	SPI_TypeDef *spi_base = spi_list[_spi];
	
	// Master mode
	spi_base->CR1 |= SPI_CR1_MSTR;
	
	if(_cpol == CPOL_0)
		spi_base->CR1 &= ~SPI_CR1_CPOL;
	else
		spi_base->CR1 |= SPI_CR1_CPOL;
	
	if(_cpha == CPHA_0)
		spi_base->CR1 &= ~SPI_CR1_CPHA;
	else
		spi_base->CR1 |= SPI_CR1_CPHA;
	
	if(_bit_order == BIT_ORDER_MSB)
		spi_base->CR1 &= ~SPI_CR1_LSBFIRST;
	else
		spi_base->CR1 |= SPI_CR1_LSBFIRST;
	
	// Disable NSS hardware management
	spi_base->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
	
	uint8_t presc = calc_presc(_spi, _baud);
	spi_base->CR1 |= ((presc << SPI_CR1_BR_Pos) & SPI_CR1_BR);
	
	spi_base->CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	
	// TODO: overrun error has happend each time with this bit
	//spi_base->CR2 |= SPI_CR2_ERRIE;
	
	spi_base->CR1 |= SPI_CR1_SPE;
	
	tx_dma.dst((uint8_t *)&spi_base->DR);
	rx_dma.src((uint8_t *)&spi_base->DR);
	
	NVIC_ClearPendingIRQ(irq_list[_spi]);
	NVIC_SetPriority(irq_list[_spi], IRQ_PRIORITY);
	NVIC_EnableIRQ(irq_list[_spi]);
}

spi::~spi()
{
	*reset_addr_list[_spi] |= reset_list[_spi];
	*reset_addr_list[_spi] &= ~reset_list[_spi];
	*rcc_addr_list[_spi] &= ~rcc_list[_spi];
}

void spi::baud(uint32_t baud)
{
	ASSERT(baud > 0);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	uint8_t presc = calc_presc(_spi, _baud);
	
	_baud = baud;
	SPI_TypeDef *spi = spi_list[_spi];
	
	spi->CR1 &= ~(SPI_CR1_SPE | SPI_CR1_BR);
	spi->CR1 |= ((presc << SPI_CR1_BR_Pos) & SPI_CR1_BR) | SPI_CR1_SPE;
	
	xSemaphoreGive(api_lock);
}

void spi::cpol(cpol_t cpol)
{
	ASSERT(cpol <= CPOL_1);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cpol = cpol;
	SPI_TypeDef *spi = spi_list[_spi];
	
	spi->CR1 &= ~SPI_CR1_SPE;
	
	if(_cpol == CPOL_0)
		spi->CR1 &= ~SPI_CR1_CPOL;
	else
		spi->CR1 |= SPI_CR1_CPOL;
	
	spi->CR1 |= SPI_CR1_SPE;
	
	xSemaphoreGive(api_lock);
}

void spi::cpha(cpha_t cpha)
{
	ASSERT(cpha <= CPHA_1);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cpha = cpha;
	SPI_TypeDef *spi = spi_list[_spi];
	
	spi->CR1 &= ~SPI_CR1_SPE;
	
	if(_cpha == CPHA_0)
		spi->CR1 &= ~SPI_CR1_CPHA;
	else
		spi->CR1 |= SPI_CR1_CPHA;
	
	spi->CR1 |= SPI_CR1_SPE;
	
	xSemaphoreGive(api_lock);
}

void spi::bit_order(bit_order_t bit_order)
{
	ASSERT(bit_order <= BIT_ORDER_LSB);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_bit_order = bit_order;
	SPI_TypeDef *spi = spi_list[_spi];
	
	spi->CR1 &= ~SPI_CR1_SPE;
	
	if(_bit_order == BIT_ORDER_MSB)
		spi->CR1 &= ~SPI_CR1_LSBFIRST;
	else
		spi->CR1 |= SPI_CR1_LSBFIRST;
	
	spi->CR1 |= SPI_CR1_SPE;
	
	xSemaphoreGive(api_lock);
}

int8_t spi::write(void *buff, uint16_t size, gpio *cs)
{
	ASSERT(buff);
	ASSERT(size > 0);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cs = cs;
	if(_cs)
		_cs->set(0);
	
	tx_buff = buff;
	tx_dma.src((uint8_t*)tx_buff);
	tx_dma.size(size);
	tx_dma.start_once(on_dma_tx, this);
	spi_list[_spi]->CR2 |= SPI_CR2_TXDMAEN;
	
	/* Task will be blocked during spi tx operation */
	/* irq_lock will be given later from irq handler */
	xSemaphoreTake(irq_lock, portMAX_DELAY);
	
	xSemaphoreGive(api_lock);
	
	return irq_res;
}

int8_t spi::write(uint8_t byte, gpio *cs)
{
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cs = cs;
	if(_cs)
		_cs->set(0);
	
	tx_buff = &byte;
	tx_dma.src((uint8_t*)tx_buff);
	tx_dma.size(1);
	tx_dma.start_once(on_dma_tx, this);
	spi_list[_spi]->CR2 |= SPI_CR2_TXDMAEN;
	
	/* Task will be blocked during spi tx operation */
	/* irq_lock will be given later from irq handler */
	xSemaphoreTake(irq_lock, portMAX_DELAY);
	
	xSemaphoreGive(api_lock);
	
	return irq_res;
}

int8_t spi::read(void *buff, uint16_t size, gpio *cs)
{
	ASSERT(buff);
	ASSERT(size > 0);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cs = cs;
	if(_cs)
		_cs->set(0);
	
	rx_buff = buff;
	rx_dma.dst((uint8_t*)rx_buff);
	rx_dma.size(size);
	rx_dma.start_once(on_dma_rx, this);
	
	/* Setup tx for reception */
	tx_buff = rx_buff;
	tx_dma.src((uint8_t*)tx_buff);
	tx_dma.size(size);
	tx_dma.start_once(on_dma_tx, this);
	spi_list[_spi]->DR;
	spi_list[_spi]->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
	
	/* Task will be blocked during spi rx operation */
	/* irq_lock will be given later from irq handler */
	xSemaphoreTake(irq_lock, portMAX_DELAY);
	
	xSemaphoreGive(api_lock);
	
	return irq_res;
}

int8_t spi::exch(void *buff_tx, void *buff_rx, uint16_t size, gpio *cs)
{
	ASSERT(buff_tx);
	ASSERT(buff_rx);
	ASSERT(size > 0);
	
	xSemaphoreTake(api_lock, portMAX_DELAY);
	
	_cs = cs;
	if(_cs)
		_cs->set(0);
	
	rx_buff = buff_rx;
	rx_dma.src((uint8_t*)rx_buff);
	rx_dma.size(size);
	rx_dma.start_once(on_dma_rx, this);
	spi_list[_spi]->DR;
	
	tx_buff = buff_tx;
	tx_dma.src((uint8_t*)tx_buff);
	tx_dma.size(size);
	tx_dma.start_once(on_dma_tx, this);
	
	spi_list[_spi]->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
	
	/* Task will be blocked during spi exch operation */
	/* irq_lock will be given later from irq handler */
	xSemaphoreTake(irq_lock, portMAX_DELAY);
	
	xSemaphoreGive(api_lock);
	
	return irq_res;
}

static void gpio_af_init(spi::spi_t spi, gpio &gpio)
{
	GPIO_TypeDef *gpio_reg = gpio_list[gpio.port()];
	
	uint8_t pin = gpio.pin();
	/* Push-pull type */
	gpio_reg->OTYPER &= ~(GPIO_OTYPER_OT_0 << pin);
	
	/* Configure alternate function */
	if(pin < 8)
	{
		gpio_reg->AFR[0] &= ~(0x0F << (pin * 4));
		gpio_reg->AFR[0] |= gpio_af_list[spi] << (pin * 4);
	}
	else
	{
		gpio_reg->AFR[1] &= ~(0x0F << ((pin - 8) * 4));
		gpio_reg->AFR[1] |= gpio_af_list[spi] << ((pin - 8) * 4);
	}
}

static uint8_t calc_presc(spi::spi_t spi, uint32_t baud)
{
	uint32_t div = rcc_get_freq(rcc_src_list[spi]) / baud;
	
	/* Baud rate is too low or too high */
	ASSERT(div > 1 && div <= 256);
	
	uint8_t res = 0;
	/* Calculate how many times div can be divided by 2 */
	while((div /= 2) > 1)
		res++;
	
	return res;
}

void spi::on_dma_tx(dma *dma, dma::event_t event, void *ctx)
{
	if(event == dma::EVENT_HALF)
		return;
#if configUSE_TRACE_FACILITY
	vTraceStoreISRBegin(isr_dma_tx);
#endif
	spi *obj = static_cast<spi *>(ctx);
	BaseType_t hi_task_woken = 0;
	
	obj->tx_buff = NULL;
	SPI_TypeDef *spi = spi_list[obj->_spi];
	
	spi->CR2 &= ~SPI_CR2_TXDMAEN;
	if(event == dma::EVENT_CMPLT)
	{
		if(obj->rx_buff)
		{
#if configUSE_TRACE_FACILITY
			vTraceStoreISREnd(hi_task_woken);
#endif
			return;
		}
		spi->CR2 |= SPI_CR2_TXEIE;
	}
	else if(event == dma::EVENT_ERROR)
	{
		if(obj->rx_buff)
		{
			spi->CR2 &= ~SPI_CR2_RXDMAEN;
			obj->rx_dma.stop();
			obj->rx_buff = NULL;
		}
		
		if(obj->_cs)
		{
			obj->_cs->set(1);
			obj->_cs = NULL;
		}
		
		obj->irq_res = RES_FAIL;
		xSemaphoreGiveFromISR(obj->irq_lock, &hi_task_woken);
		portYIELD_FROM_ISR(hi_task_woken);
	}
	
#if configUSE_TRACE_FACILITY
	vTraceStoreISREnd(hi_task_woken);
#endif
}

void spi::on_dma_rx(dma *dma, dma::event_t event, void *ctx)
{
	if(event == dma::EVENT_HALF)
		return;
#if configUSE_TRACE_FACILITY
	vTraceStoreISRBegin(isr_dma_rx);
#endif
	spi *obj = static_cast<spi *>(ctx);
	SPI_TypeDef *spi = spi_list[obj->_spi];
	
	obj->rx_buff = NULL;
	spi->CR2 &= ~SPI_CR2_RXDMAEN;
	if(event == dma::EVENT_CMPLT)
		obj->irq_res = RES_OK;
	else if(event == dma::EVENT_ERROR)
		obj->irq_res = RES_FAIL;
	
	if(obj->_cs)
	{
		obj->_cs->set(1);
		obj->_cs = NULL;
	}
	
	BaseType_t hi_task_woken = 0;
	xSemaphoreGiveFromISR(obj->irq_lock, &hi_task_woken);
	portYIELD_FROM_ISR(hi_task_woken);
#if configUSE_TRACE_FACILITY
	vTraceStoreISREnd(hi_task_woken);
#endif
}

extern "C" void spi_irq_hndlr(hal::spi *obj)
{
	SPI_TypeDef *spi = spi_list[obj->_spi];
	uint32_t sr = spi->SR;
	uint32_t dr = spi->DR;
#if configUSE_TRACE_FACILITY
	vTraceStoreISRBegin(isr_spi);
#endif
	
	if((spi->CR2 & SPI_CR2_TXEIE) && (sr & SPI_SR_TXE))
	{
		spi->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_TXDMAEN);
		/* Wait for last byte transmission/receiving */
		while(spi->SR & SPI_SR_BSY);
		obj->irq_res = spi::RES_OK;
	}
	else if((spi->CR2 & SPI_CR2_ERRIE) &&
		(sr & (SPI_SR_UDR | SPI_SR_MODF | SPI_SR_OVR)))
	{
		spi->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);
		if(obj->tx_buff)
		{
			obj->tx_dma.stop();
			obj->tx_buff = NULL;
		}
		if(obj->rx_buff)
		{
			obj->rx_dma.stop();
			obj->rx_buff = NULL;
		}
		obj->irq_res = spi::RES_FAIL;
	}
	
	if(obj->_cs)
	{
		obj->_cs->set(1);
		obj->_cs = NULL;
	}
	
	BaseType_t hi_task_woken = 0;
	xSemaphoreGiveFromISR(obj->irq_lock, &hi_task_woken);
#if configUSE_TRACE_FACILITY
	vTraceStoreISREnd(hi_task_woken);
#endif
	portYIELD_FROM_ISR(hi_task_woken);
}

extern "C" void SPI1_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_1]);
}

#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F410Cx) || defined(STM32F410Rx) || \
	defined(STM32F411xE) || defined(STM32F412Cx) || defined(STM32F412Rx) || \
	defined(STM32F412Vx) || defined(STM32F412Zx) || defined(STM32F413xx) || \
	defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
extern "C" void SPI2_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_2]);
}
#endif

#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) || \
	defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F412Cx) || \
	defined(STM32F412Rx) || defined(STM32F412Vx) || defined(STM32F412Zx) || \
	defined(STM32F413xx) || defined(STM32F415xx) || defined(STM32F417xx) || \
	defined(STM32F423xx) || defined(STM32F427xx) || defined(STM32F429xx) || \
	defined(STM32F437xx) || defined(STM32F439xx) || defined(STM32F446xx) || \
	defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void SPI3_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_3]);
}
#endif

#if defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F446xx) || defined(STM32F469xx) || \
	defined(STM32F479xx)
extern "C" void SPI4_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_4]);
}
#endif

#if defined(STM32F410Cx) || defined(STM32F410Rx) || defined(STM32F411xE) || \
	defined(STM32F412Cx) || defined(STM32F412Rx) || defined(STM32F412Vx) || \
	defined(STM32F412Zx) || defined(STM32F413xx) || defined(STM32F423xx) || \
	defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void SPI5_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_5]);
}
#endif

#if defined(STM32F427xx) || defined(STM32F429xx) || defined(STM32F437xx) || \
	defined(STM32F439xx) || defined(STM32F469xx) || defined(STM32F479xx)
extern "C" void SPI6_IRQHandler(void)
{
	spi_irq_hndlr(obj_list[spi::SPI_6]);
}
#endif
