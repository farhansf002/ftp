#include <ftpc.h>
#include <sys/time.h>
#include <sys/select.h>

static int handle_stdin(int fd, struct client *cli);
static int handle_server_response(struct client *cli, struct ftp *req);
static void prompt(int validity);

int main()
{
	int sockfd, maxfd, nready;
	struct sockaddr_in addr;
	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	fd_set allset, rset;
	struct client cli = {0};

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(SERVER_PORT);
	if (inet_pton(AF_INET, SERVER_IP, &addr.sin_addr) <= 0) {
		perror("inet_pton error");
		exit(-1);
	}

	Connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));//creating connection
	cli.fd = sockfd;
	if (ftpc_cmd_auth_handle(NULL, &cli) == -1) {
		fprintf(stderr, "ftpc_cmd_auth_handle error\n");
		exit(-1);
	}
	fprintf(stderr, "You are authenticated\n");
	/* Prepare set */
	/*FD_ZERO(&allset);
	FD_SET(STDIN_FILENO, &allset);
	FD_SET(sockfd, &allset);
	maxfd = sockfd;*/
	for (; ;) {
	//	rset = allset;
		prompt(1);
	/*	nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		
		if (nready == -1) {
			perror("select error");
			continue;
		}*/

		//if (FD_ISSET(STDIN_FILENO, &rset)) {
			/* TODO handle quit command,
			 * If user typed QUIT then exit from loop and do the cleanup
			 * step. */
			handle_stdin(STDIN_FILENO, &cli);
		//}

		/*if (FD_ISSET(sockfd, &rset))*/
	}


	close(sockfd);// closing connection

	return 0;
}

static int handle_stdin(int fd, struct client *cli)
{
	int index;
	struct ftp req = {0};
	char buf[BUFSIZ];
	ssize_t nbytes;
	printf("\033[38;5;42m");
	fflush(stdout);

	if ((nbytes = read(fd, buf, sizeof(buf))) == -1) {
		perror("read error");
		return -1;
	}
	buf[nbytes] = 0;
	printf("\033[0m");
	fflush(stdout);

	if (ftpc_parse(buf, &req) == -1) {
		return -1;
	}

	/* If command is empty or contains only space then ignore and return */
	str_trim(req.cmd);
	if (!req.cmd[0])
		return 0;

	/* Detect the command using req struct and call appropriate
	 * command handler */
	 if ((index = ftpc_command_find(req.cmd)) == -1) {
		 fprintf(stderr, "\033[38;5;196mERROR \033[0mhandle_stdin: command '%s' not found\n", req.cmd);
		 return -1;
	 }

	 if ((nbytes = ftpc_prepare(&req, buf, sizeof(buf), index)) == -1)
		 return -1;

	 if (ftpc_request_send(cli->fd, buf, nbytes) == -1)
		 return -1;

	req.body = strdup(buf);

	return handle_server_response(cli, &req);
}

static int
handle_server_response(struct client *cli, struct ftp *req)
{
	int index;

	if ((index = ftpc_handle_find(req->cmd)) == -1) {
		fprintf(stderr, "\033[38;5;196mERROR\033[0mCommand '%s' not found in commands array\n", req->cmd);
		return -1;
	}

	if (commands[index].handler == NULL) {
		fprintf(stderr, "\033[38;5;196mERROR\033[0mHandler not implemented for command '%s'\n", req->cmd);
		return -1;
	}

	return commands[index].handler(req, cli);
}

static void prompt(int validity)
{
	if(validity)
		printf("\033[1m\033[33mftp> \033[0m");
	else
		printf("\033[1m\033[31mftp> \033[0m");
	fflush(stdout);
}

