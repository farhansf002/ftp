/**
 * This header file is responsible for declaring functions and
 * defining structs which will be used globally (common to all)
 * by all source files.
 */
#ifndef _FTP_H_
#define _FTP_H_

#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libsocket.h>

#include <str.h>


#define VALIDUSER 1
#define INVALIDUSER 0
#define PEER_TERMINATED -2
#define MAX_FTP_PARAM 6

/**
 * ----------------------------------------------------------------------------
 * REPLY SYMBOLIC CONSTANTS
 * ----------------------------------------------------------------------------
 */
#define REPLY_150 "File status okay; about to send data."
#define REPLY_200 "Command okay."
#define REPLY_221 "Service closing control connection."
#define REPLY_226 "Transfer Complete"
#define REPLY_230 "User logged in, proceed"
#define REPLY_331 "%s okay, need password"
#define REPLY_500 "Syntax error, command unrecognized."
#define REPLY_501 "Syntax error in parameters or arguments."
#define REPLY_502 "Command not implemented."
#define REPLY_503 "Bad sequence of commands."
#define REPLY_504 "Command not implemented for that parameter."
#define REPLY_530 "Not logged in."
#define REPLY_550 "Requested action not taken.File unavailable."

struct client {
	int 	fd;
	bool 	file_state;
	int 	state;
	bool	is_auth;
	char    *prev_cmd;
	int	prev_cmd_count;
};

struct ftp {
	char *cmd;
	char *params[MAX_FTP_PARAM];
	int   nparam;
	int   code;

	char *body;
};

struct command {
	char *cmd;
	int (*handler)(struct ftp *, struct client *); /* req, cli */
};

extern const struct command commands[];

int 	 ftp_command_find(struct ftp *req);
int 	 ftp_is_file(const char *filepath);
int 	 ftp_dir_list_get(const char *filepath, char **res);
int 	 ftp_file_details_get(const char *filepath, char **res);
int	 is_dir(const char *filepath);
double 	 show_unit(size_t size, char *unit);

#endif

