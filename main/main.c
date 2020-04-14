#include <stdio.h>


#include <esp_wifi.h>
//#include <esp_event_loop.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_ota_ops.h>

#include "http_serv.h"

#include "config.h"
#include "mqtt.h"
#include "rtc_sntp.h"
#include "comm_uart.h"
#include "telnet.h"
#include "comm_queue.h"
//#include "board_gpio.h"
#include "sled.h"
#include "net_camera.h"


static const char *TAG = "main.c";




//static esp_err_t event_handler(void *ctx, system_event_t *event)
static void wifi_event_handler(void* ctx, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  httpd_handle_t *server = (httpd_handle_t *)ctx;

  if(event_base==WIFI_EVENT) {
  		  switch (event_id)
		  {
		  case WIFI_EVENT_STA_STOP:
		    ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
		    mqtt_setlower_state(STATE_RESET);
			break;
		  case WIFI_EVENT_STA_START:
			//set_sled_state(SLED_SLOW_BLINK);
			mqtt_sethigher_state(STATE_WIFI_CONNECTING);
			ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
			ESP_ERROR_CHECK(esp_wifi_connect());
			
			//http_serv_start();
			
			break;
		  case WIFI_EVENT_STA_DISCONNECTED:
		    mqtt_setlower_state(STATE_WIFI_CONNECTING);
			ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
			
			if (mqtt_get_state()==STATE_WIFI_CONNECTING) ESP_ERROR_CHECK(esp_wifi_connect());

			/* Stop the web server */
			
			if (*server)
			{
			  stop_webserver(*server);
			  *server = NULL;
			}
			
			
			//http_serv_stop();
			break;
		  case WIFI_EVENT_AP_START:
		    if (*server)
			{
			  stop_webserver(*server);
			  *server = NULL;
			}
			
		    http_serv_start();
		  default:
			break;
		  }
  }	else if (event_base==IP_EVENT) {
	  switch (event_id)
		{
			case IP_EVENT_STA_GOT_IP:
				mqtt_sethigher_state(STATE_WIFI_CONNECTED);
				ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
				
				ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
				ESP_LOGI(TAG, "Got IP:%s",
						 ip4addr_ntoa(&event->ip_info.ip));
				
				/* Start the web server */
				
				if (*server == NULL)
				{
				  *server = start_webserver();
				}
				
								
				break;
			default:
			break;
		}
  }
}

static void wifi_init(void *arg)
{
  tcpip_adapter_init();
  //ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
  //ESP_ERROR_CHECK(esp_event_init(event_handler, arg));
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  //ESP_ERROR_CHECK(esp_event_handler_register(ESP_EVENT_ANY_BASE , ESP_EVENT_ANY_ID , default_event, arg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, arg));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, arg));
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config;
  bzero(&wifi_config, sizeof(wifi_config_t));
  memcpy(wifi_config.sta.ssid, sysCfg.sta_ssid, sizeof(wifi_config.sta.ssid));
  memcpy(wifi_config.sta.password, sysCfg.sta_pwd, sizeof(wifi_config.sta.password));

  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}

static void wifi_start(void *arg)
{
	ESP_ERROR_CHECK(esp_wifi_start());
	
	char hostname[64];
    uint8_t chipid[6];
    char s_chipid[13];
	
    esp_efuse_mac_get_default(chipid);
    sprintf(s_chipid,"%02X%02X%02X", chipid[3], chipid[4], chipid[5]);
    sprintf(hostname,"ESP32-%s-%s",s_chipid,sysCfg.mqtt_topic);
    ESP_LOGI(TAG, "Setting hostname %s...", hostname);
    ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname));
}

void app_main()
{
  static httpd_handle_t server = NULL;
  
  CFG_Init();
  ESP_LOGI(TAG,"CFG initialized");
  CFG_Load();
  
  ESP_LOGI(TAG,"SSID: %s",sysCfg.sta_ssid);
  
  {
    printf("Project %s (Topic %s, Fallback %s, GroupTopic %s) Version %s\n",
    esp_ota_get_app_description()->project_name, sysCfg.mqtt_topic, sysCfg.device_id, sysCfg.mqtt_grptopic, esp_ota_get_app_description()->version);
  }
  
  comm_queue_init();
  
  
  wifi_init(&server);
  mqtt_init();
  
  init_camera();
  wifi_start(&server);
  
  //mqtt_start();

  rtc_sntp_init(sysCfg.timezone);
  comm_uart_init();
  telnetInit(23);
  
  

}
