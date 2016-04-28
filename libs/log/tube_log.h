#ifndef _TUBE_LOG_H_
#define _TUBE_LOG_H_

#include <stdio.h>

#if 0
#ifndef ENUM_BEGIN
#define ENUM_BEGIN(type)	enum type {
#endif

#ifndef ENUM
#define ENUM(name)		name
#endif

#ifndef ENUM_END
#define ENUM_END(type)		};
#endif

ENUM_BEGIN(tube_log_level_bit)
	ENUM(TUBE_LOG_ERROR),
	ENUM(TUBE_LOG_WARN),
	ENUM(TUBE_LOG_INFO),
	ENUM(TUBE_LOG_DEBUG),
ENUM_END(tube_log_level_bit)
#endif

enum tube_log_level_bit {
	TUBE_LOG_ERROR,
	TUBE_LOG_WARN,
	TUBE_LOG_INFO,
	TUBE_LOG_DEBUG,
};

extern unsigned int tube_log_level;
extern FILE *log_fp;

#define tube_log_error(text, args...)	tube_log_print(TUBE_LOG_ERROR, text, ##args)
#define tube_log_warn(text, args...)	tube_log_print(TUBE_LOG_WARN, text, ##args)
#define tube_log_info(text, args...)	tube_log_print(TUBE_LOG_INFO, text, ##args)
#define tube_log_debug(text, args...)	tube_log_print(TUBE_LOG_DEBUG, text, ##args)

#define tube_log_print(mylevel, text, args...)                                  \
        do {                                                                    \
                if((1<<mylevel) & tube_log_level)                               \
                        fprintf(log_fp, "["#mylevel"]@%s()\t"text, __func__, ##args);    \
        } while(0)

void tube_log_enable_level(enum tube_log_level_bit level);
void tube_log_disable_level(enum tube_log_level_bit level);

void tube_log_init(char *file);

#endif //#define _TUBE_LOG_H_
