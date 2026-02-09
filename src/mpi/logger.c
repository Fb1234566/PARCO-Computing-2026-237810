#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static FILE* s_fp = NULL;
static log_level_t s_level = LOG_LEVEL_INFO;
static pthread_mutex_t s_mtx = PTHREAD_MUTEX_INITIALIZER;

static inline const char* lvl_str(log_level_t l) {
    switch (l) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "NONE";
    }
}

int logger_init(const char* path, log_level_t level) {
    pthread_mutex_lock(&s_mtx);
    if (s_fp) { pthread_mutex_unlock(&s_mtx); return 0; }
    s_fp = fopen(path, "a");
    if (!s_fp) { pthread_mutex_unlock(&s_mtx); return -1; }
    s_level = level;
    // Disable buffering to update the file immediately during execution.
    setvbuf(s_fp, NULL, _IONBF, 0);
    pthread_mutex_unlock(&s_mtx);
    return 0;
}

void logger_set_level(log_level_t level) {
    pthread_mutex_lock(&s_mtx);
    s_level = level;
    pthread_mutex_unlock(&s_mtx);
}

void logger_log(log_level_t level, const char* file, int line, const char* fmt, ...) {
    if (!s_fp || level < s_level) return;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    time_t secs = ts.tv_sec;
    struct tm tm;
    localtime_r(&secs, &tm);
    long ms = ts.tv_nsec / 1000000;

    pid_t pid = getpid();
    long tid = syscall(SYS_gettid);

    pthread_mutex_lock(&s_mtx);
    if (!s_fp) { pthread_mutex_unlock(&s_mtx); return; }

    fprintf(s_fp, "%04d-%02d-%02d %02d:%02d:%02d.%03ld [%s] %d:%ld %s:%d: ",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, ms,
            lvl_str(level), (int)pid, tid, file, line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(s_fp, fmt, ap);
    va_end(ap);

    fputc('\n', s_fp);
    // Flush libc buffers and also fsync underlying file descriptor to ensure persistence.
    fflush(s_fp);
    int fd = fileno(s_fp);
    if (fd >= 0) { fsync(fd); }
    pthread_mutex_unlock(&s_mtx);
}

void logger_close(void) {
    pthread_mutex_lock(&s_mtx);
    if (s_fp) { fflush(s_fp); int fd = fileno(s_fp); if (fd >= 0) fsync(fd); fclose(s_fp); s_fp = NULL; }
    pthread_mutex_unlock(&s_mtx);
}
