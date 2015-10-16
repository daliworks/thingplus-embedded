```
Copyright (c) 2014, Daliworks, Inc. All rights reserved.

Reproduction and/or distribution without the written consent of 
Daliworks, Inc. is prohibited.
```

__Version: 0.9.5__

    
Change History:

0.9.5

 * adding "Implementation Note" section

0.9.4

 * newly introducing "Gateway data" message. You have an option to send multiple sensors' data together instead of multiple "Sensor data" messages.

0.9.3

  * "Gateway status" message format change. Each sensor status to have valid_duration.
  
 ```
 {gateway_status},{valid_duration},{sensor_id},{sensor_status},
      ...{repeats for the remaining sensors}
 ```
 -->
 
 ```
 {gateway_status},{valid_duration},{sensor_id},{sensor_status},{valid_duration} 
       ...{repeats for the remaining sensors}
```

  * adding timeSync rpc command
  
0.9.2

  * Reformating and fix typos.

---        
# Gateway <-> Server over MQTT

## Gateway/Sensor status definition
 * status
    * ```off```
      * off or expired(no status report within expire period)
    * ```on```
    * ```err```
      * on mqtt disconnection(offline)

 * sensor/gateway status transition on mqtt connection change

   previous sensor/gateway status | on MQTT connection   | on MQTT disconnection
   -----------|------------------|------------
   null       | off              | off
   err        | off on expire    | off
   on         | off on expire    | off
   off        | off              | off

## Terms that topic and content contains: 
  * reportInterval: the interval that all gathered sensor data are sent regularily. It must be equal or greater than 1 minutes.
  * {service} : ```v``` for now. It is part of topic and reserved for future extension.
  * {region} : ```a``` for now. It is part of topic and reserved for future extension.
  * g: gateway. part of topic.
  * s: sensor. part of topic.
  * {gateway_id} : uniq gateway id.(e.g. mac address, IMEI number)
  * {sensor_id} : uniq sensor id. It must be agreed beforehand.(e.g. {sensor_type}-{gateway_id}-{sequence})
  * {gateway_status}, {sensor_status} : on|off|err[:{error_code_or_message}]
    * {error_code_or_message} : [OPTIONAL] error code(comply with http status codes), or message which doesn't include comma.
  * {time} : the number of milliseconds since 1970/01/01
  * {value} : any string or json object
  * {valid_duration} : the value will be expired in this ealid duration. -1 for unlimited.

## Connection setting (MUST)
  * protocolId: Protocol ID, usually ```MQIsdp```
  * protocolVersion: Protocol version, ```3```
  * clientId: {gateway_id}
  * clean: the 'clean start' flag. boolean, ```true```
  * keepalive: keepalive period in seconds, recommend ```reportInterval * 2```
  * will: the client's will message options.
    * topic: {service}/{region}/g/{gateway_id}/mqtt/status
    * message: ```err``` with options:  qos ```1``` and retain ```true```
  * on connected, publish below message
    * topic: {service}/{region}/g/{gateway_id}/mqtt/status
    * message: ```on``` with options:  qos ```1``` and retain ```true```
  * secure connection: tls v1.x connection, port 8883
    * supporting below two authentication methods. 2nd method is recommended only for high-end linux based gateway.
      1. username and password for authentication.
      	 * username: gateway id
	     * password: Thing+ service to provide passcode on pre-registration of the gateway.
      2. client private key and certificate 
         * Thing+ service to provide gateway's private key on its pre-registration.

  * for testing purpose, you can use mosquito(http://mosquitto.org/)
  
      ```
      # subscribe rpc request and set will message on disconnection
      > mosquitto_sub -h mqtt.thingplus.net  -p 8883 \
      -u {mac addr} -P {password}  -k 120 -i {mac addr} -q 1 \
      --cafile ./ca-cert.pem -t 'v/a/g/{mac address}/req' \
      --will-qos 1 --will-retain -will-topic v/a/g/b827eb1dcccc/mqtt/status \
      --will-payload err 
      # On connected, publish 'on' as retained message.
      > mosquitto_pub -h mqtt.thingplus.net -p 8883 \
      -u {mac addr} -P {password} -i {mac addr} -q 1 \
      --cafile ./ca-cert.pem \
      -t 'v/a/g/b827eb1dcccc/mqtt/status' -r -m "on" 
      ```

## Publish message 

### Publish message options (MUST)
  * qos level : ```1```
  * retain : ```false```

Note: a zip compressed message is supported. It is recommended for large messages(e.g. message length > 1kbytes).

### Gateway status
  * topic:
    * ```{service}/{region}/g/{gateway_id}/status```
  * message:
    * ```{gateway_status},{valid_duration},{sensor_id},{sensor_status},{valid_duration} ...{repeats for the remaining sensors}```
    * status data starts with gateway status and all sensor data of the gateway follow.
  * example:
    * ```v/a/g/b827eb1dcccc/status on,90,28–000003a82057,on,90,28–000003a7f65d,on,90```
   
### Sensor status
  * topic:
    * ```{service}/{region}/g/{gateway_id}/s/{sensor_id}/status```
  * message:
    * ```{sensor_status},{valid_duration}```
  * example:
    * ```v/a/g/b827eb1dcccc/s/28–000003a7f65d/status on,90```

### Sensor data
  * topic:
    * ```{service}/{region}/g/{gateway_id}/s/{sensor_id}```
  * message:
    * ```{time},{value},...(repeats)```
    * more than one sensor data(time and value pair) can be sent together. Json array of time and value pair is also accepted.
  * example:
    * ```v/a/g/b827eb1dcccc/s/28–000003a82057 1372874400865,-15.687,1372874401865,-16.687```
  * json example:
    * ```v/a/g/b827eb1dcccc/s/28–000003a82057 [1372874400865,-15.687,1372874401865,-16.687]```
    * ```v/a/g/b827eb1dcccc/s/28–000003a82057 [1372874400865, {"lat': 37.11, "lng": 117.22}]```

### Gateway data(sending multiple sensors' data);
  * topic:
    * ```{service}/{region}/g/{gateway_id}```
  * message:
    * json object with key and value pairs ```{sensor_id}: [{time},{value},...(repeats)]```
  * example:
    * ```v/a/g/b827eb1dcccc {"28–000003a82057": [1372874400865,-15.687,1372874401865,-16.687], "28–000003a82057": [1372874400865,-15.687,1372874401865,-16.687]}```

## Remore procedure call(RPC) via MQTT

The message format is comply with JSON-RPC 2.0(http://www.jsonrpc.org/specification)

Note: "setProperty.reportInterval" shoul be implemented. "controlActuator" should be 
implemented if any actuator is supported on your device. 

### RPC Request from Server
  * topic
     * ```{service}/{region}/g/{gateway_id}/req```
     * subscription options: 
        * qos level : ```1```
  * request message
     * id: String. The request id
     * method: String
     * params: Object
        * poweroff, reboot, restart, swUpdate and swInfo params: NONE
        * setProperty params:
           * ```reportInterval```: Number, report interval in milliseconds
        * controlActuator params:
           * powerSwitch and led
              * ```id``` : String, actuator id
              * ```cmd``` : String, actuator command(on|off|blink)
              * ```options.duration``` : Number, in milliseconds
              * ```options.interval``` : Number, in milliseconds
           * Note: for other actuators like Camera or your proprietary one, Thing+ service team will help you on supporting them.
        * timeSync params:
           * ```time```: Number, current time in milliseconds since jan 1, 1970

### RPC Response to Server
  * topic
    * ```{service}/{region}/g/{gateway_id}/res```
  * response message on success
    * id: String. It must be the same id as the request it is responding to.
    * result: String or Object
  * response message on failure
    * id: String. It MUST be the same as the value of the id member in the Request Message.
    * error: Object
      * code: Number
      * message: String

### setproperty example: setting reportInterval
  * subscribe topic: ```v/a/g/b827eb1dcccc/req``` with ```qos 1``` option.
    * received message: ```{"id":"e1kcs13bb","method":"setProperty","params":{"reportInterval":"60000"}}```
  * publish result topic: ```v/a/g/b827eb1dcccc/res```
    * success message: ```{"id":"e1kcs13bb","result":""}```
    * error message: ```{"id":"e1kcs13bb","error": {"code": -32000, "message": "invalid interval"}}```

### controlActuator example: control led
  * subscribe topic: ```v/a/g/b827eb1dcccc/req``` with ```qos 1``` option.
    * received blink command: ```{"id":"46h6f8xp3","method":"controlActuator","params":{"id":"led-b827eb1dcccc-r","cmd":"blink", "options": { "interval": 5000, "duration": 20000}}}```
    * cmds and their options
      * cmd: on
         * options.duration: milliseconds [OPTIONAL]
      * cmd: off
         * options: none
      * cmd: blink
         * options.interval: milliseconds [OPTIONAL]
         * options.duration: milliseconds [OPTIONAL]
  * publish result topic: ```v/a/g/b827eb1dcccc/res``` 
    * success message: ```{"id":"46h6f8xp3","result":""}```
    * error message: ```{"id":"46h6f8xp3","error": {"code": -32000, "message": "invalid options"}}```


# Implementation Note

## MQTT Connection

 * When MQTT connection is closed, the device must wait until close event form the MQTT server. The next reconnection try must happen after waiting at least 10 secs.

## Sending Sensor Data 

 * The report interval must be equal or greater than 1 minutes.
 * The time of sensor data must be reported in time order. Otherwise, those data will be ignored.
