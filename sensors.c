/* 
 *
 * This sample code is in the public domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <dht/dht.h>

#include "ds18b20/ds18b20.h"

#include "esp8266.h"

// extern int g_humidity;
// extern int g_temperature;
extern float g_temperature;

/* An example using the ubiquitous DHT** humidity sensors
 * to read and print a new temperature and humidity measurement
 * from a sensor attached to GPIO pin 4.
 */
uint8_t const dht_gpio = 0;
const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;

#if 0
void dhtMeasurementTask(void *pvParameters)
{
    int16_t temperature = 0;
    int16_t humidity = 0;

    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...
    gpio_set_pullup(dht_gpio, false, false);

    while(1) {
        if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature)) {
            printf("Humidity: %d%% Temp: %dC\n", 
                    humidity / 10, 
                    temperature / 10);
            g_humidity = humidity / 10;
            g_temperature = temperature / 10;
        } else {
            printf("Could not read data from sensor\n");
        }

        // Three second delay...
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
#endif


#define MAX_SENSORS 1
#define SENSOR_GPIO 13
#define LOOP_DELAY_MS 1000

#define RESCAN_INTERVAL 1
void ds18b20MeasurementTask(void *pvParameters) {
    ds18b20_addr_t addrs[MAX_SENSORS];
    float temps[MAX_SENSORS];
    int sensor_count;
    
    // There is no special initialization required before using the ds18b20
    // routines.  However, we make sure that the internal pull-up resistor is
    // enabled on the GPIO pin so that one can connect up a sensor without
    // needing an external pull-up (Note: The internal (~47k) pull-ups of the
    // ESP8266 do appear to work, at least for simple setups (one or two sensors
    // connected with short leads), but do not technically meet the pull-up
    // requirements from the DS18B20 datasheet and may not always be reliable.
    // For a real application, a proper 4.7k external pull-up resistor is
    // recommended instead!)

    gpio_set_pullup(SENSOR_GPIO, true, true);

    while(1) {
        // Every RESCAN_INTERVAL samples, check to see if the sensors connected
        // to our bus have changed.
        sensor_count = ds18b20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS);

        if (sensor_count < 1) {
            printf("\nNo sensors detected!\n");
            vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS);
        } else {
            printf("\n%d sensors detected:\n", sensor_count);
            // If there were more sensors found than we have space to handle,
            // just report the first MAX_SENSORS..
            if (sensor_count > MAX_SENSORS) sensor_count = MAX_SENSORS;

            // Do a number of temperature samples, and print the results.
            for (int i = 0; i < RESCAN_INTERVAL; i++) {
                ds18b20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
                for (int j = 0; j < sensor_count; j++) {
                    // The DS18B20 address is a 64-bit integer, but newlib-nano
                    // printf does not support printing 64-bit values, so we
                    // split it up into two 32-bit integers and print them
                    // back-to-back to make it look like one big hex number.
                    uint32_t addr0 = addrs[j] >> 32;
                    uint32_t addr1 = addrs[j];
                    float temp_c = temps[j];
                    float temp_f = (temp_c * 1.8) + 32;
                    printf("  Sensor %08x%08x reports %f deg C (%f deg F)\n", addr0, addr1, temp_c, temp_f);
                    g_temperature = temp_c;
                }
                printf("\n");

                // Wait for a little bit between each sample (note that the
                // ds18b20_measure_and_read_multi operation already takes at
                // least 750ms to run, so this is on top of that delay).
                vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS);
            }
        }
    }
}
#undef MAX_SENSORS
#undef SENSOR_GPIO
#undef LOOP_DELAY_MS
#undef RESCAN_INTERVAL

