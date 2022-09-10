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
 |  Author: Mihai Baneu                           Last modified: 10.Sep.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "queue.h"
#include "gpio.h"
#include "system.h"
#include "tft.h"

 /* Queue used to communicate TFT update messages. */
QueueHandle_t tft_queue = NULL;

void tft_init()
{
    tft_queue = xQueueCreate(2, sizeof(tft_event_t));
}

void tft_run(void *params)
{
    (void)params;
    static int call_number = 0;
//
//    st7066u_hw_control_t hw = {
//        .config_control_out  = gpio_config_control_out,
//        .config_data_out     = gpio_config_data_out,
//        .config_data_in      = gpio_config_data_in,
//        .e_high              = gpio_e_high,
//        .e_low               = gpio_e_low,
//        .rs_high             = gpio_rs_high,
//        .rs_low              = gpio_rs_low,
//        .data_wr             = gpio_data_wr,
//        .data_rd             = gpio_data_rd,
//        .delay_us            = delay_us
//    };
//
//    st7066u_init(hw);
//
//    st7066u_cmd_function_set(ST7066U_8_BITS_DATA, ST7066U_2_LINE_DISPLAY, ST7066U_5x8_SIZE);
//    st7066u_cmd_on_off(ST7066U_DISPLAY_ON, ST7066U_CURSOR_OFF, ST7066U_CURSOR_POSITION_OFF);
//    st7066u_cmd_clear_display();
//    st7066u_cmd_entry_mode(ST7066U_INCREMENT_ADDRESS, ST7066U_SHIFT_DISABLED);
//
//    st7066u_write_str("    Welcome!    ");
//    st7066u_cmd_set_ddram(0x40);
//    st7066u_write_str("I2C  EEPROM  DMA");
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    for (;;) {
        tft_event_t tft_event;
        if (xQueueReceive(tft_queue, &tft_event, portMAX_DELAY) == pdPASS) {
            // display
            //st7066u_cmd_clear_display();
            //st7066u_write_str(lcd_event.row1_txt);
            //st7066u_cmd_set_ddram(0x40);
            //st7066u_write_str(lcd_event.row2_txt);

            call_number++;
        }
    }
}
