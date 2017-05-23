/* http_get - Retrieves a web page over HTTP GET.
 *
 * See http_get_ssl for a TLS-enabled version.
 *
 * This sample code is in the public domain.,
 */
#include <stdbool.h>
#include <stdint.h>

#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "tm1637.h" // local

#include "ssid_config.h"

#define WEB_SERVER "192.168.1.120"
#define WEB_PORT  "7777"
#define WEB_URL "/"

#define TASK_SHOW_TEMP_DELAY 5000
#define TASK_LED_PRIORITY 3
#define TASK_HTTP_PRIORITY 2


// globals
// int g_humidity;
// int g_temperature;
float g_temperature;

void ds18b20MeasurementTask(void *pvParameters);

void show_temp(void *pvParameters)
{
    tm1637_t disp = {
        .pin_clock = 4,
        .pin_data = 5
    };
    tm1637_init(&disp);
    tm1637_set_brightness(&disp, 0x2, true);
    tm1637_clear(&disp);

    for(;;) {
        tm1637_clear(&disp);
        tm1637_show_number_dec_ex(&disp, (g_temperature*100)/100, 0, 0, 2, 0);
        vTaskDelay(TASK_SHOW_TEMP_DELAY / portTICK_PERIOD_MS);

        tm1637_clear(&disp);
        tm1637_show_number_dec_ex(&disp, (g_temperature*100)/100, 0, 0, 2, 1);
        vTaskDelay(TASK_SHOW_TEMP_DELAY / portTICK_PERIOD_MS);

        tm1637_clear(&disp);
        tm1637_show_number_dec_ex(&disp, (g_temperature*100)/100, 0, 0, 2, 2);
        vTaskDelay(TASK_SHOW_TEMP_DELAY / portTICK_PERIOD_MS);

        tm1637_clear(&disp);
        tm1637_show_number_dec_ex(&disp, (g_temperature*100)/100, 0, 0, 2, 1);
        vTaskDelay(TASK_SHOW_TEMP_DELAY / portTICK_PERIOD_MS);
    }
}



void http_get_task(void *pvParameters)
{
    int successes = 0, failures = 0;


    printf("HTTP get task starting...\r\n");

    while(1) {
        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

        printf("Running DNS lookup for %s...\r\n", WEB_SERVER);
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
            if(res)
                freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            failures++;
            continue;
        }
        /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

        int s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            printf("... Failed to allocate socket.\r\n");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            failures++;
            continue;
        }

        printf("... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            printf("... socket connect failed.\r\n");
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            failures++;
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);

        const char *req =
            "GET "WEB_URL"\r\n"
            "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
            "\r\n";
        if (write(s, req, strlen(req)) < 0) {
            printf("... socket send failed\r\n");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            failures++;
            continue;
        }
        printf("... socket send success\r\n");

        static char recv_buf[128];
        int r;
        do {
            bzero(recv_buf, 128);
            r = read(s, recv_buf, 127);
            if(r > 0) {
                printf("%s", recv_buf);
            }
        } while(r > 0);

        printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        if(r != 0) {
            failures++;
        } else {
            successes++;
        }
        close(s);

        printf("successes = %d failures = %d\r\n", successes, failures);

        for(int countdown = 10; countdown >= 0; countdown--) {
            printf("%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        printf("\r\nStarting again!\r\n");
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    // xTaskCreate(&http_get_task,      "get_task",           256, NULL, TASK_HTTP_PRIORITY, NULL);
    xTaskCreate(&show_temp,          "show_temp",          128, NULL, TASK_LED_PRIORITY,  NULL);
    xTaskCreate(&ds18b20MeasurementTask, "ds18b20MeasurementTask", 256, NULL, TASK_HTTP_PRIORITY, NULL);

}

