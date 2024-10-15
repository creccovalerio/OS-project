#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <netinet/in.h>
#include <netdb.h>

#include "helper.h"           /*  Our own helper functions  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_LINE 4096


int       conn_s;                /*  connection socket         */
char      buffer[MAX_LINE];      /*  character buffer          */
char      cho[2];





void *thread_function(){

    while(1){
        memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
        Readline(conn_s, buffer, MAX_LINE-1);
        printf("%s", buffer);
        fflush(stdout);
        if(strcmp(buffer, ":exit\n") == 0 || strcmp(buffer, ":quit\n") == 0 || strcmp(buffer, ":q\n") == 0){
            printf("exiting\n");
            fflush(stdout);
            pthread_exit(NULL);
        }
    }


    //pthread_exit(NULL);
}



int main(int argc, char *argv[]){
        
    short int port;                  //  port number
    struct    sockaddr_in servaddr;  //  socket address structure
    char     *szAddress;             //  Holds remote IP address
    char     *szPort;                //  Holds remote port
    char     *endptr;                //  for strtol()
    struct    hostent *he;
    int       choice;
    pthread_t tid;
    int       ret;
    
    he = NULL;
    
    // Get command line arguments
    ParseCmdLineCLIENT(argc, argv, &szAddress, &szPort);
    
    //  Set the remote port
    port = strtol(szPort, &endptr, 0);
    if ( *endptr ){
        printf("client: porta non riconosciuta.\n");
        exit(EXIT_FAILURE);
    }
    
    //Create the listening socket
    if ((conn_s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "client: errore durante la creazione della socket.\n");
        exit(EXIT_FAILURE);
    }
    
    //  Set all bytes in socket address structure to
    //zero, and fill in the relevant data members
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(port);
    
    /*
    //  Set the remote IP address
    szAddress = getIP(he);
    printf("IP: %s\n", szAddress);
    fflush(stdout);
    */

    if ( inet_aton(szAddress, &servaddr.sin_addr) <= 0 )
    {
        printf("client: indirizzo IP non valido.\nclient: risoluzione nome...");
        
        if ((he=gethostbyname(szAddress)) == NULL)
        {
            printf("fallita.\n");
            exit(EXIT_FAILURE);
        }
        printf("riuscita.\n\n");
        servaddr.sin_addr = *((struct in_addr *)he->h_addr);
    }
    
    //  connect() to the remote echo server    
    if ( connect(conn_s, (struct sockaddr *) &servaddr, sizeof(servaddr) ) < 0 ){
        printf("client: errore durante la connect.\n");
        exit(EXIT_FAILURE);
    }

    // insert the nickname of the current client
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    printf("client: please enter your nickname to start the app: ");
    fflush(stdout);
    fgets(buffer, MAX_LINE, stdin);
    fflush(stdin);

    
    // sent the nickname to the server
    Writeline(conn_s, buffer, strlen(buffer));
    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
    
    Readline(conn_s, buffer, MAX_LINE-1);
    printf("%s\n", buffer); //nickname ok

rechoice:

    while(1){


        printf("\t\t\t\n***|| MENU ||***:\n");
        fflush(stdout);
        printf("\t(1) Create a new chat group\n\t(2) Join an existing group\n\t(3) Exit\n");
        fflush(stdout);

        fgets(cho, 3, stdin);
        fflush(stdin);
        choice = atoi(cho);
        printf("%d\n", choice);
        fflush(stdout);

        // signal SIGINT to close the socket and the thread to send the string
        //signal(SIGINT, sigint_handler);

        switch(choice){
                
            case 1:
    
                // send the choice to the sever
                Writeline(conn_s, cho, strlen(cho));
                printf("Insert the name of the group to create: ");
                fflush(stdout);
                memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
                fgets(buffer, MAX_LINE, stdin);
                fflush(stdin);
                Writeline(conn_s, buffer, strlen(buffer));
                printf("sent the name of the group: %s\n", buffer);
                break;
                
            case 2:
    
                // send the choice to the server
                Writeline(conn_s, cho, strlen(cho));

                
                printf("The groups on-line are:\n");
                while(1){

                    memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
                    Readline(conn_s, buffer, MAX_LINE-1);

                    if(strcmp(buffer, "option_not_available\n") == 0){
                        goto rechoice;                    }

                    if(strcmp(buffer, ":-finito\n") == 0){
                        break;
                    }

                    printf("\t - %s\n", buffer);
                    fflush(stdout);

                }
                printf("Insert the id of the group you want to join: ");
                fgets(cho, 6, stdin);
                fflush(stdin);
                Writeline(conn_s, cho, strlen(cho));
                break;
                
            case 3:
            
                // send the choice to the server
                Writeline(conn_s, cho, strlen(cho));
                exit(1);
                
                break;

            default:

                goto rechoice;

                break;

        }


        // creation of the reading from socket thread
        ret = pthread_create(&tid, NULL, thread_function, (void*)(long)conn_s);
        if(ret != 0){
            printf("pthread_create error\n");
            exit(-1);
        }

        printf("If you want to sent a message write it and press 'ENTER'\n");
        fflush(stdout);


        while(1){
            
            //  Get string to echo from user
            memset(buffer, 0, sizeof(char)*(strlen(buffer)+1));
            fgets(buffer, MAX_LINE, stdin);
            fflush(stdin);
            
            //  Send string to echo server
            
            Writeline(conn_s, buffer, strlen(buffer));
            if(strcmp(buffer, ":exit\n") == 0 || strcmp(buffer, ":quit\n") == 0 || strcmp(buffer, ":q\n") == 0){
                printf("waiting for termination of the son thread\n");
                pthread_join(tid, NULL);
                break;
            }

        }

    
    }
}