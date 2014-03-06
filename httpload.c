#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "libtsc.h"


char *server;
char *url;

int fetchtask(void*);

int user_main(int argc, char **argv)
{
	int i, n;
	
	if(argc != 4){
		fprintf(stderr, "usage: httpload n server url\n");
		thread_exit(-1);
	}
	n = atoi(argv[1]);
	server = argv[2];
	url = argv[3];

	for(i=0; i<n; i++){
		thread_allocate (fetchtask, 0, "fetch", TSC_THREAD_NORMAL, 0);
		thread_yield ();

		sleep(1);
	}

    while (1) 
        thread_yield ();

    thread_exit (0);
}


int fetchtask(void *v)
{
	int fd, n;
	char buf[512];
	
	fprintf(stderr, "starting...\n");
	for(;;){
		if((fd = tsc_net_dial(true, server, 80)) < 0){
			fprintf(stderr, "dial %s: %s\n", server, strerror(errno));
			continue;
		}
		snprintf(buf, sizeof buf, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", url, server);
		tsc_net_write(fd, buf, strlen(buf));
		while((n = tsc_net_read(fd, buf, sizeof buf)) > 0)
			;
		close(fd);
		write(1, ".", 1);
	}

    thread_exit (0);
}
