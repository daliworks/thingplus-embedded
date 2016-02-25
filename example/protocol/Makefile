# common
OBJS_DIR=./build/obj
UNAME_S := $(shell uname -s)

ifdef TARGET_PORT
	CFLAGS += -D ${TARGET_PORT}
else
ifeq ($(UNAME_S),Linux)
  CFLAGS += -D LINUX
	TARGET_PORT := linux
else
  $(error "Unknown TARGET_PORT")
endif
endif

$(info TARGET_PORT=${TARGET_PORT})

# mqtt directories
MQTT_DIR=./paho/MQTTPacket
MQTT_SRC_DIR=$(MQTT_DIR)/src
MQTT_PORT_DIR=./${TARGET_PORT}
CJSON_DIR=./cjson

# mqtt objects
MQTT_PACKET_OBJS=MQTTConnectClient.o MQTTSerializePublish.o MQTTPacket.o MQTTSubscribeClient.o MQTTDeserializePublish.o MQTTConnectServer.o MQTTSubscribeServer.o MQTTUnsubscribeServer.o MQTTUnsubscribeClient.o MQTTFormat.o
MQTT_PORT_OBJS=MQTTLinux.o 
CJSON_OBJS=cJSON.o
OBJS=$(patsubst %,$(OBJS_DIR)/%,$(MQTT_PACKET_OBJS)) $(patsubst %,$(OBJS_DIR)/%,$(MQTT_PORT_OBJS)) $(patsubst %,$(OBJS_DIR)/%,$(CJSON_OBJS))

# flags
ifdef ENABLE_DEBUG
	DEBUG_FLAGS = -g -D DEBUG_LEVEL=0
endif
CFLAGS+=-I$(MQTT_SRC_DIR) -I$(MQTT_PORT_DIR) -I$(CJSON_DIR) -I . $(DEBUG_FLAGS)
LDLIBS=-lssl -lcrypto -lm
LD_FLAGS= $(DEBUG_FLAGS)


# mqttcd objects
THINGPLUS_OBJS=$(patsubst %,$(OBJS_DIR)/%,MQTTClient.o ThingPlus.o ThingPlusConfig.o)

all: ${OBJS_DIR} $(OBJS) $(THINGPLUS_OBJS)
	gcc -o thingplus $(LD_FLAGS) $(OBJS) $(THINGPLUS_OBJS) $(LDLIBS)

$(OBJS_DIR)/%.o: $(MQTT_SRC_DIR)/%.c
	gcc -o $@ $(CFLAGS) -c $<

$(OBJS_DIR)/%.o: $(MQTT_PORT_DIR)/%.c
	gcc -o $@ $(CFLAGS) -c $<

$(OBJS_DIR)/%.o: $(CJSON_DIR)/%.c
	gcc -o $@ $(CFLAGS) -c $<

$(OBJS_DIR)/%.o: %.c
	gcc -o $@ $(CFLAGS) -c $<

${OBJS_DIR}:
	mkdir -p $@

clean:
	rm -f mqttcd $(OBJS_DIR)/*.o
