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
 |  Author: Mihai Baneu                           Last modified: 08.Oct.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "stdlib.h"
#include "string.h"
#include "queue.h"
#include "gpio.h"
#include "system.h"
#include "tft.h"
#include "spi.h"
#include "st7735.h"
#include "printf.h"

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
    tft_queue = xQueueCreate(6, sizeof(tft_event_t));
}

void tft_run_(void *params)
{
    (void)params;
    char display_txt[6][17] = { 0 };
    st7735_color_16_bit_t bk_colors[] = {
        st7735_rgb_yellow,
        st7735_rgb_lime,
        st7735_rgb_teal
    };
    uint8_t bk_color_index = 0;

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

    /* prepare the background */
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    st7735_draw_rectangle(10, 10, 150, 120, st7735_rgb_red, bk_colors[bk_color_index]);

    /* process events */
    for (;;) {
        tft_event_t tft_event;
        if (xQueueReceive(tft_queue, &tft_event, portMAX_DELAY) == pdPASS) {
            switch (tft_event.type) {
                case tft_event_text_up:
                    memcpy(display_txt[0], display_txt[1], 17);
                    memcpy(display_txt[1], display_txt[2], 17);
                    memcpy(display_txt[2], display_txt[3], 17);
                    memcpy(display_txt[3], display_txt[4], 17);
                    memcpy(display_txt[4], display_txt[5], 17);
                    memcpy(display_txt[5], tft_event.row_txt, 17);
                    
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 2*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[0]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 4*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[1]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 6*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[2]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 8*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[3]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,10*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[4]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,12*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[5]);
                    break;

                case tft_event_text_down:
                    memcpy(display_txt[5], display_txt[4], 17);
                    memcpy(display_txt[4], display_txt[3], 17);
                    memcpy(display_txt[3], display_txt[2], 17);
                    memcpy(display_txt[2], display_txt[1], 17);
                    memcpy(display_txt[1], display_txt[0], 17);
                    memcpy(display_txt[0], tft_event.row_txt, 17);
                    
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 2*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[0]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 4*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[1]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 6*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[2]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 8*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[3]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,10*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[4]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,12*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[5]);
                    break;
                
                case tft_event_background:
                    bk_color_index++;
                    if (bk_color_index >= sizeof(bk_colors)/sizeof(st7735_color_16_bit_t)) {
                        bk_color_index = 0;
                    }

                    /* prepare the background */
                    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
                    st7735_draw_rectangle(10, 10, 150, 120, st7735_rgb_red, bk_colors[bk_color_index]);

                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 2*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[0]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 4*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[1]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 6*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[2]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 8*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[3]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,10*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[4]);
                    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8,12*8, st7735_rgb_black, bk_colors[bk_color_index], display_txt[5]);
                    break;
                
                default:
                    break;
            }
        }
    }
}

static void test_draw_display_id()
{
    char txt[16] = { 0 };
    uint32_t id;
    uint8_t id1, id2, id3;

    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    st7735_read_display_id(&id);
    sprintf(txt, "ID:  0x%08X", id);
    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 2*8, st7735_rgb_black, st7735_rgb_white, txt);

    st7735_read_id1(&id1);
    sprintf(txt, "ID1: 0x%02X", id1);
    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 5*8, st7735_rgb_black, st7735_rgb_white, txt);

    st7735_read_id2(&id2);
    sprintf(txt, "ID2: 0x%02X", id2);
    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 8*8, st7735_rgb_black, st7735_rgb_white, txt);

    st7735_read_id3(&id3);
    sprintf(txt, "ID3: 0x%02X", id3);
    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 11*8, st7735_rgb_black, st7735_rgb_white, txt);

    st7735_display_inversion_on();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_display_inversion_off();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_display_inversion_on();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_display_inversion_off();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_fill()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { ST7735_RGB(0, 0, i*5) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { ST7735_RGB(0, i*5, 0) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint16_t i = 0; i < 50; i++) {
        st7735_color_16_bit_t color = { ST7735_RGB(i*5, 0, 0) };
        st7735_draw_fill(i, i, 160-1-i, 128-1-i, color);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_region()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_fill(20, 20, 140, 110, st7735_rgb_red);
    st7735_draw_fill(30, 30, 50, 50, st7735_rgb_blue);
    st7735_draw_fill(80, 80,  100, 100, st7735_rgb_cyan);
    st7735_draw_fill(60, 30,  90, 90, st7735_rgb_green);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_filled_rectangle()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(40, 32, 120, 96, st7735_rgb_red, st7735_rgb_yellow);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(10, 15, 50, 78, st7735_rgb_red, st7735_rgb_cyan);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_rectangle(60, 70, 143, 100, st7735_rgb_red, st7735_rgb_lime);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_lines()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for (uint8_t i = 0; i < 160; i++) {
        if (i < 128) {
            st7735_draw_h_line(0, 160-1, i, st7735_rgb_red);
        }
        st7735_draw_v_line(0, 128-1, i, st7735_rgb_blue);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    for (uint8_t i = 0; i < 160; i++) {
        st7735_draw_line(0, 0, i, 128-1, st7735_rgb_red);
    }
    for (uint8_t i = 128; i > 0; i--) {
        st7735_draw_line(0, 0, 160-1, i-1, st7735_rgb_red);
    }

    for (uint8_t i = 0; i < 160; i++) {
        st7735_draw_line(0, 128-1, i, 0, st7735_rgb_blue);
    }
    for (uint8_t i = 0; i < 128; i++) {
        st7735_draw_line(0, 128-1, 160-1, i, st7735_rgb_blue);
    }

    for (uint8_t i = 160; i > 0; i--) {
        st7735_draw_line(160-1, 0, i-1, 128-1, st7735_rgb_green);
    }
    for (uint8_t i = 128; i > 0; i--) {
        st7735_draw_line(160-1, 0, 0, i-1, st7735_rgb_green);
    }

    for (uint8_t i = 160; i > 0; i--) {
        st7735_draw_line(160-1, 128-1, i-1, 0, st7735_rgb_yellow);
    }
    for (uint8_t i = 0; i < 128; i++) {
        st7735_draw_line(160-1, 128-1, 0, i, st7735_rgb_yellow);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_circles()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);

    st7735_draw_fill_circle(50, 50, 30, st7735_rgb_green);
    st7735_draw_fill_circle(100, 50, 10, st7735_rgb_yellow);
    st7735_draw_fill_circle(80, 80, 15, st7735_rgb_red);
    st7735_draw_fill_circle(70, 20, 20, st7735_rgb_blue);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    st7735_draw_circle(50, 50, 30, st7735_rgb_black);
    st7735_draw_circle(100, 50, 10, st7735_rgb_black);
    st7735_draw_circle(80, 80, 15, st7735_rgb_black);
    st7735_draw_circle(70, 20, 20, st7735_rgb_black);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_image()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_black);
    st7735_draw_image(0, 0, logo.width, logo.height, (uint8_t *)logo.pixel_data);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static uint8_t read_buffer[128*3] = { 0 };
static st7735_color_16_bit_t write_buffer[128];
static void test_draw_read_write()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_black);
    st7735_draw_image(0, 0, logo.width, logo.height, (uint8_t *)logo.pixel_data);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    for (uint8_t i = 0; i < 160; i++) {
        st7735_interface_pixel_format(ST7735_18_PIXEL);
        st7735_row_address_set(i, i);
        st7735_column_address_set(0, 128 - 1);
        st7735_memory_read(read_buffer, sizeof(read_buffer));
        st7735_interface_pixel_format(ST7735_16_PIXEL);

        vTaskDelay(10 / portTICK_PERIOD_MS);
        for (uint8_t j = 0; j < 128; j++) {
            st7735_color_16_bit_t color =  { ST7735_RGB_18bit((255 - read_buffer[j*3+1]), (255 - read_buffer[j*3+2]), (255 - read_buffer[j*3]))  };
            write_buffer[j] = color;
        }

        st7735_row_address_set(i, i);
        st7735_column_address_set(0, 128 - 1);
        st7735_memory_write(write_buffer, sizeof(write_buffer), 1);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_text()
{
    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    st7735_draw_string(u8x8_font_8x13B_1x2_f, 2*8, 7*8, st7735_rgb_black, st7735_rgb_white, "Text: ");
    for (int i = 0; i < 2500; i++) {
        char txt[5] = { 0, 0, 0, 0, 0};
        itoa(i, txt, 10);
        st7735_draw_string(u8x8_font_8x13B_1x2_f, 9*8, 7*8, st7735_rgb_black, st7735_rgb_white, txt);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void test_draw_animation()
{
    uint8_t mario_x = 0, mario_y = 90;

    st7735_draw_fill(0, 0, 160-1, 128-1, st7735_rgb_white);
    st7735_draw_image(78, mario_y + mario.height - plant.height, plant.width, plant.height, (uint8_t *)plant.pixel_data);

    while (mario_x <= (160 - mario.width - 1)) {
        st7735_draw_fill(mario_x, mario_y, mario_x + mario.width-1, mario_y + mario.height-1, st7735_rgb_white);
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

        st7735_draw_image(mario_x, mario_y, mario.width, mario.height, (uint8_t *)mario.pixel_data);
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
        //goto draw_read_write;
        all = 1;

draw_display_id:
        test_draw_display_id();
        if (!all) continue;

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

draw_circles:
        test_draw_circles();
        if (!all) continue;
   
draw_image:
        test_draw_image();
        if (!all) continue;

draw_read_write:
        test_draw_read_write();
        if (!all) continue;

draw_text:
        test_draw_text();        
        if (!all) continue;

draw_animation:
        test_draw_animation();        
        if (!all) continue;
    }
}