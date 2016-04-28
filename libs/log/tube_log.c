#include <stdio.h>
#include "tube_log.h"

#if 0
#undef ENUM_BEGIN
#undef ENUM
#undef ENUM_END

#define ENUM_BEGIN(type)        static const char* type##_string[] = {
#define ENUM(name)              #name
#define ENUM_END(type)          };
#undef _TUBE_LOG_H_

#include "tube_log.h"
#endif

unsigned int tube_log_level = (1 << TUBE_LOG_ERROR) | \
			      (1 << TUBE_LOG_WARN) | \
			      (1 << TUBE_LOG_INFO) | \
			      (1 << TUBE_LOG_DEBUG) ;
FILE *log_fp;

void tube_log_enable_level(enum tube_log_level_bit level)
{
	tube_log_level |= 1 << level;
}

void tube_log_disable_level(enum tube_log_level_bit level)
{
	tube_log_level &= ~(1 << level);
}

void tube_log_init(char *file)
{
	log_fp = stderr;

	if (file) {
		log_fp = fopen(file, "a");

		if (log_fp == NULL) {
			fprintf(stderr, "[TUBE_LOG] can`t open log file.");
			fprintf(stderr, "[TUBE_LOG] log is printed to stderr");
			log_fp = stderr;
		}
	}
}
