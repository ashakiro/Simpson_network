#ifndef	COMMON_H
#define COMMON_H

//=============================================================================
//============ MODS ===========================================================
//=============================================================================
#define DEBUG_MOD
#ifdef	DEBUG_MOD
#define DEBUG
#else	
#define DEBUG if (0)
#endif

//=============================================================================
//============ DEFINES ========================================================
//=============================================================================
#define ERR_RETURN(err_code)						\
 	do {								\
 		printf("ERROR [%d]: file [%s], function [%s], line [%d], ",\
 			err_code, __FILE__, __FUNCTION__, __LINE__);	\
 		perror("");						\
 		printf("\n");						\
 		return err_code;					\
 	} while (0);

#define SET_ADDR(addr, new_sin_family, new_sin_port, new_s_addr)	\
 	do {								\
 		addr.sin_family = new_sin_family;			\
 		addr.sin_port   = new_sin_port;				\
 		addr.sin_addr.s_addr = new_s_addr;			\
 	} while (0);
//=============================================================================
//============ CONSTS =========================================================
//=============================================================================
enum COMMON_CONST
{
	MAGIC_PORT = 4444,
	MSG_ALIVE,
	MSG_RESULT,
};

enum ERROR 
{
	ERR_ARG1   = -101,
	ERR_ARG2   = -102,
	ERR_ARG3   = -103,
	ERR_ARG4   = -104,
	ERR_ARG5   = -105,
	ERR_CALLOC = -99,
	ERR_SOCKET,
	ERR_SENDTO,
	ERR_REALLOC,
	ERR_BIND,
	ERR_FCNTL,
	ERR_RECVFROM,
	ERR_STR_TO_LONG,
	ERR_SETSOCKOPT,
	ERR_LISTEN,
	ERR_PTHREAD_CREATE = -89,
	ERR_PTHREAD_JOIN,
	ERR_READ,
	ERR_CONNECT,
	ERR_WRITE,
	ERR_GETSOCKNAME,
	ERR_CLOSE,
	ERR_SWITCH,
	ERR_FORK,
	ERR_PTHREAD_SIGMASK,
};
//=============================================================================
//============ STRUCTURES =====================================================
//=============================================================================
struct server_arg {
	double x1;
	double x2;
	double step;
};
//=============================================================================
//============ PROTOTYPES =====================================================
//=============================================================================
/**
 * @brief converts a string to a number
 * @param str [pointer to the string]
 * @param num [will be used to save the result]
 * @return [0 on success, -1 if arguments are bad, -2 if strtol fails]
 */
int str_to_long (const char *str, long *num);


#endif