/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include <math.h>
#include "esp_system.h"
#include "unistd.h"
#include "nvs_flash.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "crcTable.h"
static const char *TAG = "MQTT_EXAMPLE";
// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.
#define ECHO_TEST_TXD (CONFIG_ECHO_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_ECHO_UART_RXD)

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define ECHO_TEST_RTS (CONFIG_ECHO_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)
#define BAUD_RATE (CONFIG_ECHO_UART_BAUD_RATE)

// Read packet timeout
#define PACKET_READ_TICS (200 / portTICK_RATE_MS)
#define ECHO_TASK_STACK_SIZE (4095)
#define ECHO_TASK_PRIO (10)
#define ECHO_UART_PORT (CONFIG_ECHO_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
#define ECHO_READ_TOUT (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

uint8_t display1[8] = {0x01, 0x04, 0x22, 0x02, 0, 2}; //={0x01,0x03,0xA0,4};0x22,0x04
uint8_t display2[8] = {0x01, 0x04, 0x22, 0xC0, 0, 2}; //={1,3,0xA0,2};
static esp_mqtt_client_handle_t client = NULL;
// static EventGroupHandle_t mqtt_event_group;
// const static int CONNECTED_BIT = BIT0;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //   xEventGroupSetBits(mqtt_event_group, CONNECTED_BIT);
        // rockwool 4 m
      //  msg_id = esp_mqtt_client_subscribe(client, "channels/274053/subscribe/fields/field1", 0);
      //  msg_id = esp_mqtt_client_subscribe(client, "channels/274053/subscribe/fields/field2", 0);

        // 1 m batch
        // msg_id = esp_mqtt_client_subscribe(client, "channels/1833642/subscribe/fields/field2", 0);
        // msg_id = esp_mqtt_client_subscribe(client, "channels/1833642/subscribe/fields/field2", 0);
        // 2Mhz flow meter 1846253
          msg_id = esp_mqtt_client_subscribe(client, "channels/1846253/subscribe/fields/field2", 0);
           msg_id = esp_mqtt_client_subscribe(client, "channels/1846253/subscribe/fields/field1", 0);

        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .port = 1883,
        // rockwool 4 m
      //  .client_id = "ADUXLhA8CTQDLjQNGzsROR4",
      //  .username = "ADUXLhA8CTQDLjQNGzsROR4",
      //  .password = "ZPNSyZl7dImxg9AzHUxx9g+9"
        // ADUXLhA8CTQDLjQNGzsROR4
        //  1m batch flowmeter 1Mhz batch and trails
        /*.client_id = "OwUtCTIRFxMwIhUzLAo0AzM",
        .username = "OwUtCTIRFxMwIhUzLAo0AzM",
        .password = "UtG+AD1hP0W3GteuM1WtOfXi" */
        // 1.5 m batch flowmeter 2 MHz
           .client_id ="FR4QFDYJMjEpIxMkITgNMTk",
           .username ="FR4QFDYJMjEpIxMkITgNMTk",
            .password ="W+PpckUT4vIR5wF6tukdkxxf" 
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0)
    {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128)
        {
            int c = fgetc(stdin);
            if (c == '\n')
            {
                line[count] = '\0';
                break;
            }
            else if (c > 0 && c < 127)
            {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    }
    else
    {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    client = client;
}

static void echo_send(const int port, const uint8_t *str, uint8_t length)
{
    if (uart_write_bytes(port, str, length) != length)
    {
        ESP_LOGE(TAG, "Send data critical failure.");
        // add your code to handle sending failure here
        abort();
    }
    for (int i = 0; i < length; i++)
        ESP_LOGE(TAG, "%X\n", str[i]);
}

unsigned short CRC16(puchMsg, usDataLen) /* The function returns the CRC as a unsigned short type */
unsigned char *puchMsg;                  /* message to calculate CRC upon */
unsigned short usDataLen;                /* quantity of bytes in message */
{
    unsigned char uchCRCHi = 0xFF; /* high byte of CRC initialized */
    unsigned char uchCRCLo = 0xFF; /* low byte of CRC initialized */
    unsigned uIndex;               /* will index into CRC lookup table */
    while (usDataLen--)            /* pass through message buffer */
    {
        uIndex = uchCRCLo ^ *puchMsg++; /* calculate the CRC */
        uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
        uchCRCHi = auchCRCLo[uIndex];
    }
    return (uchCRCHi << 8 | uchCRCLo);
}
// function is from https://www.technical-recipes.com/2012/converting-between-binary-and-decimal-representations-of-ieee-754-floating-point-numbers-in-c/
double Binary32ToDouble(int32_t value)
{
    int minus = -1, exponent;
    double fraction, result;

    if ((value & 0x80000000) == 0)
        minus = 1;
    exponent = ((value & 0x7F800000) >> 23) - 127;
    fraction = (value & 0x7FFFFF) + 0x800000;
    fraction = fraction / 0x800000;
    result = minus * fraction * pow(2, exponent);
    return (result);
}
// An example of echo test with hardware flow control on UART
static void echo_task(void *arg)
{
    const int uart_num = ECHO_UART_PORT;
    gpio_config_t io_conf = {};
    // char togle=0;
    int len = 0;

    uint32_t raw = 0;
    uint16_t CRC = 0;
    union
    {
        uint32_t b;
        float f;
    } u;

    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART_HW_FLOWCTRL_RTS,
        .rx_flow_ctrl_thresh = 122,            // 122,
        .source_clk = UART_SCLK_APB,
    };
    // io_conf.mode = GPIO_MODE_OUTPUT;
    //  io_conf.pin_bit_mask = CONFIG_ECHO_UART_RTS;
    //   gpio_config(&io_conf);
    gpio_set_direction(CONFIG_ECHO_UART_RTS, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_ECHO_UART_RTS, 1);
    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Start RS485 application test and configure UART.");

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings
    ESP_ERROR_CHECK(uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Set RS485 half duplex mode
    ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, ECHO_READ_TOUT));
    CRC = CRC16(display1, 6);
    display1[6] = 0x00FF & CRC;
    display1[7] = CRC >> 8;
    uint16_t rec_CRC = 0;
    CRC = CRC16(display2, 6);
    display2[6] = 0x00FF & CRC;
    display2[7] = CRC >> 8;
    double res1 = 0, res2 = 0, new1 = 0, new2 = 0, old2 = 0;
    int i = 0, n = 0, k = 0;
    // Allocate buffers for UART
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    uint8_t *data1 = (uint8_t *)malloc(BUF_SIZE);
    ESP_LOGI(TAG, "UART start recieve loop.\r\n");
    // echo_send(uart_num, "Start RS485 UART test.\r\n", 24);
    char payl[80] = {0};
    int msg_id = 0;
    while (1)
    {
        // Read data from UART
        // gpio_set_level(CONFIG_ECHO_UART_RTS, 0);
        if (i < 6)
        {
            i++;
            gpio_set_level(CONFIG_ECHO_UART_RTS, 1);
            // uart_write_bytes(uart_num,display1,6);
            echo_send(uart_num, display1, 8);
            // for(int i=0;i<6;i++)   ESP_LOGI(TAG,"trans :%X ",display1[i]);

            gpio_set_level(CONFIG_ECHO_UART_RTS, 0);
            usleep(150000);
            // ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 20));

            len = uart_read_bytes(uart_num, data, BUF_SIZE, PACKET_READ_TICS);
            ESP_LOGI(TAG, "legth 1 %d", len);
            if (len > 0)
            {
                ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
                for (int i = 0; i < len; i++)
                    ESP_LOGI(TAG, "data 1 hi\n %X", data[i]);
                rec_CRC = (data[8] << 8) + data[7];
                ESP_LOGI(TAG, "modtaget CRC \n %X", rec_CRC);
                CRC = CRC16(data, 7);
                ESP_LOGI(TAG, "calculated CRC \n %X", CRC);
                raw = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + data[6];
                ESP_LOGI(TAG, "raw display1 %X\n", raw);

                u.b = raw;
                new1 = Binary32ToDouble(raw);
                printf("flow value %.4f", new1);
                if (rec_CRC == CRC)
                {
                   // if (new1 > 0)
                  //  {
                        printf("flow l/s %g\n", u.f);
                        res1 = new1 + res1;
                        n++;
                   // }
                }
            }
            usleep(150000);

            gpio_set_level(CONFIG_ECHO_UART_RTS, 1);
            //  uart_write_bytes(uart_num,display2,6);
            echo_send(uart_num, display2, 8);
            gpio_set_level(CONFIG_ECHO_UART_RTS, 0);
            usleep(150000);

            // ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 20));

            len = uart_read_bytes(uart_num, data1, BUF_SIZE, PACKET_READ_TICS);
            ESP_LOGI(TAG, "legth 2 %d", len);

            // Write data back to UART
            if (len > 0)
            {
                // gpio_set_level(CONFIG_ECHO_UART_RTS, 1);

                ESP_LOGI(TAG, "Received %u bytes:", len);
                ESP_LOG_BUFFER_HEXDUMP(TAG, data1, len, ESP_LOG_INFO);
                raw = (data1[3] << 24) + (data1[4] << 16) + (data1[5] << 8) + data1[6];
                ESP_LOGI(TAG, "raw display2 %X\n", raw);
                rec_CRC = (data1[8] << 8) + data1[7];
                ESP_LOGI(TAG, "modtaget CRC \n %X", rec_CRC);
                CRC = CRC16(data1, 7);
                ESP_LOGI(TAG, "calculated CRC \n %X", CRC);

                u.b = raw;
                new2 = Binary32ToDouble(raw);
                if (rec_CRC == CRC)
                {
                    if (new2 > 0)
                    {
                        if ( (new2 < 1500)){
                            new2 = old2;
                        res2 = new2 + res2;
                        k++;
                        printf("Soundspeed m/s %g\n", u.f);
                        old2 = new2;
                        }
                    }
                }
                for (int i = 1; i < len; i++)
                    ESP_LOGI(TAG, "%X", data1[i]);
            }
        }
        else if ((i = 6) && (n > 0) && (k > 0))

        {
            esp_mqtt_client_start(client);
            // xEventGroupWaitBits(mqtt_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
            i = 0;
            res1 = res1 / n;
            printf("flow l/s %f\n", res1);
            res2 = res2 / k;
            printf("speed %f\n", res2);
            //  if((res1<0)||(res1>200))res1=0;
            sprintf(payl, "field1=%f&field2=%f&status=MQTTPUBLISH", res1, res2);
            // rockwool flowmeter 4m  274053
         //   msg_id = esp_mqtt_client_publish(client, "channels/274053/publish", payl, 0, 0, 0);

            // 1 MHz flow meter
            //  msg_id = esp_mqtt_client_publish(client, "channels/1833642/publish", payl, 0, 0, 0);
            // 2 Mhz flow meter
            // 1846253   1846253
             msg_id = esp_mqtt_client_publish(client, "channels/1846253/publish", payl, 0, 0, 0);

            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            k = 0;
            n = 0;
            res1 = 0;
            res2 = 0;
            esp_mqtt_client_stop(client);
        }
        else
            i = 0;

        vTaskDelay(pdMS_TO_TICKS(6625)); // 60 sekunder
    }
    free(data);
    free(data1);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, ECHO_TASK_PRIO, NULL);
}
