/*
 * Copyright (c) 2014, Daliworks. All rights reserved.
 *
 * Reproduction and/or distribution in source and binary forms 
 * without the written consent of Daliworks, Inc. is prohibited.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>

#include "cJSON.h"
#include "MQTTClient.h"
#include "ThingPlusConfig.h"

/*
 * Constants
 */
#define CONFIG_FILENAME "config.json"
#define DEFAULT_REPORT_INTERVAL  60000 //default minimum report interval
#define SENSING_INTERVAL 60000

#define STATUS_ON "on"
#define STATUS_OFF "off"
#define STATUS_ERR "err"

#define TRUE 1
#define FALSE 0

#define MAX_BUF_LEN 1024

enum { /* topic list */
	LED_ID = 0,
	ONOFF_ID, TEMPERATURE_ID, HUMIDITY_ID,
	TOPIC_GATEWAY, TOPIC_GATEWAY_STATUS, TOPIC_MQTT_STATUS,
	TOPIC_RPC_REQ, TOPIC_RPC_RES,
	TOPIC_LED, TOPIC_ONOFF, TOPIC_TEMPERATURE, TOPIC_HUMIDITY,
	TOPIC_LED_STATUS, TOPIC_ONOFF_STATUS, TOPIC_TEMPERATURE_STATUS, TOPIC_HUMIDITY_STATUS,
	TOPIC_LAST
} topicList;

/* globals */
volatile int toStop = 0;
int gReportInterval = DEFAULT_REPORT_INTERVAL; // retrieved from stored value.
Network gNetwork;
Client gClient;
unsigned char gWritebuf[MAX_BUF_LEN];
unsigned char gReadbuf[MAX_BUF_LEN];
char* topics[TOPIC_LAST];

void prepareTopics(const char* gatewayId)
{
	char buf[128];

	strcpy(buf, "led-"); strcat(buf, gatewayId), strcat(buf, "-0");
	topics[LED_ID] = strdup(buf);
	strcpy(buf, "onoff-"); strcat(buf, gatewayId), strcat(buf, "-0");
	topics[ONOFF_ID] = strdup(buf);
	strcpy(buf, "temperature-"); strcat(buf, gatewayId), strcat(buf, "-0");
	topics[TEMPERATURE_ID]  = strdup(buf);
	strcpy(buf, "humidity-"); strcat(buf, gatewayId), strcat(buf, "-0");
	topics[HUMIDITY_ID]  = strdup(buf);

	strcpy(buf, "v/a/g/"); strcat(buf, gatewayId);
	topics[TOPIC_GATEWAY]  = strdup(buf);

	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/status");
	topics[TOPIC_GATEWAY_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/mqtt/status");
	topics[TOPIC_MQTT_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/req");
	topics[TOPIC_RPC_REQ]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/res");
	topics[TOPIC_RPC_RES]  = strdup(buf);

	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[LED_ID]);
	topics[TOPIC_LED]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[ONOFF_ID]);
	topics[TOPIC_ONOFF]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[TEMPERATURE_ID]);
	topics[TOPIC_TEMPERATURE]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[HUMIDITY_ID]);
	topics[TOPIC_HUMIDITY]  = strdup(buf);

	strcpy(buf, topics[TOPIC_LED]); strcat(buf, "/status/");
	topics[TOPIC_LED_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_ONOFF]); strcat(buf, "/status/");
	topics[TOPIC_ONOFF_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_TEMPERATURE]); strcat(buf, "/status/");
	topics[TOPIC_TEMPERATURE_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_HUMIDITY]); strcat(buf, "/status/");
	topics[TOPIC_HUMIDITY_STATUS]  = strdup(buf);
}

static void freeTopics(void)
{
	int i;
	for(i=0; i < TOPIC_LAST; i++) {
		free(topics[i]);
	}
}

static void stopSignal(int sig)
{
	signal(SIGINT, NULL);
	toStop = TRUE;
}

static double getCurrentTimeInMs(void)
{
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	return floor((tv.tv_sec) * 1000. + (tv.tv_usec) / 1000.);
}

/*
 * Simulate sensors. 
 * Note: only temperature and onoff supported for now.
 *
 * @param {cstring} id  sensorid, 
 * @param {cstring} buf appending sensor data to existing string.
 * @param {int} len     buf length
 * @return {int} string length
 */
static int getSensorData(char* id, cJSON * arrBuf) 
{
	static double nextTemperatureSensingTime = -1;
	static double nextHumiditySensingTime = -1;
	static double nextOnOffEventTime = -1;
	double now = getCurrentTimeInMs();
	//for stream sensor 
	if (strncmp("temperature", id, strlen("temperature")) == 0) {
		if (nextTemperatureSensingTime <= now) {
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber(getCurrentTimeInMs()));
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber((double)((rand() % 5000) / 100 )));
			nextTemperatureSensingTime = now + SENSING_INTERVAL;
		}
	} else if (strncmp("humidity", id, strlen("humidity")) == 0) {
		if (nextHumiditySensingTime <= now) {
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber(getCurrentTimeInMs()));
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber((double)((rand() % 10000) / 100 )));
			nextHumiditySensingTime = now + SENSING_INTERVAL;
		}
		//for event sensor 
	} else if (strncmp("onoff", id, strlen("onoff")) == 0) {
		//FIXME: to have timer for each sensor
		if (nextOnOffEventTime <= now) {
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber(getCurrentTimeInMs()));
			cJSON_AddItemToArray(arrBuf, cJSON_CreateNumber(rand() % 2));
			nextOnOffEventTime = now + ((rand() % 300)/100 * gReportInterval);
		}
	} else {
		ERROR_LOG("Unknown sensor=%s", id);
	}
	return cJSON_GetArraySize(arrBuf);
}

/*
 * Get gateway and sensor status 
 * @return {int} string length
 */
static int getStatus(char* buf, int len) 
{
	char* ptr = &buf[strlen(buf)];
	int duration = (int) (gReportInterval/1000 * 1.5);
	//gateway status
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%d,", STATUS_ON, duration);
	//sensor status
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d,", topics[LED_ID], STATUS_ON, duration);
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d,", topics[ONOFF_ID], STATUS_ON, duration);
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d,", topics[TEMPERATURE_ID], STATUS_ON, duration);
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d", topics[HUMIDITY_ID], STATUS_ON, duration);

	return strlen(buf);
};

/*
 * Send rpc reply
 * @param id {cstring} json rpc id
 * @param code {int} error if less then zero
 * @param message {int} error message or result message depending on code.
 */
static void sendRpcReply(cJSON* id, int code, char* message)
{
	cJSON *reply = cJSON_CreateObject();
	cJSON *error;
	char* err_msg;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	int rc;

	if (NULL != id) {
		cJSON_AddItemToObject(reply, "id", cJSON_Duplicate(id, 0));
	}
	if (code >= 0) { //error
		if (NULL != message) {
			cJSON_AddStringToObject(reply, "result", message);
		}
	}  else {
		error = cJSON_CreateObject();
		cJSON_AddNumberToObject(error, "code", code);
		if (NULL != message) {
			cJSON_AddStringToObject(error, "message", message);
		}
		cJSON_AddItemToObject(reply, "error", error);
	}
	err_msg = cJSON_PrintUnformatted(reply);
	msg.retained = FALSE;
	msg.payload = err_msg;
	msg.payloadlen = strlen(err_msg);
	rc = MQTTPublish(&gClient, topics[TOPIC_RPC_RES], &msg);
	INFO_LOG("RPC response rc=%d, %s %s\n", rc, topics[TOPIC_RPC_RES], (char*)msg.payload);
	free(err_msg);

	cJSON_Delete(reply);
}

static int rpc_cb_property_set(cJSON *id, cJSON *params)
{
	int i;
	if ( NULL != params && NULL != cJSON_GetObjectItem(params, "reportInterval")) {
		if (cJSON_String == cJSON_GetObjectItem(params,"reportInterval")->type) {
			char *ptr = NULL;
			i = (int)strtol(cJSON_GetObjectItem(params,"reportInterval")->valuestring, &ptr, 10);
		} else if (cJSON_Number == cJSON_GetObjectItem(params,"reportInterval")->type) {
			i = cJSON_GetObjectItem(params, "reportInterval")->valueint;
		}
		if (i >= DEFAULT_REPORT_INTERVAL) {
			gReportInterval = i;
			INFO_LOG("setProperty:reportInterval=%d\n", gReportInterval);
			sendRpcReply(id, 0, "success");
		} else {
			ERROR_LOG("setProperty:reportInterval(%d)<= minimum reportInterval(%d), not changed\n", gReportInterval, DEFAULT_REPORT_INTERVAL);
			sendRpcReply(id, 0, "not changed");
		}
	} else {
		sendRpcReply(id, -32602, "Invalid params");
		return;
	}
}

static int rpc_cb_actuator_control(cJSON *id, cJSON *params)
{
	char *actuatorId = NULL;
	char *actuatorCmd = NULL;
	int actuatorCmdInterval = -1;
	int actuatorCmdDuration = -1;
	cJSON* actuatorOptions = NULL;

	if (NULL != cJSON_GetObjectItem(params,"id") &&
			cJSON_String == cJSON_GetObjectItem(params,"id")->type) {
		actuatorId = cJSON_GetObjectItem(params,"id")->valuestring;
	}
	if (NULL != cJSON_GetObjectItem(params,"cmd") &&
			cJSON_String == cJSON_GetObjectItem(params,"cmd")->type) {
		actuatorCmd = cJSON_GetObjectItem(params,"cmd")->valuestring;
	}
	if (NULL == params || NULL == actuatorId || NULL == actuatorCmd) {
		sendRpcReply(id, -32602, "Invalid params");
		return;
	}
	if (NULL != cJSON_GetObjectItem(params,"options") && 
			cJSON_Object == cJSON_GetObjectItem(params,"options")->type) {
		actuatorOptions = cJSON_GetObjectItem(params,"options");
		if (NULL != actuatorOptions &&
				NULL != cJSON_GetObjectItem(actuatorOptions,"interval") &&
				cJSON_Number == cJSON_GetObjectItem(actuatorOptions,"interval")->type) {
			actuatorCmdInterval = cJSON_GetObjectItem(actuatorOptions,"interval")->valueint;
		}
		if (NULL != actuatorOptions &&
				NULL != cJSON_GetObjectItem(actuatorOptions,"duration") &&
				cJSON_Number == cJSON_GetObjectItem(actuatorOptions,"duration")->type) {
			actuatorCmdDuration = cJSON_GetObjectItem(actuatorOptions,"duration")->valueint;
		}
	}
	INFO_LOG("Run Actuator=%s, Cmd=%s, duration=%d, interval=%d\n", actuatorId, actuatorCmd, actuatorCmdDuration, actuatorCmdInterval);
	sendRpcReply(id, 0, "success");
}

static int rpc_cb_timesync(cJSON *id, cJSON *params)
{
	double remoteTime = -1;
	if (NULL != cJSON_GetObjectItem(params,"time") &&
			cJSON_Number == cJSON_GetObjectItem(params,"time")->type) {
		remoteTime = cJSON_GetObjectItem(params,"time")->valuedouble;
	}
	if (NULL == params || -1 == remoteTime) {
		sendRpcReply(id, -32602, "Invalid params");
		return;
	}
	INFO_LOG("Sync time: localTime(%.lf)-remoteTime(%.lf)=%.lf\n", getCurrentTimeInMs(), remoteTime, getCurrentTimeInMs() - remoteTime);
	sendRpcReply(id, 0, "success");
}

static void rpcRequestCallback(MessageData* md)
{
	MQTTMessage* message = md->message;
	char buf[MAX_BUF_LEN];
	char *err_msg = NULL;
	cJSON* id = NULL;
	char *method = NULL;
	cJSON* params = NULL;

	INFO_LOG("%.*s: %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, (int)message->payloadlen, (char*)message->payload);
	if (strncmp(topics[TOPIC_RPC_REQ], md->topicName->lenstring.data, md->topicName->lenstring.len) != 0) {
		ERROR_LOG("Not expected message\n");
		return;
	}

	if ((int)message->payloadlen <= 0) {
		return;
	}

	strncpy(buf, (char*)message->payload, (int)message->payloadlen);
	buf[(int)message->payloadlen] = '\0';

	cJSON * root = cJSON_Parse(buf);
	if (NULL == root) {
		ERROR_LOG("JSON parse error=%s", buf);
		sendRpcReply(id, -32700, "Parse error");
		goto exit;
	}

	err_msg = cJSON_Print(root);
	INFO_LOG("RPC req:\n%s\n", err_msg);
	free(err_msg);
	if (NULL != cJSON_GetObjectItem(root,"id") &&
			( cJSON_String == cJSON_GetObjectItem(root,"id")->type ||
			  cJSON_Number == cJSON_GetObjectItem(root,"id")->type)) {
		id = cJSON_GetObjectItem(root,"id");
	}
	if (NULL != cJSON_GetObjectItem(root,"method") &&
			cJSON_String == cJSON_GetObjectItem(root,"method")->type) {
		method = cJSON_GetObjectItem(root,"method")->valuestring;
	}
	if (NULL == id || NULL == method) {
		sendRpcReply(id, -32600, "Invalid Request");
		goto exit;
	}
	params = cJSON_GetObjectItem(root,"params");

	// setProperty for reportInterval
	if (strcmp(method, "setProperty") == 0) {
		rpc_cb_property_set(id, params);
	} else if (strcmp(method, "controlActuator") == 0) {
		rpc_cb_actuator_control(id, params);
	} else if (strcmp(method, "timeSync") == 0) {
		rpc_cb_timesync(id, params);
	} else {
		sendRpcReply(id, -32601, "Method not found");
		goto exit;
	}

exit:
	cJSON_Delete(root);
}

static void _gateway_sensor_status_send(void)
{
	int rc = 0;
	unsigned char statusBuf[MAX_BUF_LEN];
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};

	/* publish gateway and sensor status*/
	statusBuf[0] = '\0';
	getStatus(statusBuf, sizeof(statusBuf));
	msg.retained = FALSE;
	msg.payload = statusBuf;
	msg.payloadlen = strlen(statusBuf);
	rc = MQTTPublish(&gClient, topics[TOPIC_GATEWAY_STATUS], &msg);
	INFO_LOG("[%.lf] MQTTPublish %s: %s rc=%d\n", getCurrentTimeInMs(), topics[TOPIC_GATEWAY_STATUS], statusBuf, rc);
}

static void _series_sensor_value_send(void)
{
	int rc = 0;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	cJSON* temp0 = NULL;
	cJSON* humi0 = NULL;

	if (!temp0) { temp0 = cJSON_CreateArray(); }
	if (!humi0) { humi0 = cJSON_CreateArray(); }

	getSensorData(topics[TEMPERATURE_ID], temp0);
	getSensorData(topics[HUMIDITY_ID], humi0);

	/* publish sensor values*/
	if (cJSON_GetArraySize(temp0) > 0 || cJSON_GetArraySize(humi0) > 0) {
		cJSON* gwData = cJSON_CreateObject();
		if (cJSON_GetArraySize(temp0) > 0) {
			cJSON_AddItemToObject(gwData, topics[TEMPERATURE_ID], temp0);
			temp0 = NULL;
		}
		if (cJSON_GetArraySize(humi0) > 0) {
			cJSON_AddItemToObject(gwData, topics[HUMIDITY_ID], humi0);
			humi0 = NULL;
		}
		msg.retained = FALSE;
		msg.payload = cJSON_PrintUnformatted(gwData);
		msg.payloadlen = strlen(msg.payload);
		rc = MQTTPublish(&gClient, topics[TOPIC_GATEWAY], &msg);
		INFO_LOG("[%.lf] MQTTPublish %s: %s rc=%d\n", getCurrentTimeInMs(), topics[TOPIC_GATEWAY], (char*)msg.payload, rc);
		free(msg.payload); msg.payload = NULL;
		cJSON_Delete(gwData);
	}
}

static void _event_sensor_value_send(cJSON* onoff0)
{
	int rc = 0;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};

	msg.retained = FALSE;
	msg.payload = cJSON_PrintUnformatted(onoff0);
	msg.payloadlen = strlen(msg.payload);
	rc = MQTTPublish(&gClient, topics[TOPIC_ONOFF], &msg);
	INFO_LOG("[%.lf] MQTTPublish %s: %s rc=%d\n", getCurrentTimeInMs(), topics[TOPIC_ONOFF], (char*)msg.payload, rc);
	free(msg.payload); msg.payload = NULL;
	cJSON_Delete(onoff0);
}

static void _mqtt_connect(struct config *config)
{
	int rc = 0;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	/*
	 * Init TLS connection
	 */
	NewNetwork(&gNetwork);
	rc = ConnectTlsNetwork(&gNetwork, config->host, 
			config->port, config->ca_file, config->hostname);
	if (rc < 0) {
		ERROR_LOG("ConnectNetwork failure rc=%d\n", rc);
		exit(1);
	}

	/** 
	 * MQTT Connection setting
	 **/
	MQTTClient(&gClient, &gNetwork, 3000/*3sec timeout*/, 
			gWritebuf, sizeof(gWritebuf), gReadbuf, sizeof(gReadbuf));

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
	data.MQTTVersion = 3;
	data.clientID.cstring = config->gateway_id;
	data.cleansession = TRUE; //clean true
	data.keepAliveInterval = (gReportInterval/1000) * 2; // in secs
	data.willFlag = TRUE;
	data.will.topicName.cstring  = topics[TOPIC_MQTT_STATUS];
	data.will.message.cstring  = STATUS_ERR;
	data.will.qos = QOS1;
	data.will.retained = TRUE;
	data.username.cstring = config->username;
	data.password.cstring = config->password;

	INFO_LOG("Connecting to %s %d\n", 
			config->host, config->port);
	rc = MQTTConnect(&gClient, &data);
	INFO_LOG("Connected rc=%d\n", rc);
	/** 
	 * Publish "on" to mqtt/status on "connected"
	 **/
	if (0 == rc) {
		msg.retained = TRUE;
		msg.payload = STATUS_ON;
		msg.payloadlen = strlen(STATUS_ON);
		rc = MQTTPublish(&gClient, topics[TOPIC_MQTT_STATUS], &msg);
		INFO_LOG("[%.lf] MQTTPublish %s %s rc=%d\n", getCurrentTimeInMs(), topics[TOPIC_MQTT_STATUS], STATUS_ON, rc);
	} else {
		ERROR_LOG("MQTTConnection failed rc=%d\n", rc);
		exit(1);
	}

}

static void _mqtt_disconnect(void)
{
	MQTTDisconnect(&gClient);
	gNetwork.disconnect(&gNetwork);
}

int main(int argc, char** argv)
{
	int rc = 0;
	double nextReportTime = -1;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	char* hostname = NULL;
	cJSON* onoff0 = NULL;

	struct config *config = config_init(CONFIG_FILENAME);
	if (config == NULL) {
		ERROR_LOG("config_init failed\n");
		return -1;
	};

	prepareTopics(config->gateway_id);
	srand(time(NULL)); /*random seed*/

	signal(SIGINT, stopSignal);
	signal(SIGTERM, stopSignal);

	_mqtt_connect(config);

	/** 
	 * Subscribe to RPC request
	 **/
	INFO_LOG("Subscribing to %s\n", topics[TOPIC_RPC_REQ]);
	rc = MQTTSubscribe(&gClient, topics[TOPIC_RPC_REQ], QOS1, rpcRequestCallback);
	INFO_LOG("Subscribed rc=%d\n", rc);

	nextReportTime = getCurrentTimeInMs();
	while (!toStop)
	{
		MQTTYield(&gClient, 3000);

		if (!onoff0) { onoff0 = cJSON_CreateArray(); }

		/*
		 * Report status and sensors periodically(per report interval).
		 */
		if (nextReportTime <= getCurrentTimeInMs()) {
			_gateway_sensor_status_send();
			_series_sensor_value_send();
			nextReportTime = getCurrentTimeInMs() + gReportInterval;
		}

		/*
		 * onoff sensor report immediately on event 
		 */
		if (getSensorData(topics[ONOFF_ID], onoff0) > 0) {
			_event_sensor_value_send(onoff0);
			onoff0 = NULL;
		}
	} /*while*/

	ERROR_LOG("Stopping\n");

	_mqtt_disconnect();
	config_cleanup(config);
	freeTopics();

	return 0;
}
