#ifndef	CLIENT_H
#define CLIENT_H

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
const double X1;
const double X2;
const double STEP;

enum CLIENT_CONST
{
	MAX_NSERVERS = 50,
	MAX_NSLEEPS = 3,
	CLIENT_MAGIC_PORT = 5555,
	I_AM_ALIVE_SLEEP = 2,
};

enum CLIENT_ERROR
{
	ERR_HNDL_SRV_RESPS = -299,
	ERR_PREPARE_SOCKETS,
	ERR_ESTABLISH_TCP,
	ERR_COLLECT_SRV_INFO,
	ERR_NO_SERVERS_FOUND = -295,
	ERR_SEND_TASKS,
	ERR_FIND_SERVERS,
	ERR_RECIEVE_RESULTS,
	ERR_RUN_CHECKERS,
	ERR_SERVER_CRASHED = -290,
	ERR_GET_MSG,
};

//=============================================================================
//============ STRUCTURES =====================================================
//=============================================================================
struct connection {
	int sk;
	int nCores;
	double result;
};

struct alive_arg {
	struct connection *connections;
	unsigned nServers;
};

//=============================================================================
//============ PROTOTYPES =====================================================
//=============================================================================
/**
 * @brief prepares udp-socket for broadcast and tcp-socket for listening to
 *        servers
 * 
 * @param sk_udp is used to save prepared udp-socket
 * @param sk_tcp is used to save prepared tcp-socket
 * 
 * @return 0 on success
 */
int prepare_sockets(int *sk_udp, int *sk_tcp);

/**
 * @brief finds servers, establishes tcp-connections with them, collects info
 * 	   about them
 * 
 * @param sk_udp is used to broadcast
 * @param sk_tcp is used to listen to servers
 * @param nServers is used to save the number of found servers
 * @param connections is used to save an array of info avout connections
 * 
 * @return 0 on success
 */
int find_servers(int sk_udp,
		 int sk_tcp,
		 unsigned *nServers,
		 struct connection **connections);

/**
 * @brief does listening and establishes to accepted servers
 * 
 * @param sk_tcp is used to listeninig to servers
 * @param nServers is used to save the number of accepted servers
 * @param connection is used to save an array of info avout connections
 * 
 * @return 0 on success
 */
int establish_tcp(int sk_tcp,
		  unsigned *nServers,
		  struct connection **connections);

/**
 * @brief learns numbers of threads, which servers are going to used
 * 
 * @param connection the array of found servers
 * @param nServers the number of found servers
 * 
 * @return 0 in success
 */
int collect_srv_info(struct connection* connections, unsigned nServers);

/**
 * @brief sends tasks to servers in accordance with the number of threads
 *  	  that they are going to used
 * 
 * @param connection the array of servers
 * @param nServers the number of servers
 * 
 * @return 0 on success
 */
int send_tasks(struct connection *connections, unsigned nServers);

/**
 * @brief waits for results from servers; fails, if at least one of servers
 *        doesn't send any news about his status more, than TIMEOUT seconds
 * 
 * @param connection the array of servers
 * @param nServers the number of servers
 * 
 * @return 0 on success
 */
int recieve_results(struct connection *connections, unsigned nServers);

/**
 * @brief gets a message from a server; recognizes two types: MSG_ALIVE
 * 	  and MSG_RESULT (see common.h)
 * 
 * @param connection the array of servers
 * @param nServers the number of servers
 * @param nFinished is used to increas thenumber of finished servers,
 *        if message type is MSG_RESULT
 *        
 * @return 0 on success
 */
int get_msg(struct connection *connections,
	    unsigned nServers,
	    unsigned *nFinished);

/**
 * @brief (is used in a thread) sends to servers message of MSG_ALIVE type
 *        every I_AM_ALIVE_SLEEP seconds
 * 
 * @param arg should be used as alive_arg* to get all necessary info about
 *        servers
 * 
 * @return on success: terminates, when the main thread terminates
 */
void* i_am_alive(void *arg);

/**
 * @brief runs a client, using argument presented by X1, X2, STEP
 *
 * @return 0 on success
 */
int run_client();

#endif