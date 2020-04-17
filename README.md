# ESP32 mqtt camera 

This is a complete implementation of a esp32 camera that publish images via mqtt. Images are also served on http

- images are published periodically as data/es32-mqtt-cam/image
- pub time can be configured via pubinterval parameter
- all camera parameters can be changed via mqtt publish to cmnd/es32-mqtt-cam/camera, messages mu be in JSON format:

cmnd/es32-mqtt-cam/camera {"framesize" : "3"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "6"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "8"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "9"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "10"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "11"}		:set framesize to 
cmnd/es32-mqtt-cam/camera {"framesize" : "13"}		: set framesize to UXGA (1600x1200)

cmnd/es32-mqtt-cam/camera {"nightmode" : "0"}		:disable night mode (normal framerate) 
cmnd/es32-mqtt-cam/camera {"nightmode" : "1"}		:enable night mode (slow framerate) 
cmnd/es32-mqtt-cam/camera {"save_to_nvs" : "1"}		:save current camera parameters 




All mqtt funtions are base on the repository framenic/esp32-mqtt-swith

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
