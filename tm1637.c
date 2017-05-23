// converted from https://github.com/avishorp/TM1637/
// to a more C style

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp/gpio.h"
#include <espressif/esp_misc.h> // sdk_os_delay_us
// #include <i2c/i2c.h>

#include "tm1637.h"

#define DELAY_BIT 50

#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80

#define bit_delay() do { sdk_os_delay_us(DELAY_BIT); } while (0)

/** 
*      A
*     ---
*  F |   | B
*     -G-
*  E |   | C
*     ---
*      D
*/
#define SEG_A   0b00000001
#define SEG_B   0b00000010
#define SEG_C   0b00000100
#define SEG_D   0b00001000
#define SEG_E   0b00010000
#define SEG_F   0b00100000
#define SEG_G   0b01000000

#define TM1637_COLON_BIT 0b10000000


const uint8_t digit_to_segment[] = {
    // XGFEDCBA
    0b00111111,    // 0
    0b00000110,    // 1
    0b01011011,    // 2
    0b01001111,    // 3
    0b01100110,    // 4
    0b01101101,    // 5
    0b01111101,    // 6
    0b00000111,    // 7
    0b01111111,    // 8
    0b01101111,    // 9
    0b01110111,    // A
    0b01111100,    // b
    0b00111001,    // C
    0b01011110,    // d
    0b01111001,    // E
    0b01110001     // F
};

/*********************************************************************
* private functions
*********************************************************************/
static void start(const tm1637_t *disp)
{
  gpio_enable(disp->pin_data, GPIO_OUTPUT);
  bit_delay();
}

static void stop(const tm1637_t *disp)
{
    gpio_enable(disp->pin_data, GPIO_OUTPUT);
    bit_delay();
    gpio_enable(disp->pin_clock, GPIO_INPUT);
    bit_delay();
    gpio_enable(disp->pin_data, GPIO_INPUT);
    bit_delay();
}

bool write_byte(const tm1637_t *disp, uint8_t b)
{
    uint8_t data = b;

    // 8 Data Bits
    for(uint8_t i = 0; i < 8; i++) {
        // CLK low
        gpio_enable(disp->pin_clock, GPIO_OUTPUT);
        bit_delay();

        // Set data bit
        if (data & 0x01) {
            gpio_enable(disp->pin_data, GPIO_INPUT);
        } else {
            gpio_enable(disp->pin_data, GPIO_OUTPUT);
        }

        bit_delay();

        // CLK high
        gpio_enable(disp->pin_clock, GPIO_INPUT);
        bit_delay();
        data = data >> 1;
    }

    // Wait for acknowledge
    // CLK to zero
    gpio_enable(disp->pin_clock, GPIO_OUTPUT);
    gpio_enable(disp->pin_data, GPIO_INPUT);
    bit_delay();

    // CLK to high
    gpio_enable(disp->pin_clock, GPIO_INPUT);
    bit_delay();
    uint8_t ack = gpio_read(disp->pin_data);

    if (ack == 0) {
        gpio_enable(disp->pin_data, GPIO_OUTPUT);
    }

    bit_delay();
    gpio_enable(disp->pin_clock, GPIO_OUTPUT);
    bit_delay();

    return ack;
}


/*********************************************************************
* public functions 
*********************************************************************/
void tm1637_init(const tm1637_t *disp)
{ 
    gpio_enable(disp->pin_clock, GPIO_INPUT);
    gpio_enable(disp->pin_data,  GPIO_INPUT);

    gpio_write(disp->pin_clock, 0);
    gpio_write(disp->pin_data, 0);
}

void tm1637_set_brightness(tm1637_t *disp, uint8_t brightness, bool on)
{
    disp->brightness = (brightness & 0x7) | (on? 0x08 : 0x00);
    disp->backlight = on;
}

void tm1637_set_colon(tm1637_t *disp, bool on)
{
    disp->colon = on;
}

void tm1637_set_segments(const tm1637_t *disp, uint8_t segments[], uint8_t length, uint8_t pos)
{
    // Write COMM1
    start(disp);
    write_byte(disp, TM1637_I2C_COMM1);
    stop(disp);

    // Write COMM2 + first digit address
    start(disp);
    write_byte(disp, TM1637_I2C_COMM2 + (pos & 0x03));

    if (length == 4 && pos == 0 && disp->colon) {
        segments[1] |= TM1637_COLON_BIT;
    }

    // Write the data bytes
    for (uint8_t k=0; k < length; k++) {
      write_byte(disp, segments[k]);
    }

    stop(disp);

    // Write COMM3 + brightness
    start(disp);
    write_byte(disp, TM1637_I2C_COMM3 + (disp->brightness & 0x0f));
    stop(disp);
}

uint8_t tm1637_encode_digit(uint8_t digit)
{
    return digit_to_segment[digit & 0x0f];
}

void tm1637_clear(const tm1637_t *disp)
{
    uint8_t off[] = {0,0,0,0};
    tm1637_set_segments(disp, off, 4, 0);
}

void tm1637_show_number_dec(const tm1637_t *disp, int num, uint8_t pos)
{
  tm1637_show_number_dec_ex(disp, num, 0, 0, 4, pos);
}

void tm1637_show_number_dec_ex(const tm1637_t *disp, int num, uint8_t dots, bool leading_zero,
                                    uint8_t length, uint8_t pos)
{
    uint8_t digits[4];
    const static int divisors[] = { 1, 10, 100, 1000 };
    bool leading = true;

    for(int8_t k = 0; k < 4; k++) {
        int divisor = divisors[4 - 1 - k];
        int d = num / divisor;
        uint8_t digit = 0;

        if (d == 0) {
            if (leading_zero || !leading || (k == 3)) {
              digit = tm1637_encode_digit(d);
            } else {
              digit = 0;
            }
        } else {
            digit = tm1637_encode_digit(d);
            num -= d * divisor;
            leading = false;
        }
        
        // Add the decimal point/colon to the digit
        digit |= (dots & 0x80); 
        dots <<= 1;
        
        digits[k] = digit;
    }

    tm1637_set_segments(disp, digits + (4 - length), length, pos);
}