#ifndef __MQTT_LOGGING_
#define __MQTT_LOGGING_
/*
 * DEBUG PRINT 
 */
#define DEBUG_TRACE 0
#define DEBUG_INFO 1
#define DEBUG_ERROR 2

/* change debug level here */
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 1
#endif

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL <= DEBUG_TRACE
#define TRACE_LOG(fmt, args...) fprintf(stderr, "TRACE: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
#define TRACE_LOG(fmt, args...) 
#endif

#if defined(DEBUG_LEVEL) && DEBUG_LEVEL <= DEBUG_INFO
#define INFO_LOG(fmt, args...) fprintf(stderr, "INFO: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
#define INFO_LOG(fmt, args...) 
#endif

#define ERROR_LOG(fmt, args...) fprintf(stderr, "ERROR: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)

#endif /*__MQTT_LOGGING_*/
