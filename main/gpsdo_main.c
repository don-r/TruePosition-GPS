
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "GPS.h"

#include "gpsdo.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"

#include "ll.h"
#include "WebServer.h"

typedef struct gps_resp_t {
	char name[25];
	char data[128];
	time_t	time;
} gps_resp_t;

#define MAX_RESP_LIST_LENGTH 10

LinkedList_t *resp_list;
	
WebPage_t *info_page;
WebPage_t *satellite_page;
WebPage_t *cmd_page;

typedef struct GPSSatellite_t {
	int prn;
	int channel;
	int elevation;
	int azimuth;
	int signal_strength;
	time_t last_seen;
} GPSSatellite_t;

int _gps_10MHz_bad, _gps_1PPS_bad, _gps_ant_fault, _gps_holdover_sec, _gps_n_sat, _gps_state;
int _gps_previous_state;
time_t _gps_abs_time;
int _gps_leap_sec, _gps_TFOM;
double _gps_latitude, _gps_longtitude;
int _gps_elevation;
int _gps_elev_correction, _gps_survey_time;
char _gps_lock_acquired[25], _gps_lock_lost[25];
bool _gps_ok = false;

LinkedList_t *_gps_sat_l;

char _status_body[1024];
char _time_body[256];
char _position_body[256];
char _survey_body[256];
char _sat_list_body[4096];
char _response_list_body[4096];
char _web_page_body[12000];

void resetGPSData(){

	_gps_ok				= false;
	
	_gps_10MHz_bad 		= 1;		// assume bad until gps tells us otherwise
	_gps_1PPS_bad 		= 1;
	_gps_ant_fault 		= 1;
	_gps_holdover_sec 	= 0;
	_gps_n_sat 			= 0;
	_gps_state			= 6;		// no GPS until told otherwise
	_gps_previous_state = 6;
	
	_gps_abs_time		= 0LL;
	_gps_leap_sec		= 0;
	_gps_TFOM			= 7;		// assume worst TFOM until told otherwise
	
	strcpy( _gps_lock_acquired, ctime(&_gps_abs_time));
	strcpy( _gps_lock_lost, ctime(&_gps_abs_time));
}

void parse_GETVER( const char * data ) {
	if (data != NULL && strstr(data,"BOOT")) { 
		char buf[30];
		gps_sendCommand( "$PROCEED", "" );
		sprintf( buf, " %ld %ld %d", \
			(long)(atof( GPS_LATITUDE ) * 1000000L), \
			(long)(atof( GPS_LONGTITUDE ) * 1000000L), \
			atoi( GPS_ELEVATION ) );
		gps_sendCommand( "$SETPOS", buf );
		gps_sendCommand( "$GETPOS", "" );	
	}
} 

int _prn_compare( void *prn, void *data ) {
	if (data != NULL && prn != NULL) return *((int *)prn) - ((GPSSatellite_t *)data)->prn;
	else return -1;
}

WebPage_t * webInputHandler( char * recv_buf ) {
	char *buf = strstr(recv_buf, "command=");
	char command[32];
	char data[128];

ESP_LOGI(TAG,"in handlr: %s\n", recv_buf);
	if (buf != NULL) {
		buf += 8;
		strcpy( command, "$" );
		strcat( command, strtok( buf, "&"));
		strtok( NULL, "=");
		strcpy( data, strtok( NULL, "= "));
	
		gps_sendCommand( command, data);	
	}

ESP_LOGI(TAG,"in handlr, returning page: %s\n", cmd_page->title );
	return cmd_page;
}

void parse_STATUS( const char *data ){

	if (data != NULL) {
		sscanf( data, "%d %d %d %d %d %d", \
			&_gps_10MHz_bad, &_gps_1PPS_bad, &_gps_ant_fault, &_gps_holdover_sec, &_gps_n_sat, &_gps_state );
	}

	if (_gps_state == 0 && _gps_previous_state != 0) {
		strcpy( _gps_lock_acquired, ctime( &_gps_abs_time ));
	} else if (_gps_state != 0 && _gps_previous_state == 0) {
		strcpy( _gps_lock_lost, ctime( &_gps_abs_time ));
	}
	_gps_previous_state = _gps_state;
	
	sprintf( _status_body, INFO_PAGE_STATUS_BODY, 
			_gps_10MHz_bad ? "red" : "lime",
			_gps_1PPS_bad ? "red" : "lime",
			_gps_ant_fault ? "red" : "lime",
			_gps_holdover_sec ? "yellow" : "lime", _gps_holdover_sec ,
			_gps_n_sat ? (_gps_n_sat < 3 ? "yellow" : "lime") : "red", _gps_n_sat,
			_gps_state ? "yellow" : "lime", _gps_state,
			_gps_ok ? "lime" : "red",
			_gps_lock_acquired,
			_gps_lock_lost );
}

void parse_POS( const char *data ) {
	long int latitude, longtitude;	
	if (data != NULL ) {
		sscanf( data, "%ld %ld %d", &latitude, &longtitude, &_gps_elevation );
		_gps_latitude = latitude / 1000000.0;
		_gps_longtitude = longtitude / 1000000.0;
	}
	
	sprintf( _position_body, INFO_PAGE_POSITION_BODY, 
			_gps_latitude, _gps_longtitude, _gps_elevation);
}

void parse_SURVEY( const char *data ) {

	long int latitude, longtitude;	
	if (data != NULL ) {
		sscanf( data, "%ld %ld %d %d %d", &latitude, &longtitude, &_gps_elevation, &_gps_elev_correction, &_gps_survey_time );
		_gps_latitude = latitude / 1000000.0;
		_gps_longtitude = longtitude / 1000000.0;
	}
	
	parse_POS( NULL );
	
	sprintf( _survey_body, INFO_PAGE_SURVEY_BODY, 
			_gps_survey_time ? "yellow" : "lime", _gps_survey_time/3600, (_gps_survey_time%3600)/60 );
}

void parse_CLOCK( const char *data ){
	
	long int gps_time;
	
	if (data != NULL) {
		sscanf( data, "%ld %d %d", &gps_time, &_gps_leap_sec, &_gps_TFOM );
		_gps_abs_time = (time_t)( (long long)gps_time + 315964835LL);	// fudge factor
	}
	
	time_t cur_time = (long int)_gps_leap_sec + _gps_abs_time;

	sprintf( _time_body, INFO_PAGE_TIME_BODY,
			ctime( &cur_time ), _gps_TFOM < 6 ? (_gps_TFOM < 4 ? "lime" : "yellow") : "red", _gps_TFOM);
}

void parse_SAT( const char *data ) {
	char row[256];
	int chnl, prn, elev, azim, sig;
	
	if (data != NULL) {
		sscanf(data,"%d %d %d %d %d", &chnl, &prn, &elev, &azim, &sig);
	
		GPSSatellite_t *sat = ll_find( _gps_sat_l, &prn, _prn_compare );
	
		if (sat == NULL) {
			sat = (GPSSatellite_t *)malloc(sizeof(GPSSatellite_t));	
			sat->prn = prn;
			ll_insertAt( _gps_sat_l, sat, 0 );
		}
		
		sat->channel = chnl;
		sat->elevation = elev;
		sat->azimuth = azim;
		sat->signal_strength = sig;
		sat->last_seen = _gps_abs_time;
	}
		
	strcpy( _sat_list_body, SATELLITE_LIST_BODY );
	LinkedListNode_t *sn = _gps_sat_l->head;
	for (int i = 0; sn != NULL; i++) {
		GPSSatellite_t *sat = (GPSSatellite_t *)sn->data;
		sprintf( row, "<tr><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%.24s</td></tr>", \
				sat->prn, sat->channel, sat->elevation, sat->azimuth, sat->signal_strength, ctime( &(sat->last_seen) ) );
		sn = (LinkedListNode_t *)sn->next;
		strcat( _sat_list_body, row);
	}
	strcat( _sat_list_body, "</table></p>");
	
	ws_setPageBody( satellite_page, _sat_list_body );	
}
			 
void parse_OTHER( const char * name, const char * data) {
	char row[256];

	if (name) {
		gps_resp_t *resp = (gps_resp_t *) malloc(sizeof(gps_resp_t));
		strcpy( resp->name, name );
		strcpy( resp->data, data );
		resp->time = _gps_abs_time;
		if (ll_insertAt( resp_list, resp, 0) > MAX_RESP_LIST_LENGTH) ll_removeAt( resp_list, -1);
	}
	
	strcpy( _response_list_body, "<table border=1><tr><th>Time</th><th>Response</th><th>Data</th></tr>");
	LinkedListNode_t *rn = resp_list->head;
	for (int i = 0; rn != NULL; i++) {
		gps_resp_t *resp = (gps_resp_t *)rn->data;
		sprintf( row, "<tr><td>%.24s</td><td>%s</td><td>%s</td></tr>", \
				ctime( &(resp->time) ), resp->name, resp->data);
		rn = (LinkedListNode_t *)rn->next;
		strcat( _response_list_body, row);
	}
	strcat( _response_list_body, "</table>");
}

void updateInfoPage() {

	strcpy( _web_page_body, _status_body );
	strcat( _web_page_body, _time_body );
	strcat( _web_page_body, _position_body );
	strcat( _web_page_body, _survey_body );
	strcat( _web_page_body, _response_list_body );
					
	ws_setPageBody( info_page, _web_page_body );
}

void app_main(void)
{

	char name[GPS_NAME_SIZE];
	char data[GPS_DATA_SIZE];
	int ret;
	
	resp_list = ll_create();
	_gps_sat_l = ll_create();
	
	info_page = ws_createWebPage( "GPSDO - Info", "GET / HTTP/1.1", 5 );
	satellite_page = ws_createWebPage( "GPSDO - Satellite List", "GET /satellites HTTP/1.1", 30 );
	cmd_page = ws_createWebPage( "GPSDO - Command", "GET /command", 0 );

	ws_setPageBody( cmd_page, COMMAND_PAGE_BODY );
	ws_setPageBody( info_page, "<p>GPS initializing...</p>" );
	
	ws_setInputCallback( webInputHandler );
    ws_init();
	
	gpio_set_direction( LED_PIN, GPIO_MODE_OUTPUT );
	
	resetGPSData();
	
	vTaskDelay( 3000/ portTICK_PERIOD_MS);		// wait for 3 seconds for the webserver to spin up.
	
	gps_init();
	
	parse_CLOCK( NULL );
	parse_STATUS( NULL );
	parse_SAT( NULL );
	parse_SURVEY( NULL );
	parse_POS( NULL );
	parse_GETVER( NULL );
	parse_OTHER( NULL, NULL );
	
	updateInfoPage();
	
	for (;;) {
		ret = gps_receiveCommand( name, data );					
		
		if (ret) {
			_gps_ok = true;
			gpio_set_level( LED_PIN, 1);			// flash LED
			vTaskDelay( 50/ portTICK_PERIOD_MS);
			gpio_set_level( LED_PIN, 0);
			
			if (!strcmp(name, "CLOCK")) parse_CLOCK( data );
			else if (!strcmp(name, "STATUS") || !strcmp(name, "EXTSTATUS")) parse_STATUS( data );
			else if (!strcmp(name, "SAT") || !strcmp(name, "WSAT")) parse_SAT( data );
			else if (!strcmp(name, "SURVEY")) parse_SURVEY( data );
			else if (!strcmp(name, "GETPOS")) parse_POS( data );
			else if (!strcmp(name, "GETVER")) parse_GETVER( data );
			else parse_OTHER( name, data );
		} else resetGPSData();
		updateInfoPage();	
	}
}
