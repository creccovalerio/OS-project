/*
  HELPER.C
  ========
  
*/

#include "helper.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*  Read a line from a socket  */

ssize_t Readline(int sockd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ ) {
		rc = read(sockd, &c, 1);
		if ( rc == 1 ) {
	    		*buffer++ = c;
	    		if ( c == '\n' ) break;
		}
		else
			if ( rc == 0 ) {
			    if ( n == 1 ) return 0;
			    else break;
			}
			else {
			    if ( errno == EINTR ) continue;
			    return -1;
			}
    }
    *buffer = 0;
    return n;
}


/*  Write a line to a socket  */

ssize_t Writeline(int sockd, const void *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char *buffer;

    buffer = vptr;
    nleft  = n;

    while ( nleft > 0 )
	{
		if ( (nwritten = write(sockd, buffer, nleft)) <= 0 )
		{
	    	if ( errno == EINTR ) nwritten = 0;
		    else return -1;
		}
		nleft  -= nwritten;
		buffer += nwritten;
    }
    return n;
}

int ParseCmdLineSERVER(int argc, char *argv[], char **szPort){
    int n = 1;
    
    while ( n < argc ){
        if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
            *szPort = argv[++n];
        else
            if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) ) {
                printf("Sintassi:\n\n");
                printf("    server -p (porta) [-h]\n\n");
                exit(EXIT_SUCCESS);
            }
        ++n;
    }
    if (argc==1) {
        printf("Sintassi:\n\n");
        printf("    server -p (porta) [-h]\n\n");
        exit(EXIT_SUCCESS);
    }
    return 0;
}




int ParseCmdLineCLIENT(int argc, char *argv[], char **szAddress, char **szPort)
{
    int n = 1;

    while ( n < argc )
    {
        if ( !strncmp(argv[n], "-a", 2) || !strncmp(argv[n], "-A", 2) )
        {
            *szAddress = argv[++n];
        }
        else 
            if ( !strncmp(argv[n], "-p", 2) || !strncmp(argv[n], "-P", 2) )
            {
                *szPort = argv[++n];
            }
            else
                if ( !strncmp(argv[n], "-h", 2) || !strncmp(argv[n], "-H", 2) )
                {
                    printf("Sintassi:\n\n");
                    printf("    client -a (indirizzo server) -p (porta del server) [-h].\n\n");
                    exit(EXIT_SUCCESS);
                }
        ++n;
    }
    if (argc==1)
    {
        printf("Sintassi:\n\n");
        printf("    client -a (indirizzo server) -p (porta del server) [-h].\n\n");
        exit(EXIT_SUCCESS);
    }
    return 0;
}
