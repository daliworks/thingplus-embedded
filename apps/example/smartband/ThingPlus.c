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
#include "tube_http.h"

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
	BATTERY_ID, STEP_ID, LOCATION_ID,
	TOPIC_GATEWAY, TOPIC_GATEWAY_STATUS, TOPIC_MQTT_STATUS,
	TOPIC_RPC_REQ, TOPIC_RPC_RES,
	TOPIC_BATTERY, TOPIC_STEP, TOPIC_LOCATION,
	TOPIC_BATTERY_STATUS, TOPIC_STEP_STATUS, TOPIC_LOCATION_STATUS,
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

	strcpy(buf, gatewayId); strcat(buf, "-0-batteryGauge-0");
	topics[BATTERY_ID]  = strdup(buf);

	strcpy(buf, gatewayId); strcat(buf, "-0-stepCount-0");
	topics[STEP_ID]  = strdup(buf);

	strcpy(buf, gatewayId); strcat(buf, "-0-string-0");
	topics[LOCATION_ID]  = strdup(buf);

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

	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[BATTERY_ID]);
	topics[TOPIC_BATTERY]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[STEP_ID]);
	topics[TOPIC_STEP]  = strdup(buf);
	strcpy(buf, topics[TOPIC_GATEWAY]); strcat(buf, "/s/"); strcat(buf, topics[LOCATION_ID]);
	topics[TOPIC_LOCATION]  = strdup(buf);

	strcpy(buf, topics[TOPIC_BATTERY]); strcat(buf, "/status/");
	topics[TOPIC_BATTERY_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_STEP]); strcat(buf, "/status/");
	topics[TOPIC_STEP_STATUS]  = strdup(buf);
	strcpy(buf, topics[TOPIC_LOCATION]); strcat(buf, "/status/");
	topics[TOPIC_LOCATION_STATUS]  = strdup(buf);
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
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d,", topics[BATTERY_ID], STATUS_ON, duration);
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d,", topics[STEP_ID], STATUS_ON, duration);
	ptr += snprintf(ptr, (len + buf - ptr), "%s,%s,%d", topics[LOCATION_ID], STATUS_ON, duration);

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

static void _series_sensor_value_send(int step, int battery, char* location)
{
	int rc = 0;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	cJSON* battery_cjson = NULL;
	cJSON* stepcount_json = NULL;
	cJSON* location_json = NULL;

	if (!battery_cjson) { battery_cjson = cJSON_CreateArray(); }
	if (!stepcount_json) { stepcount_json = cJSON_CreateArray(); }
	if (!location_json) { location_json = cJSON_CreateArray(); }


	cJSON_AddItemToArray(battery_cjson, cJSON_CreateNumber(getCurrentTimeInMs()));
	cJSON_AddItemToArray(battery_cjson, cJSON_CreateNumber(battery));

	cJSON_AddItemToArray(stepcount_json, cJSON_CreateNumber(getCurrentTimeInMs()));
	cJSON_AddItemToArray(stepcount_json, cJSON_CreateNumber(step));

	cJSON_AddItemToArray(location_json, cJSON_CreateNumber(getCurrentTimeInMs()));
	cJSON_AddItemToArray(location_json, cJSON_CreateString(location));

	/* publish sensor values*/
	if (cJSON_GetArraySize(battery_cjson) > 0 || cJSON_GetArraySize(stepcount_json) > 0 ||cJSON_GetArraySize(location_json)) {
		cJSON* gwData = cJSON_CreateObject();
		if (cJSON_GetArraySize(battery_cjson) > 0) {
			cJSON_AddItemToObject(gwData, topics[BATTERY_ID], battery_cjson);
			battery_cjson = NULL;
		}
		if (cJSON_GetArraySize(stepcount_json) > 0) {
			cJSON_AddItemToObject(gwData, topics[STEP_ID], stepcount_json);
			stepcount_json = NULL;
		}
		if (cJSON_GetArraySize(location_json) > 0) {
			cJSON_AddItemToObject(gwData, topics[LOCATION_ID], location_json);
			location_json = NULL;
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

void _device_sessor_register(char* gw_id, char* apikey, char* band_name)
{
	void *t = tube_http_init(gw_id, apikey);
	char device_id[TUBE_HTTP_MAX_ID] = {0,};
	tube_http_device_register(t, band_name, 0, "jsonrpcFullV1.0", device_id);

	char sensor_id[TUBE_HTTP_MAX_ID] = {0,};
	tube_http_sensor_register(t, "LOCATION", 0, "string", device_id, sensor_id);
	tube_http_sensor_register(t, "BATTERY", 0, "batteryGauge", device_id, sensor_id);
	tube_http_sensor_register(t, "STEP COUNT", 0, "stepCount", device_id, sensor_id);

	tube_http_cleanup(t);
}


int thingplus(char* gw_id, char* apikey, char* band_name, int step, int battery, char* location)
{
	int rc = 0;
	double nextReportTime = -1;
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	char* hostname = NULL;

	_device_sessor_register(gw_id, apikey, band_name);

	struct config _config = {
		.gateway_id = gw_id,
		.hostname = "dmqtt.thingplus.net",
		.host = "dmqtt.thingplus.net",
		.port = 8883,
		.username = gw_id,
		.password = apikey,
		.ca_file = "./cert/ca-cert.pem",
	};
	struct config* config = &_config;

	prepareTopics(config->gateway_id);
	_mqtt_connect(config);

	INFO_LOG("Subscribing to %s\n", topics[TOPIC_RPC_REQ]);
	rc = MQTTSubscribe(&gClient, topics[TOPIC_RPC_REQ], QOS1, rpcRequestCallback);
	INFO_LOG("Subscribed rc=%d\n", rc);

	_gateway_sensor_status_send();
	_series_sensor_value_send(step, battery, location);

	_mqtt_disconnect();
	freeTopics();

	return 0;
}

int main(void)
{
	int tagging = 1;

	if (tagging)
	{
		int gw_id = "84eb18afeb0c";
		int apikey = "YHZbDd0n6TQpPJP2MLBKHM7hn7o=";;
		char* band_name = "Smart Band0";
		int step = 100;
		int battery = 38;
		char* location = "Hospital";

		thingplus(gw_id, apikey, band_name, step, battery, location);
	}
}
