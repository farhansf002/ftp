#include <ftp.h>
#include <ftps.h>
#include <regex.h>

#define LOG_FILE "ftp.log"

static void handle_request();

static int listenfd, nready;
static fd_set rset, allset;

/* non-NULL  pointer in clients[i] has connected client struct
 * nclient   is total number of active clients in clients */
static struct client *clients[FD_SETSIZE];
static size_t nclient;


int main(int argc, char *argv[])
{
	int connfd, maxfd;
	struct sockaddr_in clientaddr, *tmpaddr;
	socklen_t clientlen;
	char ipstr[200];

	listenfd = Tcp_Listen(SERVER_IP, SERVER_PORT);

	// Setup read set for select
	maxfd = listenfd;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	log_open(LOG_FILE);

	log_info("Server running...");
	for (; ;) {
		clientlen = sizeof(clientaddr);
		// Wait for rset to be ready
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		/* If New connection arrived handle it */
		if (FD_ISSET(listenfd, &rset)) {
			if ((connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen)) == -1) {
				log_error("accept error %s", strerror(errno));
			}
			tmpaddr = (struct sockaddr_in *) &clientaddr;
			log_info("New connection accepted");
			if (inet_ntop(tmpaddr->sin_family, &tmpaddr->sin_addr, ipstr, sizeof(ipstr)) == NULL) {
				log_error("...inet_ntop failed to convert ip");
			} else {
				log_info(" from (%s)...\n", ipstr);
			}
			
			/* If client limit reached then don't accept the new connection */
			if (nclient + 1 == FD_SETSIZE) {
				log_warning("Client limit exceeded, rejecting new connection.");
				/* TODO Need to tell client, your connection request rejected */
				close(connfd);
			} else {
			
				/* Save new client details in first empty space */
				if (ftps_newclient(clients, FD_SETSIZE, connfd) == -1) {
					log_error("ftps_newclient error: failed to create new client\n");
					/* TODO refuse the connection with reply message */
					close(connfd);
					continue;
				}
				++nclient;
				FD_SET(connfd, &allset);

				// Increment maxfd if required
				if (connfd > maxfd)
					maxfd = connfd;
			}

			/* If only new connection arrived then skip iteration on
			 * clients below */
			if (--nready == 0)
				continue;
		}

		log_info("Handling request");
		handle_request();

	}
}

static void handle_request()
{
	struct client *tmp;
	/* Find ready socket and handle the client request */
	for (int i = 0; i < FD_SETSIZE && nready; i++) {
		tmp = clients[i];
		if (tmp == NULL)
			continue;
		/* If client's socket is ready then process it */
		if (FD_ISSET(tmp->fd, &rset)) {
			/* If client terminated then clean client details */
			log_info("Processing client request...");
			if (ftps_request_handle(tmp) == PEER_TERMINATED) {
				/* Close the connection */
				int fd = tmp->fd;
				ftps_delclient(clients, i);
				FD_CLR(fd, &allset);
				--nclient;
				log_info("Client %d terminated\n", fd);
			}
			--nready;
		}
	}
}
