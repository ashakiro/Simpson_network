	#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common.h"
#include "server.h"

int main(int argc, char const *argv[])
{
	if (argc != 2) {
		printf("The server needs two arguments to run\n");
		return -1;
	}

	long nCores = 0;
	int err = str_to_long(argv[1], &nCores);
DEBUG 	if (err) ERR_RETURN(ERR_STR_TO_LONG);
	if (nCores <= 0 || nCores > 128) {
		printf("The number of threads must be from 1 to 128");
		return -1;
	}

	while (1) {
		err = fork();
		if (err < 0) ERR_RETURN(ERR_FORK);
		if (!err) {
			printf("SERVER FINISHED: %d\n",
			       run_server((int) nCores));
			return 0;
		}
		int child_status = 0;
		waitpid(-1, &child_status, 0);
		if (WIFSIGNALED(child_status)
		    && WTERMSIG(child_status) == SIGUSR2)
			printf("ERROR: CLIENT DISCONNECTED. RELOADING...\n");
	}	

	return 0;
}

int str_to_long(const char *str, long *num)
{
	if (!str || !num) return -1;

	char *end_ptr = NULL;
	*num = strtol(str, &end_ptr, 10);
	if (*end_ptr || errno == ERANGE) return -2;

	return 0;
}