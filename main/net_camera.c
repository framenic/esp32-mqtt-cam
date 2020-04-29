#include "esp_camera.h"

#include <esp_log.h>
#include "driver/gpio.h"

#include "esp_timer.h"
#include "net_camera.h"

static const char *TAG = "net_camera.c";

static camera_config_t camera_config = {
    .pin_pwdn = CONFIG_PWDN,
    .pin_reset = CONFIG_RESET,
    .pin_xclk = CONFIG_XCLK,
    .pin_sscb_sda = CONFIG_SDA,
    .pin_sscb_scl = CONFIG_SCL,

    .pin_d7 = CONFIG_D7,
    .pin_d6 = CONFIG_D6,
    .pin_d5 = CONFIG_D5,
    .pin_d4 = CONFIG_D4,
    .pin_d3 = CONFIG_D3,
    .pin_d2 = CONFIG_D2,
    .pin_d1 = CONFIG_D1,
    .pin_d0 = CONFIG_D0,
    .pin_vsync = CONFIG_VSYNC,
    .pin_href = CONFIG_HREF,
    .pin_pclk = CONFIG_PCLK,

    //XCLK 20MHz or 10MHz
    .xclk_freq_hz = CONFIG_XCLK_FREQ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_UXGA,   //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 8, //0-63 lower number means higher quality
    .fb_count = 2       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

esp_err_t init_camera(){
    //power up the camera if PWDN pin is defined
    if(camera_config.pin_pwdn != -1){
		gpio_pad_select_gpio(camera_config.pin_pwdn);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(camera_config.pin_pwdn, GPIO_MODE_OUTPUT);
	
        gpio_set_level(camera_config.pin_pwdn, 0);
    }

	
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
	
	esp_camera_load_from_nvs("cam_param");
	//esp_camera_sensor_get()->set_nightmode(esp_camera_sensor_get(),1);

    return ESP_OK;
}

esp_err_t set_camera(char * parameter,char *value)
{
	int val = atoi(value);
	
    ESP_LOGI(TAG, "Setting %s = %s", parameter, value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    if(!strcmp(parameter, "framesize")) {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(parameter, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(parameter, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(parameter, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(parameter, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(parameter, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(parameter, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(parameter, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(parameter, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(parameter, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(parameter, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(parameter, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(parameter, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(parameter, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(parameter, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(parameter, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(parameter, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(parameter, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(parameter, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(parameter, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(parameter, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(parameter, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(parameter, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(parameter, "ae_level")) res = s->set_ae_level(s, val);
	else if(!strcmp(parameter, "nightmode")) res = s->set_nightmode(s, val);
	else if(!strcmp(parameter, "save_to_nvs") && !strcmp(value, "1")) res = esp_camera_save_to_nvs("cam_param");
    else ESP_LOGW(TAG, "Invalid parameter %s", parameter);

	return ESP_OK;
}

typedef struct
{
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index)
  {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
  {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t jpg_httpd_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t fb_len = 0;
  int64_t fr_start = esp_timer_get_time();

  fb = esp_camera_fb_get();
  if (!fb)
  {
    ESP_LOGE(TAG, "Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  res = httpd_resp_set_type(req, "image/jpeg");
  if (res == ESP_OK)
  {
    res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  }

  if (res == ESP_OK)
  {
    if(fb->format == PIXFORMAT_JPEG) {
		fb_len = fb->len;
		res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
	} else {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
  }
  esp_camera_fb_return(fb);
  int64_t fr_end = esp_timer_get_time();
  ESP_LOGI(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len / 1024), (uint32_t)((fr_end - fr_start) / 1000));
  return res;
}

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t mjpg_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}

httpd_uri_t uri_handler_jpg = {
    .uri = "/jpg",
    .method = HTTP_GET,
    .handler = jpg_httpd_handler};


httpd_uri_t uri_handler_mjpg = {
    .uri = "/mjpg",
    .method = HTTP_GET,
    .handler = mjpg_httpd_handler};


httpd_handle_t start_webserver(void)
{
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK)
  {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &uri_handler_jpg);
	httpd_register_uri_handler(server, &uri_handler_mjpg);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

void stop_webserver(httpd_handle_t server)
{
  // Stop the httpd server
  httpd_stop(server);
}
