#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "common.h"
#include "client.h"

const long TIMEOUT =  10;
const double X1    = -10000;
const double X2    =  10000;
const double STEP  =  0.00001;

int prepare_sockets(int *sk_udp, int *sk_tcp)
{
DEBUG 	if (!sk_udp)	ERR_RETURN(ERR_ARG1);
DEBUG 	if (!sk_tcp)	ERR_RETURN(ERR_ARG2);

	struct sockaddr_in addr;
	SET_ADDR(addr, AF_INET, 0, htonl(INADDR_ANY));
	socklen_t sockaddr_len = sizeof(struct sockaddr_in);

//===== preparing upd socket =====
	int sk_udp_res = socket(AF_INET, SOCK_DGRAM, 0);
DEBUG 	if (sk_udp_res < 0) ERR_RETURN(ERR_SOCKET);	

	int int_true = 1;
	int err = setsockopt(sk_udp_res, SOL_SOCKET, SO_BROADCAST,
			     (const void*) &int_true, sizeof(int));
DEBUG 	if (err) ERR_RETURN(ERR_SETSOCKOPT);

	err = bind(sk_udp_res, (const struct sockaddr*) &addr, sockaddr_len);
DEBUG 	if (err) ERR_RETURN(ERR_BIND);

	err = getsockname(sk_udp_res,
			  (struct sockaddr*) &addr,
			  &sockaddr_len);
DEBUG 	if (err) ERR_RETURN(ERR_GETSOCKNAME);
//===== preparing tcp socket =====
	int sk_tcp_res = socket(AF_INET, SOCK_STREAM, 0);
DEBUG 	if (sk_tcp_res < 0) ERR_RETURN(ERR_SOCKET);	

	err = bind(sk_tcp_res, (const struct sockaddr*) &addr, sockaddr_len);
DEBUG 	if (err) ERR_RETURN(ERR_BIND);

	err = fcntl(sk_tcp_res, F_SETFL, O_NONBLOCK);
DEBUG 	if (err) ERR_RETURN(ERR_FCNTL);
	
	*sk_udp   = sk_udp_res;
	*sk_tcp   = sk_tcp_res;

	return 0;
}

int find_servers(int sk_udp,
		 int sk_tcp,
		 unsigned *nServers,
		 struct connection **connections)
{
DEBUG 	if (sk_udp < 0)	  ERR_RETURN(ERR_ARG1);
DEBUG 	if (sk_tcp < 0)	  ERR_RETURN(ERR_ARG2);
DEBUG 	if (!nServers)	  ERR_RETURN(ERR_ARG3);
DEBUG 	if (!connections) ERR_RETURN(ERR_ARG4);

//===== broadcasting =====
	struct sockaddr_in addr_for_broadcast;
	SET_ADDR(addr_for_broadcast,
		 AF_INET, htons(MAGIC_PORT), htonl(0xffffffff));

	int sockaddr_len = sizeof(struct sockaddr_in);
	int err = sendto(sk_udp, NULL, 0, 0,
		     (const struct sockaddr*) &addr_for_broadcast,
		     sockaddr_len);
DEBUG 	if (err < 0) ERR_RETURN(ERR_SENDTO);
	close(sk_udp);

//===== trying to establish tcp connections with servers =====
	err = establish_tcp(sk_tcp, nServers, connections);
DEBUG 	if (err) ERR_RETURN(ERR_ESTABLISH_TCP);

//===== collect info about servers =====
	err = collect_srv_info(*connections, *nServers);
DEBUG 	if (err) ERR_RETURN(ERR_COLLECT_SRV_INFO);

	return 0;
}

int establish_tcp(int sk_tcp,
		  unsigned *nServers,
		  struct connection **connections)
{
DEBUG 	if (sk_tcp < 0)	  ERR_RETURN(ERR_ARG1);
DEBUG 	if (!nServers)	  ERR_RETURN(ERR_ARG2);
DEBUG 	if (!connections) ERR_RETURN(ERR_ARG3);

	int err = listen(sk_tcp, MAX_NSERVERS);
DEBUG 	if (err) ERR_RETURN(ERR_LISTEN);

	struct connection *connections_res = \
		(struct connection*) \
		calloc (MAX_NSERVERS, sizeof(struct connection));
DEBUG 	if (!connections_res) ERR_RETURN(ERR_CALLOC);

	unsigned nServers_res = 0;
	int tmp = 0;
	sleep(1);
	while (1) {
		tmp = accept(sk_tcp, NULL, 0);
		if (tmp >= 0) {
			connections_res[nServers_res].sk = tmp;
			nServers_res++;
			if (nServers_res >= MAX_NSERVERS) break;
		}
		else break;
	}
	if (!nServers_res) {
		free(connections_res);
		printf("ERROR: HAVE NOT FOUND ANY SERVERS\n");
		ERR_RETURN(ERR_NO_SERVERS_FOUND);
	}
	*nServers = nServers_res;

	connections_res = (struct connection*) \
			realloc	\
			(connections_res, \
			nServers_res * sizeof(struct connection));
DEBUG 	if (!connections_res) {
		free(connections_res);
		ERR_RETURN(ERR_REALLOC);
	}
	*connections = connections_res;

	return 0;
}

int collect_srv_info(struct connection* connections, unsigned nServers)
{
DEBUG 	if (!connections) 			      ERR_RETURN(ERR_ARG1);
DEBUG 	if (nServers <= 0 || nServers > MAX_NSERVERS) ERR_RETURN(ERR_ARG2);

	struct timeval timeout = {1, 0};
	void *timeout_ptr = (void*) &timeout;
	unsigned i = 0;
	int err = 0;
	for (i = 0; i < nServers; i++) {
		err = setsockopt(connections[i].sk,
				 SOL_SOCKET,
				 SO_RCVTIMEO,
				 timeout_ptr,
				 sizeof(struct timeval));
DEBUG 		if (err) ERR_RETURN(ERR_SETSOCKOPT); 
	}

	unsigned buf = 0, buf_size = sizeof(unsigned);
	void *buf_ptr = (void*) &buf;

	for (i = 0; i < nServers; i++) {
		if (read(connections[i].sk, buf_ptr, buf_size) != buf_size)
			ERR_RETURN(ERR_READ);
		connections[i].nCores = buf;
	}

	return 0;
}

int send_tasks(struct connection *connections, unsigned nServers)
{
DEBUG 	if (!connections)			     ERR_RETURN(ERR_ARG1);
DEBUG 	if (nServers < 1 || nServers > MAX_NSERVERS) ERR_RETURN(ERR_ARG2);

	unsigned nParts = 0, i = 0;
	for (i = 0; i < nServers; i++)
		nParts += connections[i].nCores;
	double part_size = (X2 - X1) / nParts;
	double x = X1; //will be used for setting limits for servers

	unsigned srv_arg_size = sizeof(struct server_arg);
	struct server_arg buf;
	void *buf_ptr = (void*) &buf;

	for (i = 0; i < nServers; i++) {
		buf.x1 = x;
		x += part_size * connections[i].nCores;
		buf.x2 = x;
		buf.step = STEP;
		if (write(connections[i].sk, buf_ptr, srv_arg_size) \
		    != srv_arg_size)
			ERR_RETURN(ERR_WRITE);
	}

	return 0;
}

int recieve_results(struct connection *connections, unsigned nServers)
{
DEBUG 	if (!connections)			     ERR_RETURN(ERR_ARG1);
DEBUG 	if (nServers < 1 || nServers > MAX_NSERVERS) ERR_RETURN(ERR_ARG2);

//===== setting up the necessary timeout =====
	struct timeval timeout = {TIMEOUT, 0};
	void *timeout_ptr = (void*) &timeout;
	unsigned i = 0;
	int err = 0;
	for (i = 0; i < nServers; i++) {
		err = setsockopt(connections[i].sk,
				 SOL_SOCKET,
				 SO_RCVTIMEO,
				 timeout_ptr,
				 sizeof(struct timeval));
DEBUG 		if (err) ERR_RETURN(ERR_SETSOCKOPT); 
	}

//===== recieving results and checking servers =====
	unsigned nFinished = 0;
	unsigned *nFinished_ptr = &nFinished;
	while(1) {
		nFinished = 0;
		for (i = 0; i < nServers; i++) {
			err = get_msg(connections, i, nFinished_ptr);
			if (err == ERR_SERVER_CRASHED)
				ERR_RETURN(ERR_SERVER_CRASHED);
			if (err) ERR_RETURN(ERR_GET_MSG);
		}
		if (nFinished == nServers) break;
	}

	return 0;
}

int get_msg(struct connection *connections,
	    unsigned num,
	    unsigned *nFinished)
{
DEBUG 	if (!connections)	 ERR_RETURN(ERR_ARG1);
DEBUG 	if (num >= MAX_NSERVERS) ERR_RETURN(ERR_ARG2);
DEBUG	if (!nFinished)		 ERR_RETURN(ERR_ARG3);

	unsigned int_size = sizeof(unsigned), double_size = sizeof(double);
	void *buf = calloc(1, int_size + double_size);
DEBUG 	if (!buf) ERR_RETURN(ERR_CALLOC);

	int msg_type = 0, err = 0;
	if (connections[num].sk != -1) {
		err = read(connections[num].sk, buf, int_size);
		if (err != int_size) {
			free(buf);
			ERR_RETURN(ERR_SERVER_CRASHED);
		}
		msg_type = *((int*) buf);
		switch(msg_type) {
		case MSG_ALIVE:
			break;
		case MSG_RESULT:
			err =read(connections[num].sk, buf, double_size);
			if (err != double_size) {
				free(buf);
				ERR_RETURN(ERR_READ);
			}
			close(connections[num].sk);
			connections[num].sk = -1;
			connections[num].result = *((double*) buf);
			break;
		default:
			free(buf);
			ERR_RETURN(ERR_SWITCH);
		}
	}
	else *nFinished = *nFinished + 1;

	free(buf);
	return 0;
}

void* i_am_alive(void *arg)
{
	//ToDo: check at least something it the function
	struct alive_arg info = *((struct alive_arg*) arg);
	struct connection *connections = info.connections;
	unsigned nServers = info.nServers;

	unsigned i = 0, int_size = sizeof(int);
	int msg_type = MSG_ALIVE;
	void *msg_ptr = (void*) &msg_type;
	while (1) {
		sleep(I_AM_ALIVE_SLEEP);
		for (i = 0; i < nServers; i++)
			write(connections[i].sk, msg_ptr, int_size);
	}

	return NULL;
}

int run_client()
{
//add: close fds, free connections
	int sk_udp = 0, sk_tcp = 0;
	int err = prepare_sockets(&sk_udp, &sk_tcp);
	if (err) ERR_RETURN(ERR_PREPARE_SOCKETS);

	unsigned nServers = 0;
	struct connection *connections = NULL;
	err = find_servers(sk_udp, sk_tcp, &nServers, &connections);
	if (err) ERR_RETURN(ERR_FIND_SERVERS);

	//to exclude the chance to terminate because of SIGPIPE
	sigset_t set;
	sigset_t *set_ptr = &set;
	sigemptyset(set_ptr);
	sigaddset(set_ptr, SIGPIPE);
	err = pthread_sigmask(SIG_BLOCK, set_ptr, NULL);
	if (err) ERR_RETURN(ERR_PTHREAD_SIGMASK);

	err = send_tasks(connections, nServers);
	if (err) ERR_RETURN(ERR_SEND_TASKS);

	pthread_t thread_alive;
	struct alive_arg arg = {connections, nServers};
	err = pthread_create(&thread_alive, NULL, i_am_alive, (void*) &arg);
	if (err) ERR_RETURN(ERR_PTHREAD_CREATE);

	err = recieve_results(connections, nServers);
	if (err == ERR_SERVER_CRASHED) {
		printf("ERROR: ONE OF THE SERVERS HAS DISCONNECTED\n");
		ERR_RETURN(ERR_SERVER_CRASHED);
	}
	if (err) ERR_RETURN(ERR_RECIEVE_RESULTS);

	double result = 0;
	unsigned i = 0;
	for (i = 0; i < nServers; i++)
		result += connections[i].result;
	printf("RESULT: %lg\n", result);


//===== test tcp connection with all servers =====
	/*int i = 0;
	socklen_t sockaddr_len = sizeof(struct sockaddr_in);
	for (i = 0; i < nServers; i++) {
		char buf[13] = "HELLO SERVER";
		err = write(connections[i].sk, (void*)buf, 13);
		printf ("err: %d\n", err);
		sleep(1);
		err = read(connections[i].sk, (void*)buf, 13);
		printf ("err: %d\n", err);
		printf("%s\n", buf);

		sleep(1);
	}*/

//====================
	free(connections);
	return 0;
}
