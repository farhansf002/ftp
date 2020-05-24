#include <ftps.h>
#include <dirent.h>
#include <sys/stat.h>

static const char *username = "username";
static const char *password = "password";
static struct restart{
	char cmd[10];
	char filename[100];
	char done;
	char total;
} prev = {0};

const struct command commands[] = {
	{"USER", 	ftps_cmd_user_handle},
	{"PASS", 	ftps_cmd_pass_handle},
	{"RETR", 	ftps_cmd_retr_handle},
	{"LIST", 	ftps_cmd_list_handle},
	{"NLST", 	ftps_cmd_nlst_handle},
	{"QUIT", 	ftps_cmd_quit_handle},
	{"REST", 	ftps_cmd_rest_handle},
	{NULL, 		NULL}
};

static int check_eor_marker(char *buf, size_t size);
static void ftps_prev_cmd_init(struct ftp *req, struct client *cli);
static void ftps_prev_cmd_reset(struct client *cli);
static int file_size(char *filepath);
static int dir_isvalid(const char *path);
static int path_isvalid(const char *path);
static int send_file(int fd, const char *filename, long offset);


/**
 * This function dynamically allocates memory for a client struct,
 * initializes the client struct
 * fiinds first empty space in the clients array
 * stores it there and returns 0
 */
int ftps_newclient(struct client **clis, size_t size, int fd)
{
	int i =0;
	while(i < size && clis[i])
		i++;

	if ((clis[i] = calloc(1, sizeof(struct client))) == NULL) {
		log_error("ftps_newclient: malloc error %s", strerror(errno));
		return -1;
	}
	clis[i]->fd = fd;
	clis[i]->state = INVALIDUSER;

	return 0; /* 0 = OK, -1 = ERROR */
}

/**
 * Deleting a client from entry before closing it
 *
 */
 int ftps_delclient(struct client **clients, int index)
 {
	free(clients[index]);
	clients[index] = NULL;

	return 0; //sending the index of removed client
 }


/**
 * Handles the request of client (main request handler)
 *
 *
 */
int ftps_request_handle(struct client *cli)
{
	struct ftp req = {0};
	size_t nbytes = 0;
	char buf[BUFSIZ];
	int cmd_index;

	/* 1. Read from socket */
	// if FIN packet is recieved after client closed its connection
	if((nbytes = read(cli->fd, buf, BUFSIZ)) == -1) {
		log_error("ftps_request_handle: read error %s", strerror(errno));
		/* TODO Send appropriate error (letting client know error occured) */
		return -1;
	} else if (nbytes == 0) {
		return PEER_TERMINATED ;//-2
	}
	buf[nbytes] = 0;
	log_info("FTP command received: %s", buf);
	
	/* 2. Call parser and convert raw request to request struct */
	if (ftps_parse(buf, nbytes, &req) == -1) {
		/* TODO send 500 Syntax error response */
		return -1;
	}

	/* 3. Detect the request command */
	if ((cmd_index = ftp_command_find(&req)) == -1) {
		log_error("ftps_request_handle error: command %s not found", req.cmd);
		/* TODO Send 502 command not implemented response */
		return -1;
	}

	/* If callback is missing in handler then return error */
	if (commands[cmd_index].handler == NULL) {
		log_warning("Warning: function not implemented for command '%s'", req.cmd);
		/* TODO Send 502 command not implemented response */
		return -1;
	}

	/* 4. Call the appropriate request handler function */
	return commands[cmd_index].handler(&req, cli);
}

int ftps_response(int fd, int code, const char *msg)
{
	char buf[BUFSIZ];
	
	sprintf(buf, "%d %s\r\n", code, msg);

	if (write(fd, buf, strlen(buf)) == -1) {
		log_error("ftps_response: write error %s", strerror(errno));
		return -1;
	}

	return 0;
}

int ftps_parse(const char *buf, size_t size, struct ftp *req)
{
	char *ptr;
	long long code;
	char *bufcp = strdup(buf);

	if (bufcp == NULL) {
		log_error("ftps_parse: strdup error %s", strerror(errno));
		return -1;
	}

	if (check_eor_marker(bufcp, size) == -1) {
		log_error("ftp_request_parse: parser error: %s", buf);
		return -1;
	}

	// If command does not have any param then entire line is cmd
	ptr = strchr(bufcp, ' ');
	if (ptr == NULL) {
		req->cmd = bufcp;
		return 0;
	}

	// Check if command is numeric response of the form
	// <xyz> <string>.
	/* @Incomplete use better way to convert string to int */
	if (isdigit(bufcp[0])) {
		req->code = atoi(bufcp);
		free(bufcp);
		return 0;
	}

	req->cmd = bufcp;
	/* Handle string command of the form:
	 * <cmd> [<param>[ <param>..]] */
	 *ptr = 0;
	 ++ptr;
	 do {
		 while (ptr && isspace(*ptr))
			 ++ptr;
		if(*ptr != 0)
			req->params[req->nparam++] = ptr;
		ptr = strchr(ptr, ' ');
		if (ptr != NULL) {
			*ptr = 0;
			++ptr;
		}

	 } while (ptr);
	
	return 0;
}

static int check_eor_marker(char *buf, size_t size)
{
	char *ptr;

	if ((ptr = strchr(buf, '\n')) == NULL) return -1;
	if (*(ptr - 1) != '\r') return -1;

	*(ptr - 1) = 0;

	return 0;
}

int ftps_cmd_user_handle(struct ftp *req, struct client *cli)
{
	char res[BUFSIZ];

	if (req->params[0] == NULL) {
		log_error("ftps_cmd_user_handle: username missing");
		return ftps_response(cli->fd, 501, REPLY_501);
	}

	if (strcmp(username, req->params[0]) != 0) {
		log_error("ftps_cmd_user_handle: invalid username '%s'", req->params[0]);
		return ftps_response(cli->fd, 530, REPLY_530);
	}

	sprintf(res, REPLY_331, req->params[0]);

	return ftps_response(cli->fd, 331, res);
}

int ftps_cmd_pass_handle(struct ftp *req, struct client *cli)
{
	char res[BUFSIZ];

	if (req->params[0] == NULL) {
		log_error("ftps_cmd_pass_handle: password missing");
		return ftps_response(cli->fd, 230, REPLY_230);
	}

	if (strcmp(password, req->params[0]) != 0) {
		log_error("ftps_cmd_pass_handle: incorrect password '%s'", req->params[0]);
		return ftps_response(cli->fd, 530, REPLY_530);
	}

	sprintf(res, REPLY_331, req->params[0]);
	cli->state = VALIDUSER;

	return ftps_response(cli->fd, 331, res);
}

int ftps_cmd_retr_handle(struct ftp *req, struct client *cli)
{
	/*fprintf(stderr,"User requested for file Download\n");*/
	size_t count =0;
	char buf[BUFSIZ] = {0}, *filepath;
	int l = 0, ret =0;
	
	/*Handling the case when user did not send any paramter */
	if(req->nparam == 0)
	{
		ftps_response(cli->fd, 501, REPLY_501);
		goto cleanup;
	}
	filepath = req->params[0];


	/*1st request command handling*/
	if(cli->prev_cmd_count == 0) {
		log_info("Stage 1: executing..");
		if(ftp_is_file(filepath) != 1) {
			log_info("server responded: 501 not a file");
			ftps_response(cli->fd, 501, REPLY_501);
			ftps_prev_cmd_reset(cli);
			goto cleanup;
		}
		ftps_prev_cmd_init(req, cli);
		log_info("server responded: 200: Stage 1 cmd ok");
		ftps_response(cli->fd, 200, REPLY_200);
		goto cleanup;
	}

	/*2nd request command handling*/
	if(cli->prev_cmd_count == 1 && !strcmp(cli->prev_cmd, req->cmd)) {
		log_info("Stage 2: executing..");
		long size = file_size(filepath);
		prev.total = size;
		l = sprintf(buf, "%ld", size);
		buf[l++]= '\r';
		buf[l++]= '\n';
		buf[l++]= '\0';
		cli->prev_cmd_count=2;
		if(write(cli->fd, buf, l) == -1) {
			log_error("ftps_cmd_retr_handle:write error %s", strerror(errno));
			ret = -1;
			ftps_prev_cmd_reset(cli);
			/*send appropriate response msg*/
		}
		log_info("file size sent %ld", size);
		goto cleanup;
	}
	/*3rd request command handling*/
	if(cli->prev_cmd_count == 2 && !strcmp(cli->prev_cmd, req->cmd)) {
		strcpy(prev.cmd, req->cmd);
		log_info("Stage 3: executing (sending file)..");
		if(send_file(cli->fd, filepath, 0)==-1) {
			log_error("ftps_cmd_retr_handle:write error %s", strerror(errno));
			ret = -1;
			/*TODO imp checking 
			*ftps_prev_cmd_reset(cli);
			/*send appropriate response msg*/
		}
		cli->prev_cmd_count = 3;
		log_info("sent file '%s'", filepath);
		goto cleanup;
	}
	/*4th Request command handling*/
	if(cli->prev_cmd_count == 3 && !strcmp(cli->prev_cmd, req->cmd)) {
		log_info("Stage 4: executing (last stage)..");
		ftps_response(cli->fd, 226, REPLY_226);
		log_info(" file TRANSFERRED");
		memset(&prev, 0 , sizeof(prev));
	}
	ftps_prev_cmd_reset(cli);
	log_info("reset prev cmd");


	cleanup: 
		return ret;
}


int ftps_cmd_list_handle(struct ftp *req, struct client *cli)
{
	size_t count = 0;
	char buf[BUFSIZ] = {0}, *res = NULL, *filepath;
	int l = 0, ret = 0;
	/* If param count is 0 (zero) then send the dir list of current dir */
	if(req->nparam == 0)
		filepath = ".";
	else
		filepath = req->params[0];

	/* If it's first request for this command then send the reply and return */
	if (cli->prev_cmd_count == 0) {
		ftps_prev_cmd_init(req, cli);
		if(req->nparam == 1 && path_isvalid(filepath) == 0) {
			log_error("ftps_cmd_list_handle: filepath '%s' invalid", filepath);
			ftps_prev_cmd_reset(cli);
			ftps_response(cli->fd, 501, REPLY_501);
			goto cleanup;
		}
		ftps_response(cli->fd, 150, REPLY_150);
		goto cleanup;
	}

	/* It's command seq 2, but if prev command was not this then send error
	 * and return */
	if (strcmp(cli->prev_cmd, req->cmd) != 0) {
		ftps_prev_cmd_reset(cli);
		ftps_response(cli->fd, 503, REPLY_503);
		goto cleanup;
	}
	ftps_prev_cmd_reset(cli);

	/*if the paramater is file then return its size an return*/
	if(ftp_is_file(filepath) == 1) {
		l = ftp_file_details_get(filepath, &res);

	} else if ((l = ftp_dir_list_get(filepath, &res)) == 0) {
		log_error("opendir error: can't open %s: %s", filepath ,strerror(errno));
		ftps_response(cli->fd, 501, REPLY_501);
		goto cleanup;
	}

	if (write(cli->fd, res, l) == -1) {
		log_error("ftps_list_handle: write error %s", strerror(errno));
		ret = -1;
		goto cleanup;
	}


cleanup:
	if (res) free(res);

	return ret;
}

int ftps_cmd_nlst_handle(struct ftp *req, struct client *cli)
{
	size_t count = 0;
	char buf[BUFSIZ] = {0}, *dirls = NULL, *filepath;
	int l = 0, ret = 0;

	/* If it's first request for this command then send the reply and return */
	if (cli->prev_cmd_count == 0) {
		ftps_prev_cmd_init(req, cli);
		if ((req->nparam == 1 && dir_isvalid(req->params[0])) || req->nparam == 0) {
			ftps_response(cli->fd, 150, REPLY_150);
			goto cleanup;
		}
		ftps_response(cli->fd, 501, REPLY_501);
		ftps_prev_cmd_reset(cli);
		goto cleanup;
	}

	/* It's command seq 2, but if prev command was not this then send error
	 * and return */
	if (strcmp(cli->prev_cmd, req->cmd) != 0) {
		ftps_prev_cmd_reset(cli);
		ftps_response(cli->fd, 503, REPLY_503);
		goto cleanup;
	}
	ftps_prev_cmd_reset(cli);

	/* If param count is 0 (zero) then send the dir list of current dir */
	if(req->nparam == 0)
		filepath = ".";
	else
		filepath = req->params[0];

	if ((l = ftp_dir_list_get(filepath, &dirls)) == 0) {
		log_error("opendir error: can't open %s: %s", filepath, strerror(errno));
		ftps_response(cli->fd, 501, REPLY_501);
		ret = -1;
		goto cleanup;
	}

	if (write(cli->fd, dirls, l) == -1) {
		log_error("ftps_nlst_handle: write error %s", strerror(errno));
		ret = -1;
		goto cleanup;
	}

cleanup:
	if (dirls) free(dirls);

	return ret;
}

int ftps_cmd_quit_handle(struct ftp *req, struct client *cli)
{
	ftps_response(cli->fd, 221, REPLY_221);
	log_info("client %d requested to logout", cli->fd);

	return PEER_TERMINATED;
}

int ftps_cmd_rest_handle(struct ftp *req, struct client *cli)
{
	int ret =0;
	
	if(!cli->prev_cmd || strcmp(prev.cmd, cli->prev_cmd))
	{
		ftps_response(cli->fd, 503, REPLY_503);
		ret = -1;
		goto cleanup;
	}
	else{
		ftps_prev_cmd_init(req, cli);
		ftps_response(cli->fd, 200, REPLY_200);
		goto cleanup;
	}
	if(cli->prev_cmd_count == 1 && !strcmp(cli->prev_cmd, req->cmd)) {
		prev.done = atol(req->params[0]);
		if(send_file(cli->fd, prev.filename, prev.done) == -1) {
			log_error("ftps_cmd_rest_handle:write error %s", strerror(errno));
			ret = -1;
			/* ftps_prev_cmd_reset(cli);
			 * send appropriate response msg */
		}
		cli->prev_cmd_count = 2;
		goto cleanup;
	}
	if(cli->prev_cmd_count == 2 && !strcmp(cli->prev_cmd, req->cmd)) {
		log_info("Stage 3: executing (last stage)..");
		ftps_response(cli->fd, 226, REPLY_226);
		log_info(" file TRANSFERRED");
		memset(&prev, 0 , sizeof(prev));
	}
	ftps_prev_cmd_reset(cli);
	log_info("reset prev cmd");


cleanup:
	return ret;
}

static void ftps_prev_cmd_init(struct ftp *req, struct client *cli)
{
	cli->prev_cmd_count = 1;
	cli->prev_cmd = strdup(req->cmd);
}

static void ftps_prev_cmd_reset(struct client *cli)
{
	cli->prev_cmd_count = 0;
	if (cli->prev_cmd)
		free(cli->prev_cmd);
	cli->prev_cmd = NULL;
}
static int file_size(char *filepath)
{
	struct stat statbuf;

	if(stat(filepath, &statbuf) == -1)
		return -1;

	return statbuf.st_size; 
}

static int dir_isvalid(const char *path)
{
	DIR *dp;
	dp = opendir(path);
	if (dp != NULL) {
		closedir(dp);
		return 1;
	}

	return 0;
}

static int path_isvalid(const char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) == -1)
		return 0;

	return 1;
}
static int send_file(int fd, const char *filename, long offset)
{
	int ifd , nbytes;
	char buf[BUFSIZ] = {0};
	if ((ifd = open(filename, O_RDONLY)) == -1) {
		log_error("can't open the file %s", strerror(errno));
		return -1;
	}
	lseek(ifd, offset, SEEK_SET);
	while((nbytes = read(ifd, buf, BUFSIZ)) > 0) {
		if((nbytes = write(fd , buf, nbytes)) == -1) {
			return -1;
		}
	}
	if(nbytes== -1) {
		log_error("ftps_send_error %s", strerror(errno));
		return -1;
	}
	
	return 0;
}
