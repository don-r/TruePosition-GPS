
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "ll.h"
#include "WebServer.h"

#define TAKE_WEBPAGE		if(_webpage_mtx) xSemaphoreTake( _webpage_mtx, portMAX_DELAY)
#define GIVE_WEBPAGE		if(_webpage_mtx) xSemaphoreGive( _webpage_mtx )
								
#define TAG "WEB"
								
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;

char send_data[16000];
int send_bytes = sizeof(send_data);

LinkedList_t *_page_list = NULL;
int _num_pages = 0;
WebPage_t *(*_ws_callback)(char *) = NULL;

SemaphoreHandle_t _webpage_mtx = 0;

void ws_setPageTitle( WebPage_t *page, const char * text ){
TAKE_WEBPAGE;
	strcpy( page->title, "<link rel=\"icon\" href=\"data:;base64,iVBORw0KGgo=\">");
	if (page->refresh > 0) {
		char buf[120];
		sprintf( buf, "<meta http-equiv=\"refresh\" content=\"%d\">",page->refresh);
		strcat( page->title, buf);
	}
	strcat( page->title, "<html><head><title>");
	strcat( page->title, text );
	strcat( page->title, "</title></head><body>" );
GIVE_WEBPAGE;
}

void ws_setPageBody( WebPage_t *page, const char * text ){
TAKE_WEBPAGE;
	strcpy( page->body, text );
GIVE_WEBPAGE;
}

WebPage_t * ws_createWebPage( const char *title, const char *get, int refresh ) {
	WebPage_t *page= (WebPage_t *)malloc(sizeof(WebPage_t));
	
	page->refresh = refresh;
	ws_setPageTitle( page, title );
	strcpy( page->get, get );
	
	if (_page_list == NULL) _page_list = ll_create();
	
	ll_insertAt( _page_list, (void *)page, 0);
	return page;
}

void ws_setInputCallback( WebPage_t *(*func)(char *) ){
	_ws_callback = func;
}

static void _web_server_task(void *p)
{
    int ret = -1;

    int socket, new_socket = -1;
    socklen_t addr_len;
    struct sockaddr_in sock_addr;

    char recv_buf[RECV_BUF_LEN];

    ESP_LOGI(TAG, "server create socket ......");
    socket = socket(AF_INET, SOCK_STREAM, 0);
    if (socket >= 0) {
    	ESP_LOGI(TAG, "OK");
    	ESP_LOGI(TAG, "server socket bind ......");
    	memset(&sock_addr, 0, sizeof(sock_addr));
    	sock_addr.sin_family = AF_INET;
    	sock_addr.sin_addr.s_addr = 0;
    	sock_addr.sin_port = htons(8001);
    	ret = bind(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    	if (ret == 0) {
      		ESP_LOGI(TAG, "OK");
    		ESP_LOGI(TAG, "server socket listen ......");
    		ret = listen(socket, 32);
		}
	}
    if ( ret == 0) {
    	ESP_LOGI(TAG, "OK");

		for(;;) {

//	    	ESP_LOGI(TAG, "server socket accept client ......");
 	  	 	while ((new_socket = accept(socket, (struct sockaddr *)&sock_addr, &addr_len)) < 0);
 //	  	 	ESP_LOGI(TAG, "OK");

  //  ESP_LOGI(TAG, "server read message ......");
			do {
				memset(recv_buf, 0, RECV_BUF_LEN);
				ret = recv(new_socket, recv_buf, RECV_BUF_LEN - 1, 0);
				if (ret <= 0) break;

 //       ESP_LOGI(TAG, "read: %s", recv_buf);

				LinkedListNode_t *pn = _page_list->head;
				WebPage_t *page = NULL;
				if (strstr(recv_buf, "GET /POST") && _ws_callback) page = (*_ws_callback)( recv_buf );
				else {
					while ( pn && !strstr(recv_buf, ((WebPage_t *)(pn->data))->get) ) pn = pn->next;
					if ( pn ) page = (WebPage_t *)(pn->data);		
				}
  
  				if (page) {
TAKE_WEBPAGE;
					sprintf( send_data, PAGE_HEADER, strlen( page->title ) + strlen( page->body ) + strlen( PAGE_FOOTER ) );
					strcat( send_data, page->title );
					strcat( send_data, page->body );
GIVE_WEBPAGE;
					strcat( send_data, PAGE_FOOTER);
				
//ESP_LOGI(TAG,"WRITING: %s\n", send_data);
					ret = send(new_socket, send_data, strlen( send_data),0 );

          	 		if (ret < 0) ESP_LOGI(TAG, "error sending");
    	   		} 
    		} while (1);
    
    		close(new_socket);
    	}	
    }
    if (socket >= 0) close(socket);
ESP_LOGI(TAG, "task closed\n");

    vTaskDelete(NULL);
    return ;
} 

static esp_err_t _wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    	xTaskCreate(_web_server_task,
                      TASK_NAME,
                      TASK_STACK_WORDS,
                      NULL,
                      TASK_PRIORITY,
                      NULL); 
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect(); 
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void ws_init()
{	
	
	_webpage_mtx = xSemaphoreCreateMutex();
	if (_page_list == NULL) _page_list = ll_create();
	
    ESP_ERROR_CHECK( nvs_flash_init() );
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(_wifi_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_auto_connect(true) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]\n", WIFI_SSID, WIFI_PASS);
    ESP_ERROR_CHECK( esp_wifi_start() );
}