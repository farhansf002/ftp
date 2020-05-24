#include <ftpc.h>
#include <easyio.h>
#include <errno.h>

static int check_eor_marker(char *buf, size_t size);
static int download(struct ftp *req, struct client *cli, long size);
static int redown(struct ftp *req, struct client *cli);

const struct command commands[] = {
	{"get", 	ftpc_cmd_get_handle},
	{"ls", 		ftpc_cmd_ls_handle},
	{"nls", 	ftpc_cmd_nls_handle},
	{"quit", 	ftpc_cmd_quit_handle},
	{"rest", 	ftpc_cmd_rest_handle},
	{NULL, 		NULL}
};

static char *usrcmds[][2] = {
	{"get", "RETR"},
	{"ls", 	"LIST"},
	{"nls", "NLST"},
	{"quit", "QUIT"},
	{"rest" , "REST"},
	{NULL, 	NULL}
};

struct restart {
	char cmd[10];
	char filename[100];
	long done;
	long total;
} prev = {0};

int ftpc_parse(char *cmdline, struct ftp *req)
{
	cmdline = strdup(cmdline);
	char *ptr;

	if (check_eor_marker(cmdline, strlen(cmdline)) == -1) {
		fprintf(stderr,
			"\033[1m\033[38;5;196mERROR\033[0m ftpc_parse: parser error: %s\n", cmdline);
		return -1;
	}

	/* If command line has no space then probably the given command
	 * does not have any params */
	ptr = strchr(cmdline, ' ');
	if (ptr == NULL) {
		req->cmd = cmdline;
		return 0;
	}

	/* Handle string command of the form:
	 * <cmd> [<param> [<param>..]] */
	 *ptr = 0;
	 req->cmd = cmdline;
	 if(strcmp(req->cmd, "rest") == 0) {
		 sprintf(req->params[req->nparam++], "%ld",prev.done);
		 return 0;
	 }
	 ++ptr;
	 do {
		 while (ptr && isspace(*ptr))
			 ++ptr;

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
	*ptr = 0;

	return 0;
}

int ftpc_cmd_auth_handle(struct ftp *req, struct client *cli)
{
	String username;
	String password;
	char buf[100];
	size_t nbytes;
	int i = 3;
	int res;
	char *ptr;
	int uok=0;

	while(i) {
		if(!uok) {

			printf("Username?: ");
			GetString(&username, stdin);

			nbytes = sprintf(buf,"USER %s\r\n", username);

			if (ftpc_request_send(cli->fd, buf, nbytes) == -1)
				return -1;

			if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
				perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_auth_handle: read error");
				return -1;
			} else if (nbytes == 0) {
				fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_auth_handle: server terminated\n");
				return -1;
			}

			buf[nbytes] = 0;

			/*Handle server response */
			ptr = strchr(buf , ' ');

			if(ptr)
				*ptr = 0;

			res= atoi(buf);
			if(res != 331) {
				fprintf(stderr,"Please enter a valid user name\n");
				i--;
				continue;
			}
			uok = 1;
		}


		password = getpass("Password?: ");
		nbytes = sprintf(buf,"PASS %s\r\n", password);

		if (ftpc_request_send(cli->fd, buf, nbytes) == -1)
			return -1;

		if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
			perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_auth_handle: read error");
			return -1;
		} else if (nbytes == 0) {
			fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_auth_handle: server terminated\n");
			return -1;
		}

		buf[nbytes] = 0;

		/*Handle server response */
		ptr = strchr(buf , ' ');

		if(ptr)
			*ptr = 0;

		res = atoi(buf);
		if(res != 331) {
			fprintf(stderr ,"Wrong password\n");
			i--;
			continue;
		}
		break;
	}
	if(!i)
		return -1;


	return 0;
}

int ftpc_request_send(int fd, const char *buf, size_t size)
{
	if (write(fd, buf, size) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_send_request: write error");
		return -1;
	}

	return 0;
}

int ftpc_cmd_get_handle(struct ftp *req, struct client *cli)
{
	char buf[BUFSIZ];
	char ftpcmd[100];
	int nbytes, code;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	char *p = strchr(buf, ' ');
	if(p)	p++;
	if (code > 299) {
		/* TODO
		 * Print the error message came from server as:
		 * ERROR: error message */
		 fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		if (req->body)
			free(req->body);
		return -1;
	}

	/* Send the 2nd request*/
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	p = strchr(buf, '\r');
	p = 0;
	long size = atol(buf);
	char unit[3];
	double s = show_unit(size, unit);
	printf("Filename: %s\t Size: %.2lf %s\n", req->params[0], s, unit);
	strcpy(prev.cmd , req->cmd);
	prev.total = size;
	prev.done = 0;

	/* Send the 3rd request*/
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;
	if(download(req, cli, size) == -1)//downloading the file
		return -1;

	memset(&prev, 0, sizeof(prev));
		
	/* Send the 4th request*/
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	if(code > 299) {
		p =strchr(buf, ' ');
		if(p)	p++;
		fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		return -1;
	}

	return 0;
}

int ftpc_cmd_ls_handle(struct ftp *req, struct client *cli)
{
	char buf[BUFSIZ];
	char ftpcmd[100];
	int nbytes, code;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR \033[0m ftpc_cmd_ls_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	if (code > 299) {
		char *p =strchr(buf, ' ');
		if(p)	p++;
		fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		if (req->body)
			free(req->body);
		return -1;
	}

	/* Send the request again */
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_ls_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	printf("%s\n" , buf);

	return 0;
}

int ftpc_cmd_nls_handle(struct ftp *req, struct client *cli)
{
	char buf[BUFSIZ];
	char ftpcmd[100];
	int nbytes, code;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR \033[0m ftpc_cmd_nls_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	if (code > 299) {
		char *p =strchr(buf, ' ');
		if(p)	p++;
		fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		if (req->body)
			free(req->body);
		return -1;
	}

	/* Send the request again */
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_nls_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	printf("%s\n" , buf);

	return 0;
}

int ftpc_cmd_quit_handle(struct ftp *req, struct client *cli)
{
	char buf[200];
	int nbytes , code;
	if ((nbytes = read(cli->fd, buf, 200)) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_quit_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	char *p;
	if (code > 299) {
		 p=strchr(buf, ' ');
		 p++;
		 fprintf(stderr, "%s\n", p);
		return -1;
	}
	printf("\033[1m\033[38;5;196mLogging out\033[0m\n");
	close(cli->fd);
	exit(0);
}


int ftpc_cmd_rest_handle(struct ftp *req, struct client *cli)
{
	char buf[BUFSIZ];
	char ftpcmd[100];
	int nbytes, code;
	char *p;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR \033[0m ftpc_cmd_rest_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	if (code > 299) {
		p =strchr(buf, ' ');
		if(p)	p++;
		fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		if (req->body)
			free(req->body);
		return -1;
	}

	/* Send the request 2nd time*/
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if (redown(req, cli) == -1) {
		return -1;
	}
	memset(&prev, 0, sizeof(prev));
	/* Send the request 3rd time*/
	if (ftpc_request_send(cli->fd, req->body, strlen(req->body)) == -1) 
		return -1;

	if ((nbytes = read(cli->fd, buf, sizeof(buf))) == -1) {
		perror("\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_rest_handle: read error");
		return -1;
	}
	buf[nbytes] = 0;
	code = atoi(buf);
	if(code > 299) {
		p =strchr(buf, ' ');
		if(p)	p++;
		fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m %s", p);
		return -1;
	}

	return 0;


}


int ftpc_command_find(const char *cmd)
{
	for (int i = 0; usrcmds[i][0]; ++i) {
		if (strcmp(cmd, usrcmds[i][0]) == 0)
			return i;
	}

	return -1;
}

int ftpc_handle_find(const char *cmd)
{
	for (int i = 0; commands[i].cmd; ++i) {
		if (strcmp(cmd, commands[i].cmd) == 0)
			return i;
	}

	return -1;
}

ssize_t ftpc_prepare(struct ftp *req,
		char *buf, ssize_t buflen, int cmdindex)
{
	int len;
	sprintf(buf, "%s", usrcmds[cmdindex][1]);
	for (int i = 0; i < req->nparam; ++i) {
		strcat(buf, " ");
		strcat(buf, req->params[i]);
	}

	len = strlen(buf);
	buf[len++] = '\r';
	buf[len++] = '\n';
	buf[len++] = '\0';

	return len;
}
static int download(struct ftp *req, struct client *cli, long size)
{

	if(is_dir("Downloads") == 0)
	{
		if(mkdir("Downloads", 0777) == -1) {
			fprintf(stderr,"\033[1m\033[38;5;196mERROR\033[0m can't create downloads folder");
			return -1;
		}
	}
	char *filename, fn[100] , down[100];
	char *p ;
	char buf[BUFSIZ];
	size_t nbytes;
	p = req->params[0];
	while(strchr(p, '/')) {
		p = strchr(p, '/');
		p++;
	}
	filename = strdup(p);

	strcpy(fn, "Downloads/.");
	strcat(fn, filename);
	strcat(fn, ".part");
	strcpy(prev.filename, fn);	
	int fp = open(fn , O_RDWR | O_CREAT | O_TRUNC, 0644);
	if(fp == -1) {

		fprintf(stderr , "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle:download: open error\n");
		return -1;
	}
	char progres[100];
	long done = 0;

	while(done < size && ((nbytes = read(cli->fd , buf , sizeof(buf))) > 0)) {  //reading the contents of the file
		done += nbytes;
		prev.done = done;

		if (write(fp , buf , nbytes) == -1) {
			fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m write error:download: %s", strerror(errno));
			return -1;
		}
		progress_bar(progres, size, done, 99);
		printf("\r\033[1m\033[32m%s\033[0m", progres);
		fflush(stdout);
	}
	if(nbytes < 0) {
		fprintf(stderr , "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle:download: read error\n");
		perror("");
		return -1;
	}
	close(fp);
	strcpy(down, "Downloads/");
	strcat(down, filename);
	rename(fn, down);
	strcpy(prev.filename, down);
	free(filename);
	printf("\r");
	for(int i = 0; i<100 ; i++)
		printf(" ");
	printf("\r");
	printf("Download complete\n");
}
static int redown(struct ftp *req, struct client *cli)
{

	if(is_dir("Downloads") == 0)
	{
		if(mkdir("Downloads", 0777) == -1) {
			fprintf(stderr,"\033[1m\033[38;5;196mERROR\033[0m can't create downloads folder\n");
			return -1;
		}
	}
	char  fn[100] , down[100];
	char *p ;
	size_t nbytes;
	char buf[BUFSIZ];
	if(!prev.cmd)
	{
		fprintf(stderr,"\033[1m\033[38;5;196mERROR\033[0m previous command was not get or no incomplete download\n");

	}
	int fp = open(fn , O_RDWR | O_CREAT | O_APPEND, 0644);
	if(fp == -1) {

		fprintf(stderr , "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_rest_handle:download: open error\n");
		return -1;
	}
	char progres[100];
	long done = prev.done;
	long size = prev.total;
	lseek(fp, 0, SEEK_END);

	while(done < size && ((nbytes = read(cli->fd , buf , sizeof(buf))) > 0)) {  //reading the contents of the file
		done += nbytes;
		prev.done = done;

		if (write(fp , buf , nbytes) == -1) {
			fprintf(stderr, "\033[1m\033[38;5;196mERROR\033[0m write error:download: %s\n", strerror(errno));
			return -1;
		}
		progress_bar(progres, size, done, 99);
		printf("\r\033[1m\033[32m%s\033[0m", progres);
		fflush(stdout);
	}
	if(nbytes < 0) {
		fprintf(stderr , "\033[1m\033[38;5;196mERROR\033[0m ftpc_cmd_get_handle:download: read error\n");
		perror("");
		return -1;
	}
	close(fp);
	int s = strlen(prev.filename);
	char filename[100];
	for(int i = s-1 ; i >=0 ; i--)
	{
		if(prev.filename[i] == '.')
		{
			prev.filename[i] = 0;
			strcpy(filename, prev.filename);
			prev.filename[i] = '.';
			break;
		}
	}
	s = strlen(filename);
	int flag = 0;
	for(int i = 0; i<s ; i++)
	{
		if(filename[i] == '.' || flag)
		{
			filename[i] = filename[i+1];
			flag = 1;
		}
	}
	rename(prev.filename, filename);
	strcpy(prev.filename, filename);
	printf("\r");
	for(int i = 0; i<100 ; i++)
		printf(" ");
	printf("\r");
	printf("Download complete\n");
}
