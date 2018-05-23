/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "usbcdc.h"
#include "delay.h"
#include "lcd/lcdhw.h"

int main(void)
{
	// rcc_clock_setup_in_hsi_out_48mhz();
	rcc_clock_setup_in_hse_8mhz_out_72mhz(); 

	rcc_periph_clock_enable(RCC_AFIO);
	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_clear(GPIOA, GPIO2 | GPIO8);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO2 | GPIO8);

	delay_setup();

	lcd_init(0);

	lcd_clr(0x000F);

	lcd_box(20, 10, 200, 100, 0xF800);

	uint8_t c = 0;
	for(int y=0; y<240; y += 14) {
		for(int x=0; x<315; x += 9) {
			lcd_char(c, x, y, 0xFFFF, 0);
			c++;
		}
	}

	usbcdc_init();

	int clr = 0;
	while (1) {
		usbcdc_poll();

		// lcd_clr( (clr & 4)?0xF800:0 | (clr & 2)?0x07E0:0 | (clr & 1)?0x001F:0);
		clr = (clr+1) & 7;
	}
}
