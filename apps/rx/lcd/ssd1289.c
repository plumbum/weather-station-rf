#include "lcdhw.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/gpio.h>
#include "../delay.h"

#include "font8x14.h"

// Display ID 0x8989
// Controller: SSD1289 / Display: TFT8K1711FPC-A1-E / Board: YX32B

#define LCD_WIDTH 320
#define LCD_HEIGHT 240
#define LCD_SIZE (LCD_WIDTH*LCD_HEIGHT)

#define PIN_BL /* GPIOA, */ GPIO8 
#define PIN_CS /* GPIOC, */ GPIO13
#define PIN_RS /* GPIOA, */ GPIO0 
#define PIN_WR /* GPIOA, */ GPIO1 
// #define PIN_RD GPIOA, */ GPIO8 

#define lcd_addr() do { GPIO_BRR(GPIOA)  = PIN_RS; } while(0)
#define lcd_data() do { GPIO_BSRR(GPIOA)  = PIN_RS; } while(0)

static void lcd_write(uint16_t data)
{
	GPIO_ODR(GPIOB)  = data;
	GPIO_BRR(GPIOA)  = PIN_WR;
	GPIO_BSRR(GPIOA) = PIN_WR;
}

static void lcd_reg(uint16_t addr, uint16_t data)
{
    lcd_addr();
    lcd_write(addr);
    lcd_data();
    lcd_write(data);
}

static void lcd_moveto(lcd_coord_t x, lcd_coord_t y)
{
    // Set GDDRAM X address counter 
    lcd_reg(0x4f, x);
    // Set GDDRAM Y address counter 
    lcd_reg(0x4e, y);
}

static void lcd_area(lcd_coord_t x1, lcd_coord_t y1, lcd_coord_t x2, lcd_coord_t y2)
{
    // Horizontal RAM address position (R44h)
    lcd_reg(0x44, (y2<<8) | (y1 & 0xFF));
    // Vertical RAM address position (R45h-R46h) 
    lcd_reg(0x45, x1);
    lcd_reg(0x46, x2);
    lcd_moveto(x1, y1);
}

void lcd_box(lcd_coord_t x1, lcd_coord_t y1, lcd_coord_t x2, lcd_coord_t y2, lcd_color_t color)
{
    lcd_area(x1, y1, x2, y2);

    lcd_addr();
    lcd_write(0x22);
    lcd_data();
    for(int i=0; i<(x2-x1+1)*(y2-y1+1); i++) {
        lcd_write(color);
    }

}

void lcd_clr(lcd_color_t color)
{
    lcd_area(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1);

    lcd_addr();
    lcd_write(0x22);
    lcd_data();
    for(int i=0; i<LCD_SIZE; i++) {
        lcd_write(color);
    }
}

void lcd_char(char c, lcd_coord_t x, lcd_coord_t y, lcd_color_t fg, lcd_color_t bg)
{

    lcd_area(x, y, x+7, y+13);
    lcd_addr();
    lcd_write(0x22);
    lcd_data();

    uint8_t* pf = font8x14 + c*14;
    for(int j=0; j<14; j++)
    {
        uint8_t b = *pf++;
        for(int i=0; i<8; i++) {
            if (b & 0x80) {
                lcd_write(fg);
            } else {
                lcd_write(bg);
            }
            b <<= 1;
        }
    }
}

static void lcd_iface_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

    // Port C
	gpio_set(GPIOC, PIN_CS);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, PIN_CS);
    // Port A
	gpio_set(GPIOA, PIN_BL | PIN_RS | PIN_WR);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL,  PIN_BL | PIN_RS | PIN_WR);
    // Port B
	gpio_set(GPIOB, 0xFFFF);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, 0xFFFF);
    
    // Chip Select display
    gpio_clear(GPIOC, PIN_CS);
}

static void lcd_controller_init(void)
{
    // power supply setting
    // set R07h at 0021h (GON=1,DTE=0,D[1:0]=01)
    lcd_reg(0x07, 0x0021);
    // set R00h at 0001h (OSCEN=1)
    lcd_reg(0x00, 0x0001);
    // set R07h at 0023h (GON=1,DTE=0,D[1:0]=11)
    lcd_reg(0x07, 0x0023);
    // set R10h at 0000h (Exit sleep mode)
    lcd_reg(0x10, 0x0000);
    // Wait 30ms
    delay_us(30000);
    // set R07h at 0033h (GON=1,DTE=1,D[1:0]=11)
    lcd_reg(0x07, 0x0033);
    // Entry mode setting (R11h)
    // R11H Entry mode
    // vsmode DFM1 DFM0 TRANS OEDef WMode DMode1 DMode0 TY1 TY0 ID1 ID0 AM LG2 LG2 LG0
    //   0     1    1     0     0     0     0      0     0   1   1   1  *   0   0   0
    lcd_reg(0x11, 0x6078); // 0x6070
    // LCD driver AC setting (R02h)
    lcd_reg(0x02, 0x0600);
    // power control 1
    // DCT3 DCT2 DCT1 DCT0 BT2 BT1 BT0 0 DC3 DC2 DC1 DC0 AP2 AP1 AP0 0
    // 1     0    1    0    1   0   0  0  1   0   1   0   0   1   0  0
    // DCT[3:0] fosc/4 BT[2:0]  DC{3:0] fosc/4
    lcd_reg(0x03, 0x0804); // 0xA8A4
    lcd_reg(0x0C, 0x0000); 
    lcd_reg(0x0D, 0x0808); // 0x080C --> 0x0808
    // power control 4
    // 0 0 VCOMG VDV4 VDV3 VDV2 VDV1 VDV0 0 0 0 0 0 0 0 0
    // 0 0   1    0    1    0    1    1   0 0 0 0 0 0 0 0
    lcd_reg(0x0E, 0x2900); 
    lcd_reg(0x1E, 0x00B8); 
    lcd_reg(0x01, 0x293F);  // 0x2B3F); // Driver output control 320*240  0x6B3F
    lcd_reg(0x10, 0x0000); 
    lcd_reg(0x05, 0x0000); 
    lcd_reg(0x06, 0x0000); 
    lcd_reg(0x16, 0xEF1C); 
    lcd_reg(0x17, 0x0003); 
    lcd_reg(0x07, 0x0233);  // 0x0233
    lcd_reg(0x0B, (3<<6));
    lcd_reg(0x0F, 0x0000);  // Gate scan start position
    lcd_reg(0x41, 0x0000); 
    lcd_reg(0x42, 0x0000); 
    lcd_reg(0x48, 0x0000); 
    lcd_reg(0x49, 0x013F); 
    lcd_reg(0x4A, 0x0000); 
    lcd_reg(0x4B, 0x0000); 
    lcd_reg(0x44, 0xEF00); 
    lcd_reg(0x45, 0x0000); 
    lcd_reg(0x46, 0x013F); 
    lcd_reg(0x30, 0x0707); 
    lcd_reg(0x31, 0x0204); 
    lcd_reg(0x32, 0x0204); 
    lcd_reg(0x33, 0x0502); 
    lcd_reg(0x34, 0x0507); 
    lcd_reg(0x35, 0x0204); 
    lcd_reg(0x36, 0x0204); 
    lcd_reg(0x37, 0x0502); 
    lcd_reg(0x3A, 0x0302); 
    lcd_reg(0x3B, 0x0302); 
    lcd_reg(0x23, 0x0000); 
    lcd_reg(0x24, 0x0000); 
    lcd_reg(0x25, 0x8000);    // 65hz
    // lcd_reg(0x4f, 0x0000);  // Set GDDRAM X address counter 
    // lcd_reg(0x4e, 0x0000);  // Set GDDRAM Y address counter 

}

void lcd_init(lcd_color_t color)
{
    lcd_iface_init();
    lcd_controller_init();
    lcd_clr(color);
}


