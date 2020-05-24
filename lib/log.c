#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "log.h"

#define LOG_INFO 	"info"
#define LOG_WARNING 	"warning"
#define LOG_ERROR 	"error"


struct log_t {
	int fd;
	int is_opened;
};

static struct log_t logger;

static int log_write(const char *level,
		const char *filename,
		int line, const char *msg);

int log_open(const char *filepath)
{
	int fd;

	if ((fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1)
		return -1;

	logger.fd = fd;
	logger.is_opened = 1;

	return -1;
}

int _log_info(const char *filename, int line, const char *fmt, ...)
{
	size_t nbytes;
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	nbytes = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	buf[nbytes] = 0;

	return log_write(LOG_INFO, filename, line, buf);
}

int _log_warning(const char *filename, int line, const char *fmt, ...)
{
	size_t nbytes;
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	nbytes = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	buf[nbytes] = 0;

	return log_write(LOG_WARNING, filename, line, buf);
}

int _log_error(const char *filename, int line, const char *fmt, ...)
{
	size_t nbytes;
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	nbytes = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	buf[nbytes] = 0;

	return log_write(LOG_ERROR, filename, line, buf);
}

static int log_write(const char *level,
		const char *filename,
		int line, const char *msg)
{
	char buf[2048];
	size_t nbytes;
	char timestr[100];
	time_t curtime;
	struct tm *tm;

	if (!logger.is_opened)
		return -1;

	time(&curtime);
	tm = localtime(&curtime);
	strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S", tm);

	nbytes = snprintf(buf, sizeof(buf),
			"[%s] %s %s:%d - %s\n",
			level, timestr, filename, line, msg);

	buf[nbytes] = 0;


	return write(logger.fd, buf, strlen(buf));
}
