/*_____________________________________________________________________________
 │                                                                            |
 │ COPYRIGHT (C) 2022 Mihai Baneu                                             |
 │                                                                            |
 | Permission is hereby  granted,  free of charge,  to any person obtaining a |
 | copy of this software and associated documentation files (the "Software"), |
 | to deal in the Software without restriction,  including without limitation |
 | the rights to  use, copy, modify, merge, publish, distribute,  sublicense, |
 | and/or sell copies  of  the Software, and to permit  persons to  whom  the |
 | Software is furnished to do so, subject to the following conditions:       |
 |                                                                            |
 | The above  copyright notice  and this permission notice  shall be included |
 | in all copies or substantial portions of the Software.                     |
 |                                                                            |
 | THE SOFTWARE IS PROVIDED  "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS |
 | OR   IMPLIED,   INCLUDING   BUT   NOT   LIMITED   TO   THE  WARRANTIES  OF |
 | MERCHANTABILITY,  FITNESS FOR  A  PARTICULAR  PURPOSE AND NONINFRINGEMENT. |
 | IN NO  EVENT SHALL  THE AUTHORS  OR  COPYRIGHT  HOLDERS  BE LIABLE FOR ANY |
 | CLAIM, DAMAGES OR OTHER LIABILITY,  WHETHER IN AN ACTION OF CONTRACT, TORT |
 | OR OTHERWISE, ARISING FROM,  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR  |
 | THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                 |
 |____________________________________________________________________________|
 |                                                                            |
 |  Author: Mihai Baneu                           Last modified: 26.Sep.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "string.h"
#include "queue.h"
#include "gpio.h"
#include "system.h"
#include "tft.h"
#include "spi.h"
#include "st7735.h"

typedef struct color_16_bit_t {
    uint16_t g_msb:3;
    uint16_t r:5;
    uint16_t b:5;
    uint16_t g_lsb:3;
} color_16_bit_t;

#define RGB(R,G,B)  .r = (uint8_t)(R >> 3), .b = (uint8_t)(B >> 3), .g_lsb = (uint8_t)(G >> 2), .g_msb = (uint8_t)(G >> 5)
static const color_16_bit_t rgb_black    = { RGB(0,0,0) };
static const color_16_bit_t rgb_white    = { RGB(255,255,255) };
static const color_16_bit_t rgb_red      = { RGB(255,0,0) };
static const color_16_bit_t rgb_lime     = { RGB(0,255,0) };
static const color_16_bit_t rgb_blue     = { RGB(0,0,255) };
static const color_16_bit_t rgb_yellow   = { RGB(255,255,0) };
static const color_16_bit_t rgb_cyan     = { RGB(0,255,255) };
static const color_16_bit_t rgb_magenta  = { RGB(255,0,255) };
static const color_16_bit_t rgb_sylver   = { RGB(192,192,192) };
static const color_16_bit_t rgb_gray     = { RGB(128,128,128) };
static const color_16_bit_t rgb_maroon   = { RGB(128,0,0) };
static const color_16_bit_t rgb_olive    = { RGB(128,128,0) };
static const color_16_bit_t rgb_green    = { RGB(0,128,0) };
static const color_16_bit_t rgb_purple   = { RGB(128,0,128) };
static const color_16_bit_t rgb_teal     = { RGB(0,128,128) };
static const color_16_bit_t rgb_navy     = { RGB(0,0,128) };


 /* Queue used to communicate TFT update messages. */
QueueHandle_t tft_queue = NULL;

void tft_init()
{
    tft_queue = xQueueCreate(2, sizeof(tft_event_t));
}

extern const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char	 pixel_data[160 * 128 * 2 + 1];
} image;

extern const uint8_t u8x8_font_8x13B_1x2_f[];


static color_16_bit_t vram[160*128];
static const uint16_t vram_max_row_size = 160;
static const uint16_t vram_max_col_size = 128;
static const uint16_t vram_size = 160 * 128;
static const uint16_t vram_element_size = sizeof(color_16_bit_t);

static void draw_h_line(uint8_t x1, uint8_t x2, uint8_t y, const color_16_bit_t color)
{
    color_16_bit_t *buffer = vram;
    for (uint16_t i = x1; i <= x2; i++) {
            *buffer = color;
            buffer++;
    }

    st7735_row_address_set(x1, x2);
    st7735_column_address_set(y, y);
    st7735_memory_write((uint8_t *)vram, (x2-x1+1)*vram_element_size);
}

static void draw_v_line(uint8_t y1, uint8_t y2, uint8_t x, const color_16_bit_t color)
{
    color_16_bit_t *buffer = vram;
    for (uint16_t i = y1; i <= y2; i++) {
            *buffer = color;
            buffer++;
    }

    st7735_row_address_set(x, x);
    st7735_column_address_set(y1, y2);
    st7735_memory_write((uint8_t *)vram, (y2-y1+1)*vram_element_size);
}

static void draw_line_low(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, color_16_bit_t color)
{
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    int32_t yi = 1;

    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }

    int32_t D = (2 * dy) - dx;
    int32_t y = y1;

    for (uint8_t x = x1; x <= x2; x++) {
        st7735_row_address_set(x, x);
        st7735_column_address_set(y, y);
        st7735_memory_write((uint8_t *)&color, vram_element_size);
        if (D > 0) {
            y = y + yi;
            D = D + (2 * (dy - dx));
        }
        else
        {
            D = D + 2*dy;
        }
    }
}

static void draw_line_high(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, const color_16_bit_t color)
{
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    int32_t xi = 1;

    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int32_t D = (2 * dx) - dy;
    int32_t x = x1;

    for (uint8_t y = y1; y <= y2; y++) {
        st7735_row_address_set(x, x);
        st7735_column_address_set(y, y);
        st7735_memory_write((uint8_t *)&color, vram_element_size);
        if (D > 0) {
            x = x + xi;
            D = D + (2 * (dx - dy));
        }
        else
        {
            D = D + 2*dx;
        }
    }
}

static void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, const color_16_bit_t color)
{
    if (abs(y2 - y1) < abs(x2 - x1)) {
        if (x1 > x2)
            draw_line_low(x2, y2, x1, y1, color);
        else
            draw_line_low(x1, y1, x2, y2, color);
    } 
    else {
        if (y1 > y2)
            draw_line_high(x2, y2, x1, y1, color);
        else
            draw_line_high(x1, y1, x2, y2, color);
    }
}

static void draw_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, const color_16_bit_t color)
{
    color_16_bit_t *buffer = vram;
    for (uint16_t i = x1; i <= x2; i++) {
        for (uint16_t j = y1; j <= y2; j++) {
            *buffer = color;
            buffer++;
        }
    }

    st7735_row_address_set(x1, x2);
    st7735_column_address_set(y1, y2);
    st7735_memory_write((uint8_t *)vram, (x2-x1+1)*(y2-y1+1)*vram_element_size);
}



static void draw_filled_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, const color_16_bit_t color, const color_16_bit_t fill)
{
    color_16_bit_t *buffer = vram;
    for (uint16_t i = x1; i <= x2; i++) {
        for (uint16_t j = y1; j <= y2; j++) {
            if ((i == x1) || (i == (x2)) || (j == y1) || (j == y2)) {
                *buffer = color;
            } 
            else {
                *buffer = fill;
            }
            buffer++;
        }
    }

    st7735_row_address_set(x1, x2);
    st7735_column_address_set(y1, y2);
    st7735_memory_write((uint8_t *)vram, (x2-x1+1)*(y2-y1+1)*vram_element_size);
}

static void draw_circle(uint8_t x1, uint8_t y1, uint8_t r, uint16_t color, color_16_bit_t fill)
{
    
}

static void draw_image(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const unsigned char *pixel_data)
{
    color_16_bit_t *buffer = vram;
    for (uint16_t i = 0; i < width; i++) {
        for (uint16_t j = 0; j < height; j++) {
            color_16_bit_t color = { .r = pixel_data[1] >> 3,
                                     .b = pixel_data[0],
                                     .g_msb = pixel_data[1],
                                     .g_lsb = pixel_data[0] >> 5};
            *buffer = color;
            buffer++;
            pixel_data += 2;
        }
    }

    st7735_row_address_set(x, x + width - 1);
    st7735_column_address_set(y, y + height - 1);
    st7735_memory_write((uint8_t *)vram, width*height*vram_element_size);
}

static uint8_t get_font_data(const uint8_t *adr)
{
    return (*(const uint8_t *)(adr));
}

static void get_glyph_data(const uint8_t *font, uint8_t th, uint8_t tv, uint8_t encoding, uint8_t *buf, uint8_t tile_offset)
{
    uint8_t first, last;
    uint16_t offset;

    first = get_font_data(font + 0);
    last = get_font_data(font + 1);
    if (first <= encoding && encoding <= last) {
        offset = encoding;
        offset -= first;
        offset *= th * tv;
        offset += tile_offset;
        offset *= 8;
        offset += 4;
        for (uint8_t i = 0; i < 8; i++) {
            buf[i] = get_font_data(font + offset);
            offset++;
        }
    } else {
        for (uint8_t i = 0; i < 8; i++) {
            buf[i] = 0;
        }
    }
}

static void draw_tile(const uint8_t *buffer, uint8_t x, uint8_t y, color_16_bit_t color, color_16_bit_t background)
{
    const uint8_t colormask[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    color_16_bit_t ram[8*8];

    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            ram[i*8+j] = (buffer[i] & colormask[j]) ? color : background;
        }
    }

    st7735_row_address_set(x, x + 8 - 1);
    st7735_column_address_set(y, y + 8 - 1);
    st7735_memory_write((uint8_t *)ram, 8*8*sizeof(color_16_bit_t));
}

static void draw_glyph(const uint8_t *font, uint8_t x, uint8_t y, uint8_t th, uint8_t tv, color_16_bit_t color, color_16_bit_t background, uint8_t encoding)
{
    uint8_t buffer[8];
    uint8_t tile = 0;;

    for (uint8_t xx = 0; xx < th; xx++) {
        for (uint8_t yy = 0; yy < tv; yy++) {
            get_glyph_data(font, th, tv, encoding, buffer, tile);
            draw_tile(buffer, xx * 8 + x, (tv-yy-1) * 8 + y, color, background);
            tile++;
        }
    }
}

static void draw_char(const uint8_t *font, uint8_t x, uint8_t y, color_16_bit_t color, color_16_bit_t background, const char c)
{
    uint8_t th = get_font_data(font + 2);
    uint8_t tv = get_font_data(font + 3);

    draw_glyph(font, x, y, th, tv, color, background, c);
}

static void draw_string(const uint8_t *font, uint8_t x, uint8_t y, color_16_bit_t color, color_16_bit_t background, const char *s)
{
    uint8_t th = get_font_data(font + 2);
    uint8_t tv = get_font_data(font + 3);

    while (*s != 0) {
        draw_glyph(font, x, y, th, tv, color, background, *s);
        x += th * 8;
        s++;
    }
}

void tft_run(void *params)
{
    (void)params;

    st7735_hw_control_t hw = {
        .res_high = gpio_tft_res_high,
        .res_low  = gpio_tft_res_low,
        .dc_high  = gpio_tft_dc_high,
        .dc_low   = gpio_tft_dc_low,
        .data_wr  = spi_write,
        .data_rd  = spi_read,
        .delay_us = delay_us
    };

    st7735_init(hw);

    /* perform a HW reset */
    st7735_hardware_reset();
    st7735_sleep_out_and_booster_on();
    delay_us(10000);
    st7735_normal_mode();

    /* configure the display */
    st7735_display_inversion_off();
    st7735_interface_pixel_format(ST7735_16_PIXEL);
    st7735_display_on();
    st7735_memory_data_access_control(0, 0, 0, 0, 0, 0);
    st7735_column_address_set(0, 128-1);
    st7735_row_address_set(0, 160-1);
 
    uint8_t all = 0;
    for (;;) {
        //goto draw_text;
        all = 1;

draw_fill:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        for (uint16_t i = 0; i < 52; i++) {
            color_16_bit_t color = { RGB(i*5, i*5, i*5) };
            draw_fill(i, i, 160-1-i, 128-1-i, color);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        if (!all) continue;

draw_region:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        draw_fill(20, 20, 120, 90, rgb_red);
        draw_fill(30, 50, 100, 90, rgb_blue);
        draw_fill(15, 80,  90, 90, rgb_cyan);
        draw_fill(80, 30,  90, 90, rgb_green);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!all) continue;

draw_filled_rectangle:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        draw_filled_rectangle(40, 32, 120, 96, rgb_red, rgb_yellow);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        draw_filled_rectangle(10, 15, 50, 78, rgb_red, rgb_yellow);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        draw_filled_rectangle(60, 40, 120, 100, rgb_red, rgb_yellow);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!all) continue;

draw_lines:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        for (uint8_t i = 0; i < 160; i++) {
            if (i < 128) {
                draw_h_line(0, 160-1, i, rgb_red);
            }
            draw_v_line(0, 128-1, i, rgb_blue);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        for (uint8_t i = 0; i < 160; i++) {
            draw_line(0, 0, i, 128-1, rgb_red);
        }
        for (uint8_t i = 128; i > 0; i--) {
            draw_line(0, 0, 160-1, i-1, rgb_red);
        }

        for (uint8_t i = 0; i < 160; i++) {
            draw_line(0, 128-1, i, 0, rgb_blue);
        }
        for (uint8_t i = 0; i < 128; i++) {
            draw_line(0, 128-1, 160-1, i, rgb_blue);
        }

        for (uint8_t i = 160; i > 0; i--) {
            draw_line(160-1, 0, i-1, 128-1, rgb_green);
        }
        for (uint8_t i = 128; i > 0; i--) {
            draw_line(160-1, 0, 0, i-1, rgb_green);
        }

        for (uint8_t i = 160; i > 0; i--) {
            draw_line(160-1, 128-1, i-1, 0, rgb_yellow);
        }
        for (uint8_t i = 0; i < 128; i++) {
            draw_line(160-1, 128-1, 0, i, rgb_yellow);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!all) continue;
   
draw_image:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        draw_image(0, 0, image.width, image.height, image.pixel_data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (!all) continue;

draw_text:
        draw_fill(0, 0, 160-1, 128-1, rgb_white);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        draw_string(u8x8_font_8x13B_1x2_f, 2*8, 7*8, rgb_black, rgb_white, "Hello: ");
        for (int i = 0; i < 2500; i++) {
            char txt[5] = { 0, 0, 0, 0, 0};
            itoa(i, txt, 10);
            draw_string(u8x8_font_8x13B_1x2_f, 9*8, 7*8, rgb_black, rgb_white, txt);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        if (!all) continue;
    }
}
