#include <ftps.h>

static const char *username = "username";
static const char *password = "password";
const struct command commands[] = {
	{"USER", NULL, 		ftps_cmd_user_handle},
	{"PASS", NULL, 		ftps_cmd_pass_handle},
	{"RETR", "get", 	NULL},
	{NULL, NULL, NULL}
};

int ftps_cmd_user_handle(struct ftp *req, struct client *cli)
{
	char res[BUFSIZ];

	if (req->params[0] == NULL) {
		ftps_response(cli->fd, 501, "message");
		fprintf(stderr, "ftps_cmd_user_handle: username missing\n");
		return -1;
	}

	if (strcmp(username, req->params[0]) != 0) {
		ftps_response(cli->fd, 530, "Not logged in.");
		fprintf(stderr, "ftps_cmd_user_handle: invalid username '%s'\n", req->params[0]);
		return -1;
	}

	sprintf(res, "%s okay, need password", req->params[0]);
	ftps_response(cli->fd, 331, res);

	return 0;
}

int ftps_cmd_pass_handle(struct ftp *req, struct client *cli)
{
	char res[BUFSIZ];

	if (req->params[0] == NULL) {
		fprintf(stderr, "ftps_cmd_pass_handle: password missing\n");
		return ftps_response(cli->fd, 501,
				"Syntax error in parameters or arguments.");
	}

	if (strcmp(password, req->params[0]) != 0) {
		fprintf(stderr, "ftps_cmd_pass_handle: incorrect password '%s'\n", req->params[0]);
		return ftps_response(cli->fd, 530, "Not logged in.");
	}

	sprintf(res, "%s logged in, proceed", req->params[0]);

	return ftps_response(cli->fd, 331, res);
}

