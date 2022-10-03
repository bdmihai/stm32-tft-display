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

extern const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned char	 pixel_data[];
} mario;

extern const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned char	 pixel_data[];
} plant;

extern const struct {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned char	 pixel_data[];
} logo;

extern const uint8_t u8x8_font_8x13B_1x2_f[];

 /* Queue used to communicate TFT update messages. */
QueueHandle_t tft_queue = NULL;

void tft_init()
{
    tft_queue = xQueueCreate(2, sizeof(tft_event_t));
}

static void test_draw_fill()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { RGB(0, 0, i*5) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { RGB(0, i*5, 0) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { RGB(i*5, 0, 0) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_region()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_fill(20, 20, 140, 110, rgb_red);
    st7735_draw_fill(30, 30, 50, 50, rgb_blue);
    st7735_draw_fill(80, 80,  100, 100, rgb_cyan);
    st7735_draw_fill(60, 30,  90, 90, rgb_green);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_filled_rectangle()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(40, 32, 120, 96, rgb_red, rgb_yellow);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(10, 15, 50, 78, rgb_red, rgb_cyan);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(60, 70, 143, 100, rgb_red, rgb_lime);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_lines()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint8_t i = 0; i < 160; i++) {
        if (i < 128) {
            st7735_draw_h_line(0, 160-1, i, rgb_red);
        }
        st7735_draw_v_line(0, 128-1, i, rgb_blue);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    for (uint8_t i = 0; i < 160; i++) {
        st7735_draw_line(0, 0, i, 128-1, rgb_red);
    }
    for (uint8_t i = 128; i > 0; i--) {
        st7735_draw_line(0, 0, 160-1, i-1, rgb_red);
    }

    for (uint8_t i = 0; i < 160; i++) {
        st7735_draw_line(0, 128-1, i, 0, rgb_blue);
    }
    for (uint8_t i = 0; i < 128; i++) {
        st7735_draw_line(0, 128-1, 160-1, i, rgb_blue);
    }

    for (uint8_t i = 160; i > 0; i--) {
        st7735_draw_line(160-1, 0, i-1, 128-1, rgb_green);
    }
    for (uint8_t i = 128; i > 0; i--) {
        st7735_draw_line(160-1, 0, 0, i-1, rgb_green);
    }

    for (uint8_t i = 160; i > 0; i--) {
        st7735_draw_line(160-1, 128-1, i-1, 0, rgb_yellow);
    }
    for (uint8_t i = 0; i < 128; i++) {
        st7735_draw_line(160-1, 128-1, 0, i, rgb_yellow);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_image()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_black);
    st7735_draw_image(0, 0, logo.width, logo.height, logo.pixel_data);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_text()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 7*8, rgb_black, rgb_white, "Text: ");
    for (int i = 0; i < 2500; i++) {
        char txt[5] = { 0, 0, 0, 0, 0};
        itoa(i, txt, 10);
        st7735_draw_string(u8x8_font_8x13B_1x2_f, 9*8, 7*8, rgb_black, rgb_white, txt);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_animation()
{
    uint8_t mario_x = 0, mario_y = 90;

    st7735_draw_fill(0, 0, 160-1, 128-1, rgb_white);
    st7735_draw_image(78, mario_y + mario.height - plant.height, plant.width, plant.height, plant.pixel_data);

    while (mario_x <= (160 - mario.width - 1)) {
        st7735_draw_fill(mario_x, mario_y, mario_x + mario.width-1, mario_y + mario.height-1, rgb_white);
        mario_x++;
        if (mario_x >= 30 && mario_x < 35) {
            mario_y -= 3;
        }

        if (mario_x >= 35 && mario_x < 40) {
            mario_y += 3;
        }

        if (mario_x >= 60 && mario_x < 81) {
            mario_y -= 3;
        }

        if (mario_x >= 81 && mario_x < 102) {
            mario_y += 3;
        }

        st7735_draw_image(mario_x, mario_y, mario.width, mario.height, mario.pixel_data);
        vTaskDelay(30 / portTICK_PERIOD_MS);
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
    st7735_memory_data_access_control(0, 1, 0, 0, 0, 0);
    st7735_column_address_set(0, 128-1);
    st7735_row_address_set(0, 160-1);
 
    uint8_t all = 0;
    for (;;) {
        //goto draw_image;
        all = 1;

draw_fill:
        test_draw_fill();
        if (!all) continue;

draw_region:
        test_draw_region();
        if (!all) continue;

draw_filled_rectangle:
        test_draw_filled_rectangle();
        if (!all) continue;

draw_lines:
        test_draw_lines();
        if (!all) continue;
   
draw_image:
        test_draw_image();
        if (!all) continue;

draw_text:
        test_draw_text();        
        if (!all) continue;

draw_animation:
        test_draw_animation();        
        if (!all) continue;
    }
}
