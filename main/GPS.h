
// GPS.h

#ifndef GPS_H
#define GPS_H

#include <time.h>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "soc/uart_struct.h"
#include "ll.h"

// defines
#define GPS_UART_NUM UART_NUM_1
#define GPS_TX_PIN	GPIO_NUM_13
#define GPS_RX_PIN	GPIO_NUM_12
#define LED_PIN GPIO_NUM_2

#define GPS_DATA_SIZE (256)
#define GPS_NAME_SIZE (16)

// consts


#define TAG "GPS"

int gps_sendCommand( const char *command, const char *data );

int gps_receiveCommand( char *name, char *data );

void gps_init();


#endif