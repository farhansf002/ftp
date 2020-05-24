/**
 * This header file is responsible for declaring functions and
 * defining structs which will only be used by server
 */
#ifndef _FTPS_H_
#define _FTPS_H_

#include <ftp.h>
#include <log.h>
#include <errno.h>

int ftps_newclient(struct client **clis, size_t size, int fd);
int ftps_delclient(struct client **clients, int index);
int ftps_request_handle(struct client *cli);
int ftps_parse(const char *buf, size_t size, struct ftp *req);
int ftps_response(int fd, int code, const char *msg);

int ftps_cmd_user_handle(struct ftp *req, struct client *cli);
int ftps_cmd_pass_handle(struct ftp *req, struct client *cli);
int ftps_cmd_retr_handle(struct ftp *req, struct client *cli);
int ftps_cmd_list_handle(struct ftp *req, struct client *cli);
int ftps_cmd_nlst_handle(struct ftp *req, struct client *cli);
int ftps_cmd_quit_handle(struct ftp *req, struct client *cli);
int ftps_cmd_rest_handle(struct ftp *req, struct client *cli);

#endif

