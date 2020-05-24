#ifndef _FTPC_H_
#define _FTPC_H_

#include <ftp.h>

int 		ftpc_handle_find(const char *cmd);
int   		ftpc_command_find(const char *cmd);
int   		ftpc_parse(char *cmdline, struct ftp *req);
ssize_t 	ftpc_prepare(struct ftp *req, char *buf, ssize_t buflen, int cmdindex);
int   		ftpc_send(int fd, const char *buf);
int 		ftpc_request_send(int fd, const char *buf, size_t size);

/**
 * Dumps the request req. Helpful in debuging
 */
void ftpc_req_dump(struct ftp *req);

int ftpc_cmd_auth_handle(struct ftp *req, struct client *cli);
int ftpc_cmd_get_handle(struct ftp *req, struct client *cli);
int ftpc_cmd_ls_handle(struct ftp *req, struct client *cli);
int ftpc_cmd_nls_handle(struct ftp *req, struct client *cli);
int ftpc_cmd_quit_handle(struct ftp *req, struct client *cli);
int ftpc_cmd_rest_handle(struct ftp *req, struct client *cli);


#endif

