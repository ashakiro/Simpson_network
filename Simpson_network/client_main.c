#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include "common.h"
#include "client.h"

int main()
{
	//while (1) printf("CLIENT FINISHED: %d\n", run_client());
	printf("CLIENT FINISHED: %d\n", run_client());
	
	return 0;
}