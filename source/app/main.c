/*_____________________________________________________________________________
 │                                                                            |
 │ COPYRIGHT (C) 2021 Mihai Baneu                                             |
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
 |  Author: Mihai Baneu                           Last modified: 19.May.2022  |
 |                                                                            |
 |___________________________________________________________________________*/

#include "stm32f4xx.h"
#include "stm32rtos.h"
#include "string.h"
#include "task.h"
#include "queue.h"
#include "system.h"
#include "gpio.h"
#include "isr.h"
#include "i2c.h"
#include "dma.h"
#include "printf.h"
#include "led.h"
#include "tft.h"
#include "rencoder.h"

#define EEPROM_SIZE         512
#define EEPROM_PAGE_SIZE    16
#define EEPROM_I2C_ADDRESS  0b01010000

static void query_eeprom(uint16_t counter)
{
    dma_request_event_t dma_request_event;
    dma_response_event_t dma_response_event;
    tft_event_t tft_event = { {0}, {0} };

    // set the location for the read
    dma_request_event.length = 1;
    dma_request_event.type = dma_request_type_i2c_write;
    dma_request_event.address = EEPROM_I2C_ADDRESS + (uint8_t)(counter >> 8); // calculate the read address of the eeprom
    dma_request_event.buffer[0] = (uint8_t)counter;                           // low part needs to be send as data / high byte needs to be send in address
    xQueueSendToBack(dma_request_queue, &dma_request_event, (TickType_t) 1);
    if (xQueueReceive(dma_response_queue, &dma_response_event, portMAX_DELAY) == pdPASS) {
        if (dma_response_event.status != dma_request_status_success) {
            sprintf(tft_event.row1_txt, "%s [%d]", "Write error", dma_response_event.length);
            sprintf(tft_event.row2_txt, "   ---   ");
        }
        else {
            dma_request_event.length = 16;
            dma_request_event.type = dma_request_type_i2c_read;
            dma_request_event.address = EEPROM_I2C_ADDRESS;
            xQueueSendToBack(dma_request_queue, &dma_request_event, (TickType_t) 1);
            if (xQueueReceive(dma_response_queue, &dma_response_event, portMAX_DELAY) == pdPASS) {
                if (dma_response_event.status != dma_request_status_success) {
                    sprintf(tft_event.row1_txt, "%s [%d]", "Read error", dma_response_event.length);
                }
                else {
                    memset(tft_event.row1_txt, 0, sizeof(tft_event.row1_txt));
                    memcpy(tft_event.row1_txt, dma_response_event.buffer, dma_response_event.length);
                }
            }

            dma_request_event.length = 16;
            dma_request_event.type = dma_request_type_i2c_read;
            dma_request_event.address = EEPROM_I2C_ADDRESS;
            xQueueSendToBack(dma_request_queue, &dma_request_event, (TickType_t) 1);
            if (xQueueReceive(dma_response_queue, &dma_response_event, portMAX_DELAY) == pdPASS) {
                if (dma_response_event.status != dma_request_status_success) {
                    sprintf(tft_event.row2_txt, "%s [%d]", "Read error", dma_response_event.length);
                }
                else {
                    memset(tft_event.row2_txt, 0, sizeof(tft_event.row2_txt));
                    memcpy(tft_event.row2_txt, dma_response_event.buffer, dma_response_event.length);
                }
            }
        }
    }

    xQueueSendToBack(tft_queue, &tft_event, (TickType_t) 1);
}

static void user_handler(void *pvParameters)
{
    (void)pvParameters;

    query_eeprom(0);
    for (;;) {
        rencoder_output_event_t event;
        if (xQueueReceive(rencoder_output_queue, &event, portMAX_DELAY) == pdPASS) {
            if (event.type == rencoder_output_rotation) {
                query_eeprom(event.position * 16);
            } else if ((event.type == rencoder_output_key) && (event.key == RENCODER_KEY_RELEASED)) {
                rencoder_reset();
                query_eeprom(0);
            }
        }
    }
}

int main(void)
{
    /* initialize the system */
    system_init();

    /* initialize the gpio */
    gpio_init();

    /* initialize the interupt service routines */
    isr_init();

    /* initialize the i2c interface */
    i2c_init();

    /* initialize the dma controller */
    dma_init();

    /* init led handler */
    led_init();

    /* init lcd display */
    tft_init();

    /* initialize the encoder */
    rencoder_init(0, (EEPROM_SIZE - 32) / 16);

    /* create the tasks specific to this application. */
    xTaskCreate(led_run,      "led",          configMINIMAL_STACK_SIZE,     NULL, 3, NULL);
    xTaskCreate(tft_run,      "tft",          configMINIMAL_STACK_SIZE*2,   NULL, 2, NULL);
    xTaskCreate(dma_run,      "dma",          configMINIMAL_STACK_SIZE*2,   NULL, 2, NULL);
    xTaskCreate(user_handler, "user_handler", configMINIMAL_STACK_SIZE*2,   NULL, 2, NULL);
    xTaskCreate(rencoder_run, "rencoder",     configMINIMAL_STACK_SIZE,     NULL, 2, NULL);

    /* start the scheduler. */
    vTaskStartScheduler();

    /* should never get here ... */
    blink(10);
    return 0;
}
