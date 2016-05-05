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
#include "common.h"
#include "server.h"

const long TIMEOUT = 5;
const int SEC_TO_NANOSEC = 1000000000;

int prepare_sockets(int *sk_udp, int *sk_tcp)
{
DEBUG 	if (!sk_udp)	ERR_RETURN(ERR_ARG1);
DEBUG 	if (!sk_tcp) 	ERR_RETURN(ERR_ARG2);

	struct sockaddr_in addr;
	SET_ADDR(addr, AF_INET, htons(MAGIC_PORT), htonl(INADDR_ANY));

//===== preparing upd socket =====
	int sk_udp_res = socket(AF_INET, SOCK_DGRAM, 0);
DEBUG 	if (sk_udp_res < 0) ERR_RETURN(ERR_SOCKET);
	
	int sockaddr_len = sizeof(struct sockaddr_in);
	int err = bind(sk_udp_res,
		       (const struct sockaddr*) &addr,
		       sockaddr_len);
DEBUG 	if (err) ERR_RETURN(ERR_BIND);

//===== preparig tcp socket =====
	int sk_tcp_res = socket(AF_INET, SOCK_STREAM, 0);
DEBUG 	if (sk_tcp_res < 0) ERR_RETURN(ERR_SOCKET);

	int int_true = 1;
	err = setsockopt(sk_tcp_res, SOL_SOCKET, SO_REUSEADDR,
			 (void*) &int_true, sizeof(int));
DEBUG 	if (err) ERR_RETURN(ERR_SETSOCKOPT);
	//Probably, SO_REUSEPORT is not necessary
	err = setsockopt(sk_tcp_res, SOL_SOCKET, SO_REUSEPORT,
			 (void*) &int_true, sizeof(int));
DEBUG 	if (err) ERR_RETURN(ERR_SETSOCKOPT);

	err = bind(sk_tcp_res,
		   (const struct sockaddr*) &addr,
		   sockaddr_len);
DEBUG 	if (err) ERR_RETURN(ERR_BIND);

//====================
	*sk_udp   = sk_udp_res;
	*sk_tcp   = sk_tcp_res;

	return 0;
}

int wait_for_client(int sk_udp, struct sockaddr_in *addr_client)
{
DEBUG 	if (sk_udp < 0)	  ERR_RETURN(ERR_ARG1);
DEBUG 	if (!addr_client) ERR_RETURN(ERR_ARG2);

	socklen_t sockaddr_len = sizeof(struct sockaddr_in);
	struct sockaddr_in addr_client_res;
	int err = recvfrom(sk_udp, NULL, 0, 0,
			   (struct sockaddr*) &addr_client_res,
			   &sockaddr_len);
 	if (err < 0) ERR_RETURN(ERR_RECVFROM);
	close(sk_udp);

	*addr_client = addr_client_res;

	return 0;
}

int establish_connection(int sk_tcp,
			 struct sockaddr_in *addr_client,
			 unsigned nCores)
{
DEBUG 	if (sk_tcp < 0)			ERR_RETURN(ERR_ARG1);
DEBUG 	if (!addr_client)		ERR_RETURN(ERR_ARG2);
DEBUG 	if (nCores < 1 || nCores > 128) ERR_RETURN(ERR_ARG3);
	
	int err = 0, nSleeps = 0;
	while (1) {
		err = connect(sk_tcp,
			      (const struct sockaddr*) addr_client,
			      sizeof(*addr_client));
		if (err) {
			if (nSleeps >= MAX_NSLEEPS) break;
			sleep(1);
			nSleeps++;
		}
		else break;
	}
	//printf("NSLEEPS: %d\n", nSleeps);
	if (err) ERR_RETURN(ERR_CONNECT);

	unsigned nCores_buf = nCores;
	err = write(sk_tcp, (void*) &nCores_buf, sizeof(unsigned));
DEBUG 	if (err != sizeof(unsigned)) ERR_RETURN(ERR_WRITE);

	return 0;
}

void* checker_func(void* arg)
{
	//ToDo: check at least something it the function
	int sk_tcp = *((int*) arg);

	struct timeval timeout = {TIMEOUT, 0};
	int err = setsockopt(sk_tcp,
			 SOL_SOCKET,
			 SO_RCVTIMEO,
			 (void*) &timeout,
			 sizeof(struct timeval));

	unsigned int_size = sizeof(int);
	void *buf = calloc(1, int_size);

	while (1) {
		err = read(sk_tcp, buf, int_size);
		if (err < 0) {
			close(sk_tcp);
			kill(getpid(), SIGUSR2);
		}
		//printf("ERROR [%d]\n", err);
		err = write(sk_tcp, buf, int_size);
		sleep(3);
	}

	free(buf);
	return NULL;
}

int recieve_task(int sk_tcp, struct server_arg *srv_arg)
{
DEBUG 	if (sk_tcp < 0)	ERR_RETURN(ERR_ARG1);
DEBUG 	if (!srv_arg)	ERR_RETURN(ERR_ARG2);

	struct timeval timeout = {3, 0};
	int err = setsockopt(sk_tcp,
	 		     SOL_SOCKET,
	 		     SO_RCVTIMEO,
			     (void*) &timeout,
			     sizeof(struct timeval));
DEBUG	if (err) ERR_RETURN(ERR_SETSOCKOPT);

	unsigned arg_size = sizeof(struct server_arg);
	void *buf = calloc(1, arg_size);
DEBUG 	if (!buf) ERR_RETURN(ERR_CALLOC);

	err = read(sk_tcp, buf, arg_size);
	if (err < 0) {
		free(buf);
		ERR_RETURN(ERR_READ);
	}
	*srv_arg = *((struct server_arg*) buf);

	free(buf);
	return 0;
}

int calculate_and_send(int sk_tcp, struct server_arg *arg, unsigned nCores)
{
DEBUG 	if (sk_tcp < 0) 		ERR_RETURN(ERR_ARG1);
DEBUG 	if (!arg) 			ERR_RETURN(ERR_ARG2);
DEBUG 	if (nCores < 1 || nCores > 128)	ERR_RETURN(ERR_ARG3);

	double res = simpson_multi(foo, arg->x1, arg->x2, arg->step, nCores);

	unsigned int_size = sizeof(int);
	void *buf = calloc(1, int_size + sizeof(double));
DEBUG 	if (!buf) ERR_RETURN(ERR_CALLOC);
	
	*((int*) buf) = MSG_RESULT;
	*((double*) (buf + int_size)) = res;
	int err = write(sk_tcp, buf, int_size + sizeof(double));
	free(buf);
DEBUG 	if (err < 0) {
		sleep(1);
		close(sk_tcp);
		ERR_RETURN(ERR_WRITE);
	}

	close(sk_tcp);
	return 0;
}

void* simpson_thread(void* data)
{
	//clock_gettime is badly portable...
	/*struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);*/

	struct thread_simpson_arg *my_arg = (struct thread_simpson_arg*) data;
	double (*func)(double arg) = my_arg->func;
	double x1 = my_arg->x1;
	double x2 = my_arg->x2;
	double step = my_arg->step;
	double tmp_sum = 0;

	double coef_1 = step / 6;
	double coef_2 = step / 2;
	while (x1 + step < x2) {
		tmp_sum += coef_1 * (
				       func (x1)
			      	     + func (x1 + step)
			      	     + 4 * func (x1 + coef_2));
		x1 += step;
	}

	*(my_arg->res) += tmp_sum;

	/*clock_gettime(CLOCK_REALTIME, &end);
	double thread_time =
		(double) (end.tv_sec - start.tv_sec)		\
		* SEC_TO_NANOSEC				\
	     	+ (double) (end.tv_nsec - start.tv_nsec);
	printf ("thread_time: %lg\n", thread_time);*/

	return NULL;
}

double simpson_multi(double (*func)(double arg),
		   double x1,
		   double x2,
		   double step,
		   unsigned nthreads)
{
//DEBUG	ASSERT (func, "func NULL pointer");
//DEBUG	ASSERT (x1 <= x2, "x1 > x2");

	pthread_t threads[nthreads];
	struct thread_simpson_arg *thread_args = (struct thread_simpson_arg*)\
		calloc (nthreads, sizeof(struct thread_simpson_arg));

	double	threads_results[nthreads];
	double  from_arg = x1;
	double  part_size = (x2 - x1) / nthreads;

	//check pthread_create
	#define _CALL_THREAD_SIMPSON(x2_arg)				\
	do {								\
		thread_args[i].func = foo;				\
		thread_args[i].x1   = from_arg;				\
		from_arg = from_arg + part_size;			\
		thread_args[i].x2   = x2_arg;				\
		thread_args[i].step = step;				\
		thread_args[i].res  = threads_results + i;		\
		pthread_create(&threads[i],				\
				NULL,					\
				simpson_thread,				\
				(void*) (thread_args + i));} while (0)

	unsigned tmp = nthreads - 1;
	unsigned i = 0;
	for (i = 0; i < tmp; i++) {
		_CALL_THREAD_SIMPSON(from_arg);
	}
	_CALL_THREAD_SIMPSON(x2);
	#undef _CALL_THREAD_SIMPSON

	for (i = 0; i < nthreads; i++)  pthread_join (threads[i], NULL);
	double result = 0;
	for (i = 0; i < nthreads; i++)  result += threads_results[i];
	free (thread_args);
	return  result;
}

int run_server(int nCores)
{
DEBUG 	if (nCores <= 0 || nCores > 128) ERR_RETURN(ERR_ARG1);

	int sk_udp = 0, sk_tcp = 0;
	int err = prepare_sockets(&sk_udp, &sk_tcp);
	if (err) {
		close(sk_tcp);
		close(sk_udp);
		ERR_RETURN(ERR_PREPARE_SOCKETS);
	}

	struct sockaddr_in addr_client;
	err = wait_for_client(sk_udp, &addr_client);
	if (err) {
		close(sk_tcp);
		close(sk_udp);
		ERR_RETURN(ERR_GET_CLIENT_INFO);
	}

	//to exclude the chance to terminate because of SIGPIPE
	sigset_t set;
	sigset_t *set_ptr = &set;
	sigemptyset(set_ptr);
	sigaddset(set_ptr, SIGPIPE);
	err = pthread_sigmask(SIG_BLOCK, set_ptr, NULL);
	if (err) ERR_RETURN(ERR_PTHREAD_SIGMASK);

	err = establish_connection(sk_tcp, &addr_client, nCores);
	if (err) {
		close(sk_tcp);
		ERR_RETURN(ERR_ESTABLISH_TCP);
	}

	struct server_arg arg;
	err = recieve_task(sk_tcp, &arg);
	if (err) {
		close(sk_tcp);
		ERR_RETURN(ERR_RECIEVE_TASK);
	}

	pthread_t checker;
	err = pthread_create(&checker, NULL, checker_func, (void*) &sk_tcp);
DEBUG 	if (err) ERR_RETURN(ERR_PTHREAD_CREATE);

	err = calculate_and_send(sk_tcp, &arg, nCores);
	if (err) {
		close(sk_tcp);
		ERR_RETURN(ERR_CALCULATE_AND_SEND);
	}

//===== testing tcp and udp connections with a client =====
	/*char buf[13] = "HELLO CLIENT";
	err = write(sk_tcp, (void*)buf, 13);
	printf ("err: %d\n", err);
	sleep(1);
	err = read(sk_tcp, (void*)buf, 13);
	printf ("err: %d\n", err);
	printf("%s\n", buf);
	sleep(1);

	socklen_t sockaddr_len = sizeof(struct sockaddr_in);
	err = sendto(sk_udp, buf, 13, 0,
			     (const struct sockaddr*)
			     &addr_client_udp,
			     sockaddr_len);
	printf ("err: %d\n", err);
	sleep(1);
	err = recvfrom(sk_udp, buf, 13, 0, NULL, 0);
	printf ("err: %d\n", err);
	printf("%s\n", buf);*/

	close(sk_tcp);
	return 0;
}

double foo(double x)
{
	return x * x;
}
