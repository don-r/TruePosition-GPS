
// WebServer.h

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ll.h"
#include "sdkconfig.h"

#define MAX_TITLE_LENGTH 256
#define MAX_BODY_LENGTH 4096
#define MAX_GET_LENGTH 128

#define RECV_BUF_LEN       		2048

#define LOCAL_TCP_PORT     		8001

#define WIFI_SSID               CONFIG_WIFI_SSID
#define WIFI_PASS               CONFIG_WIFI_PASSWORD

#define TASK_NAME        		"web_server"
#define TASK_STACK_WORDS		10240
#define TASK_PRIORITY    		8

#define PAGE_HEADER "HTTP/1.1 200 OK\r\n" \
                                "Content-Type: text/html\r\n"\
								"Content-Length: %d\r\n\r\n" 
								
#define PAGE_FOOTER "</body>\r\n" \
								"</html>\r\n"
typedef struct WebPage_t {
	char title[MAX_TITLE_LENGTH];
	char body[MAX_BODY_LENGTH];
	char get[MAX_GET_LENGTH];
	int refresh;
} WebPage_t;

void ws_setPageTitle( WebPage_t *, const char * );
void ws_setPageBody( WebPage_t *, const char * );
WebPage_t * ws_createWebPage( const char *, const char *, int );
void ws_setInputCallback( WebPage_t *(*func)(char *) );
void ws_init();

#endif