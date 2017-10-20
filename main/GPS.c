
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#include "GPS.h"

#define GPS_UART_BUF_SIZE (1024)

typedef struct _gps_event_t {
	char	name[GPS_NAME_SIZE];
	char 	data[GPS_DATA_SIZE];
} _gps_event_t;

QueueHandle_t _gps_rx_q;
QueueHandle_t _gps_tx_q;

TickType_t _gps_q_delay;
	
int gps_sendCommand( const char *command, const char *data ) {
	_gps_event_t event;

// ESP_LOGI(TAG, "gpsSendCommand %s %s\n", command, data);

	strcpy( event.name, command );
	if ( data != NULL ) strcpy( event.data, data );
	return (int) xQueueSend( _gps_tx_q, &event, _gps_q_delay );
	
}

int gps_receiveCommand( char * name, char * data ) {
	_gps_event_t event;

	BaseType_t ret = xQueueReceive(_gps_rx_q, &event, 2000/portTICK_PERIOD_MS );		// 2 second wait watchdog

	strcpy( name, event.name );
	strcpy( data, event.data );
	
	return (int) ret;
}

static void _gps_writer(void *pvParameters) {
	_gps_event_t event;
	char buf[GPS_NAME_SIZE+GPS_DATA_SIZE+1];

ESP_LOGI(TAG, "_gps_writer started\n");

	while (xQueueReceive(_gps_tx_q, &event, portMAX_DELAY)) {

		strcpy( buf, (char *)event.name );
		if (event.data != NULL) strcat( buf, (char *)event.data );
		strcat(buf, "\n");
		uart_write_bytes( GPS_UART_NUM, (const char *)buf, strlen( buf ) );		

		vTaskDelay( 2000 / portTICK_PERIOD_MS);				// Wait 2 seconds between commands
	}
}

static void _gps_reader(void *pvParameters) {
	
ESP_LOGI(TAG, "_gps_reader task created.\n");
	
	_gps_event_t event;
	char *data_ptr;
	char command_delimiter;
	
	for(;;) {
		data_ptr = event.name;
		command_delimiter = '\0';

		while( command_delimiter != '$' ) { 
			uart_read_bytes(GPS_UART_NUM, (unsigned char *)&command_delimiter, 1, portMAX_DELAY); 		// look for start of name
		}
	
		uart_read_bytes(GPS_UART_NUM, (unsigned char *)data_ptr, 1, portMAX_DELAY);			// in case delimiter appears at eol
		while ( *data_ptr != ' ' && *data_ptr != '\n' && *data_ptr != '\r') {
			uart_read_bytes(GPS_UART_NUM, (unsigned char *)++data_ptr, 1, portMAX_DELAY);	// read rest of name
		}
		
		if (*data_ptr == ' ') {												// read data
			*data_ptr = '\0';
			event.data[0] = ' ';
			data_ptr = event.data;
			while ( *data_ptr != '\n' && *data_ptr != '\r' ) {
				uart_read_bytes(GPS_UART_NUM, (unsigned char *)++data_ptr, 1, portMAX_DELAY);
			}
		} else {															// no data to read
			*data_ptr = '\0';												
			data_ptr = event.data;
		}
		*data_ptr = '\0';	
		xQueueSend( _gps_rx_q, &event, _gps_q_delay );
	}
}



void gps_init() {
	
	esp_log_level_set(TAG, ESP_LOG_INFO);
ESP_LOGI(TAG,"gspInit\n");
	
    uart_config_t uart_config = {
       .baud_rate = 9600,
       .data_bits = UART_DATA_8_BITS,
       .parity = UART_PARITY_DISABLE,
       .stop_bits = UART_STOP_BITS_1,
       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
       .rx_flow_ctrl_thresh = 122,
    };
    //Set UART parameters
    uart_param_config(GPS_UART_NUM, &uart_config);
	
    //Install UART driver
    uart_driver_install(GPS_UART_NUM, GPS_UART_BUF_SIZE * 2, GPS_UART_BUF_SIZE * 2, 10, NULL, 0);

    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	
	_gps_rx_q = xQueueCreate( 10, sizeof( _gps_event_t ));
	_gps_tx_q = xQueueCreate( 10, sizeof( _gps_event_t ));
	_gps_q_delay = 100;

	xTaskCreate(_gps_reader, "_gps_reader", 2048, NULL, 10, NULL);
	vTaskDelay( 100/ portTICK_PERIOD_MS );								// crashes if no delay... for some reason
	xTaskCreate(_gps_writer, "_gps_writer", 2048, NULL, 12, NULL);
}
