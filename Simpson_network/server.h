#ifndef	SERVER_H
#define SERVER_H

//=============================================================================
//============ MODS ===========================================================
//=============================================================================

//=============================================================================
//============ DEFINES ========================================================
//=============================================================================

//=============================================================================
//============ CONSTS =========================================================
//=============================================================================
const long TIMEOUT;

enum SERVER_CONST
{
	MAX_NSLEEPS = 1,
	CHECKER_SLEEP = 3,
};

enum SERVER_ERROR {
	ERR_ESTABLISH_CONNECTION = -199,
	ERR_PREPARE_SOCKETS,
	ERR_GET_CLIENT_INFO,
	ERR_ESTABLISH_TCP,
	ERR_SEND_INFO,
	ERR_RECIEVE_TASK,
	ERR_CALCULATE_AND_SEND,
	ERR_FINISH,
	ERR_RUN_CHECKER,
};

//=============================================================================
//============ STRUCTURES =====================================================
//=============================================================================
struct thread_simpson_arg {
	double (*func)(double x);
	double x1;
	double x2;
	double step;
	double *res;
};

//=============================================================================
//============ PROTOTYPES =====================================================
//=============================================================================
/**
 * @brief prepares udp-socket for waiting for a broadcast from a client and
 * 	  tcp-socket for connecting to this client
 * 
 * @param sk_udp is used to save prepared udp-socket
 * @param sk_tcp is used to save prepared tcp-socket
 * 
 * @return 0 on success
 */
int prepare_sockets(int *sk_udp, int *sk_tcp);

/**
 * @brief waits fot a broadcast from a client
 * 
 * @param sk_udp the udp-socket, used for waiting for a broadcast from a client
 * @param sockaddr_in is used to save the addr of a client that did the
 * 	  broadcast
 * 
 * @return 0 on success
 */
int wait_for_client(int sk_udp, struct sockaddr_in *addr_client);

/**
 * @brief establishes tcp-connection with a client and
 *        sends the number of threads, which are going to be used
 * 
 * @param sk_tcp the tcp-sokcket for connecting to a client
 * @param sockaddr_in the client's addr
 * @param nCores the number of threads, which are going to be used
 * 
 * @return 0 on success
 */
int establish_connection(int sk_tcp,
			 struct sockaddr_in *addr_client,
			 unsigned nCores);

/**
 * @brief recieves task from a client
 * 
 * @param sk_tcp the tcp-socket that is used for the connection with a client
 * @param srv_arg is used to save the task
 * 
 * @return 0 on success: terminates, when the main thread terminates
 */
int recieve_task(int sk_tcp, struct server_arg *srv_arg);

/**
 * @brief calculates the integral of function foo in accordace to the task
 * 
 * @param sk_tcp the tcp-socket that is used for the connection with a client
 * @param srv_arg points to the task
 * @param nCores the number of threads, which are going to be used
 * 
 * @return 0 on success
 */
int calculate_and_send(int sk_tcp, struct server_arg *arg, unsigned nCores);

/**
 * @brief (is used in a thread) sends a message of type MSG_ALIVE every
 *        CHECKER_SLEEP seconds; kills the server if the client doesn't respond
 *        for more than TIMEOUT seconds
 * 
 * @param thread_arg should be used as int* to get the tcp-socket that is used
 * 	  for the connection
 * 
 * @return NEVER returns on success
 */
void* checker_func(void* thread_arg);

/**
 * @brief runs a server, which will use nCores threads to calculate
 * @param nCores the number of threads, which the server will use, allowed from
 * 		 1 to 128
 * @return 0 on success
 */
int run_server(int nCores);

/**
 * @brief calculates integral using Simpson's formula effectively
 * @details uses threads
 * 
 * @param func a pointer of a math double-func, which needs one double-argument
 * @param x1 the beginning of the interval
 * @param x2 the end of the interval
 * @param nthreads a positive number of threads, which the function will use
 * @return the result of calculating the integral of the func
 *         on the interval [x1, x2]
 */
double simpson_multi(double (*func) (double arg),
		   double x1,
		   double x2,
		   double step,
		   unsigned nthreads);

void* simpson_thread(void* data);

/**
 * @brief It's a usual math function
 * 
 * @param x real number, should be possible to be used in function
 * 
 * @return the result of calculating the function ;)
 */
double foo(double x);

#endif