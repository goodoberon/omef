
#include <string.h>

#include "gpio/gpio.hpp"
#include "drv/sd/sd_spi.hpp"
#include "drv/di/di.hpp"
#include "third_party/FatFs/ff.h"
#include "ul/fatfs_diskio/fatfs_diskio.hpp"
#include "FreeRTOS.h"
#include "task.h"

using namespace hal;
using namespace drv;

static void cd_cb(di *di, bool state, void *ctx);

static void di_task(void *pvParameters)
{
	di *cd_di = (di *)pvParameters;
	while(1)
	{
		cd_di->poll();
		vTaskDelay(1);
	}
}

int main(void)
{
	static gpio b1(0, 0, gpio::MODE_DI, 0);
	
	static gpio spi1_mosi(0, 7, gpio::MODE_AF, 0);
	static gpio spi1_miso(0, 6, gpio::MODE_AF, 0);
	static gpio spi1_clk(0, 5, gpio::MODE_AF, 0);
	static gpio sd_cs(0, 4, gpio::MODE_DO, 1);
	static gpio sd_cd(0, 3, gpio::MODE_DI, 1);
	
	static dma spi1_rx_dma(dma::DMA_2, dma::STREAM_0, dma::CH_3,
		dma::DIR_PERIPH_TO_MEM, dma::INC_SIZE_8);
	static dma spi1_tx_dma(dma::DMA_2, dma::STREAM_3, dma::CH_3,
		dma::DIR_MEM_TO_PERIPH, dma::INC_SIZE_8);
	
	static spi spi1(spi::SPI_1, 1000000, spi::CPOL_0, spi::CPHA_0,
		spi::BIT_ORDER_MSB, spi1_tx_dma, spi1_rx_dma, spi1_mosi, spi1_miso,
		spi1_clk);
	
	static sd_spi sd1(spi1, sd_cs, &sd_cd);
	
	static di cd_di(sd_cd, 50, 1);
	cd_di.cb(cd_cb, &sd1);
	
	ul::fatfs_diskio_add(0, sd1);
	
	xTaskCreate(di_task, "di", configMINIMAL_STACK_SIZE * 3, &cd_di,
		tskIDLE_PRIORITY + 2, NULL);
	
	vTaskStartScheduler();
}

static void cd_cb(di *di, bool state, void *ctx)
{
	if(!state)
		return;
	
	sd *sd1 = (sd *)ctx;
	
	FATFS fs;
	FRESULT ff_res = f_mount(&fs, "SD", 1);
	if(ff_res)
	{
		ff_res = f_mkfs("SD", FM_FAT32, 0, NULL, 0);
		if(ff_res)
			return;
		ff_res = f_mount(&fs, "SD", 1);
		if(ff_res)
			return;
		ff_res = f_unmount("SD");
		if(ff_res)
			return;
	}
	
	FIL file;
	ff_res = f_open(&file, "SD:test.txt", FA_WRITE | FA_CREATE_ALWAYS);
	if(ff_res)
		return;
	UINT size = strlen("abcdefgh-1234567890");
	ff_res = f_write(&file, "abcdefgh-1234567890", size, &size);
	if(ff_res)
		return;
	ff_res = f_close(&file);
	if(ff_res)
		return;
	ff_res = f_open(&file, "SD:test.txt", FA_READ);
	if(ff_res)
		return;
	
	size = 0;
	uint8_t buff[64];
	memset(buff, 0, sizeof(buff));
	ff_res = f_read(&file, buff, sizeof(buff), &size);
	
	f_close(&file);
}
