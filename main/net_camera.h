#include <esp_http_server.h>

esp_err_t init_camera();

int set_camera_param(char *camera_param,int value); 

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

esp_err_t set_camera(char * parameter,char *value);