#ifndef _LOG_H_
#define _LOG_H_

#define log_info(a, ...) _log_info(__FILE__, __LINE__, (a), ##__VA_ARGS__)
#define log_warning(a, ...) _log_warning(__FILE__, __LINE__, (a), ##__VA_ARGS__)
#define log_error(a, ...) _log_error(__FILE__, __LINE__, (a), ##__VA_ARGS__)

int log_open(const char *filepath);
int _log_info(const char *filename, int line, const char *fmt, ...);
int _log_warning(const char *filename, int line, const char *fmt, ...);
int _log_error(const char *filename, int line, const char *fmt, ...);
int log_close();

#endif

