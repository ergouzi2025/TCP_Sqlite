#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void log_with_level(const char *level, const char *fmt, va_list args) {
    time_t now = time(NULL);
    struct tm *tm_now = NULL;
    char timestamp[32];

    tm_now = localtime(&now);
    if (tm_now == NULL) {
        snprintf(timestamp, sizeof(timestamp), "%lld", (long long)now);
    } else {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_now);
    }

    fprintf(stdout, "[%s] %s ", timestamp, level);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    fflush(stdout);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_with_level("INFO", fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_with_level("WARN", fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_with_level("ERROR", fmt, args);
    va_end(args);
}
