# ESP32 mqtt camera with nightmode function

esp32-mqtt-camera is an mqtt client camera that publishes images to mqtt (using secure TLS connections). Images are also served locally at http://CAMERA_IP/jpg and http://CAMERA_IP/mjpg

- images are published periodically as data/es32-mqtt-cam/image as jpg payload
- publish interval time in seconds can be configured via pubinterval parameter (cmnd/es32-mqtt-cam/pubinterval) 0 means no publish
- all camera parameters are saved to nvs to restore configuration at power up
- all camera parameters can be changed via mqtt by publishing to cmnd/es32-mqtt-cam/camera, messages must be in JSON format (e.g. cmnd/es32-mqtt-cam/camera {"framesize" : "3"}):

{"framesize" : "3"}		:set framesize to QVGA (320x240)

{"framesize" : "6"}		:set framesize to CIF (400x296)

{"framesize" : "8"}		:set framesize to VGA (640x480)

{"framesize" : "9"}		:set framesize to SVGA (800x600)

{"framesize" : "10"}		:set framesize to XGA (1024x768)

{"framesize" : "11"}		:set framesize to HD 16/9 (1280x720)

{"framesize" : "13"}		:set framesize to UXGA (1600x1200)


Complete list of frame sizes here: https://github.com/framenic/esp32-camera/blob/master/driver/sensor.c

{"nightmode" : "0"}		:disable night mode (normal framerate)

{"nightmode" : "1"}		:enable night mode (slow framerate) 

{"save_to_nvs" : "1"}		:save current camera parameters 





All mqtt funtions are based on the repository esp32-mqtt-swith
https://github.com/framenic/esp32-mqtt-switch

- support TLS encryption to remote MQTT server
- support configuration and commands via telnet
- support configuration and commands via uart
- Real time clock synchronization via ntp
- Configuration saved via esp-idf NVS Storage
Wifi parameters, Network parameters, MQTT options are permanently saved in NVS partition, under namespace Syscfg
- support OTA updates via http protocol 
- support OTA update via mqtt messages
- support wifi AP mode for initial configuration via http (default address 192.168.1.1)

Button functions:
- 1 short press
	toggles power on/off
- 4 short press
	start ap mode, led fast flash
- long press (>4s)
	factory reset, restore flash and data

Device enters OTA update by sending message 'cmnd/esp32-cam/mqttotastart 1'

Within 20 seconds a message to OTA/DVES_XXXXXX/data must be sent containing the new frmware

After reboot, new firmware must be confirmed sending 'cmnd/esp32-cam/mqttotaconfirm 1', othewise the device performs a rollback on next reboot

Example:


mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t cmnd/esp32-cam/mqttotastart -m 1

mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t OTA/DVES_XXXXXX/data -f build/esp32cam-mqtt.bin

After reboot:

mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t cmnd/esp32-cam/mqttotaconfirm -m 1
