#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include "helper.h"

#define MESSAGE_SIZE       (4030) //4096 - 66 caratteri per "nome: "
#define S_MESSAGE_SIZE     (4096)
#define MAX_LINE           (1000)



typedef struct _clients{
    char ip[16];
    char name[64];
    int sockID;
    int groupId;
    struct _clients *next;
}clients;



typedef struct _groups{ 
    int id_group;
    char name[64];
    int num_users;
    struct _groups *next;
    struct _groups *prev;
    struct _clients *parent;  
}groups;



int reinsert_user(clients *cl);
pthread_t tid;
groups *group_head = NULL;



int send_all(int id, char *buffer, int me){
    
    int ret;
    groups *tmp;
    tmp = group_head;

    while(tmp != NULL && tmp->id_group != id){
        tmp = tmp->next;
    }
    
    printf("\nsend_all function activated\nsending message %s to all the users of group %d\n", buffer, tmp->id_group);
    //invia il messaggio a tutti quelli del gruppo eccetto me
    clients *cl;
    cl = tmp->parent;
    while(cl != NULL){
        if(cl->sockID == me){
            cl = cl->next;
            continue;
        }
        ret = Writeline(cl->sockID, buffer, strlen(buffer));
        printf("INVIATO\n");
        cl = cl->next;
    }
    return ret;
}



int del_gr(groups *g){
    
    clients *tmp, *tmp1;
    tmp = g->parent;
    
    if(g == group_head){
        if(g->next != NULL){
            group_head = g->next;
            g->next->prev = NULL;
            free(g);
            return 1;
        }
        free(g);
        group_head = NULL;
        return 1;
    }

    g->prev->next = g->next;
    g->next->prev = g->prev;
    free(g);
    return 1;
}



void* client_management(void* client){
    
    clients *c = (clients *)client;
    printf("\n**** %s manager activated ****\n", c->name);
    groups *tm;
    clients *tc;
    char msg[MESSAGE_SIZE];
    char tmp_msg[MESSAGE_SIZE];
    char error_buff[1024];
    char send_msg[4096];
    int flg = 0, ret;
 
    //cerco subito il gruppo di appartenenza del client in modo che se in futuro
    //dovrà abbandonare la chat so già dove andarlo ad eliminare
    tm = group_head;
    while(tm->id_group != c->groupId && tm != NULL){
        tm = tm->next;
    }
    if(tm == NULL){
        sprintf(error_buff, "WARNING: the group has been closed.\n");
        printf("tm error\n");
        Writeline(c->sockID, error_buff, strlen(error_buff));
        close(c->sockID); //chiudi la connessione
        free(c);
        exit(1);
    }
    
    printf("\t%s in group %d\n", c->name, tm->id_group);
    
    
    while(1){
        
        if(flg == 1) break;
        
        memset(msg, 0, sizeof(char)*(strlen(msg)+1));
        memset(tmp_msg, 0, sizeof(char)*(strlen(tmp_msg)+1));

        int receive = Readline(c->sockID, msg, MESSAGE_SIZE-1);
        printf("message: %s\n", msg);
        
        if(receive > 0){

            if(strlen(msg) == 0){
                continue;
            }
            
            if(strcmp(msg, ":exit\n") == 0 || strcmp(msg, ":quit\n") == 0 || strcmp(msg, ":q\n") == 0){
                sprintf(send_msg, "%s leave the group\n", c->name);
                Writeline(c->sockID, msg, strlen(msg));
                flg = 1;
            }

            if(strcmp(msg, ":delete\n") == 0){
                del_gr(tm);
                reinsert_user(c);
                flg = 1;
                pthread_exit(NULL);
            }
            
            strncpy(tmp_msg, msg, strlen(msg)-1);
            sprintf(send_msg, "%s: %s\n", c->name, tmp_msg);
            
            
        }else if(receive == 0){

            flg = 1;
            continue;
            
        }else{
            //errore
            printf("ERROR\n");
            break;
        }
        
        printf("send_msg: %s\n", send_msg);
        ret = send_all(c->groupId, send_msg, c->sockID);
        if(ret == -1){
            sprintf(error_buff, "ERROR: sending message error.\n");
            Writeline(c->sockID, error_buff, strlen(error_buff));
        }
    }
   
    if(tm->parent == c && tm->num_users > 1){
        tm->parent = c->next;
        c->next = NULL;
        tm->num_users--;
        printf("cambiato padrone con %s\n", tm->parent->name);
    }else if(tm->parent == c && tm->num_users == 1){
        
        del_gr(tm);
        printf("eliminato gruppo\n");
        
    }else if(c != tm->parent){
        printf("eliminato user\n");
        tc = tm->parent;
        while(tc->next->sockID != c->sockID ){
            tc = tc->next;
        }
    
    
        tc->next = c->next;
        c->next = NULL;
        tm->num_users--;
        //Writeline(c->sockID, msg, strlen(msg));

    }

    reinsert_user(c);
    pthread_exit(NULL);

}

int reinsert_user(clients *cl){
    
    int choose, ch, id, ret;
    char nickname[64];
    char groupname[64];
    char int_buf[6];
    char answer[128];
    char send_msg[MESSAGE_SIZE];
    
    groups *tmp;
    groups *newg;
    groups *gr;
    clients *nextc;
    
    
    nextc = NULL;
    tmp = NULL;
    
    memset(int_buf, 0, 3);
    Readline(cl->sockID, int_buf, 3);
    choose = atoi(int_buf);
    
    clients *tempo;
    
    switch(choose){
            
        case 1:
            //crea nuovo gruppo
            
            newg = (groups*)malloc(sizeof(groups));
            if(newg == NULL){
                printf("malloc error\n");
                exit(-1);
            }
            
            
            memset(groupname, 0, 64);
            Readline(cl->sockID, groupname, 64);
            
            strncpy(newg->name, groupname, strlen(groupname)-1);
            printf("%s\n", newg->name);
            
            
            newg->next = NULL;
            newg->prev = NULL;
            newg->num_users = 1;
            
            
            if(group_head == NULL){
                
                group_head = newg;
                newg->id_group = 0;
                
                
            }else{
                
                tmp = group_head;
                id = 0;
                while(1){
                    
                    id++;
                    if(tmp->next == NULL) break;
                    
                    /*Controllo se non ci siano buchi, ovvero se fra un nodo ed un altro si sia stato
                     cancellato un nodo in precedenza. In questo modo il nuovo gruppo prenderà il posto
                     di quello vecchio riutilizando l'id*/
                    if((tmp->next->id_group - tmp->id_group) > 1){
                        tmp->next->prev = newg;
                        newg->next = tmp->next;
                        newg->prev = tmp;
                        tmp->next = newg;
                        id++;
                        newg->id_group = id;
                        goto exit_loop;
                    }
                    
                    
                    tmp = tmp->next;
                    
                }
                
                tmp->next = newg;
                newg->prev = tmp;
                newg->id_group = id;
                
                
            }
            
        exit_loop:
            
            cl->groupId = newg->id_group;
            newg->parent = cl;
            
            printf("group: %d user: %s\n", cl->groupId, cl->name);
            //printf("%s, %d\n", (newg->parent)->name, newg->num_users);
            
            
            
            ret = pthread_create(&tid, NULL, client_management, (void *)cl);
            if(ret != 0){
                printf("thread create error\n");
                exit(-1);
            }
            
            
            break;
            
        case 2:
            
            //join a group



            gr = group_head;

            if(gr == NULL){
                printf("sono qui\n");
                memset(answer, 0, 128);
                sprintf(answer, "option_not_available\n");
                Writeline(cl->sockID, answer, strlen(answer));
                break;
            }

            while(gr != NULL){
                
                memset(answer, 0, 128);
                sprintf(answer, "(%d) %s\n", gr->id_group, gr->name);
                Writeline(cl->sockID, answer, strlen(answer));
                gr = gr->next;
                
            }
            memset(answer, 0, 128);
            sprintf(answer, ":-finito\n");
            Writeline(cl->sockID, answer, strlen(answer));
            
            
            Readline(cl->sockID, int_buf, 6);
            ch = atoi(int_buf);
            printf("received command %d\n", ch);
            
            
            cl->groupId = ch;
            
            gr = group_head;
            
            while(gr->id_group != ch){
                gr = gr->next;
            }
            
            (gr->num_users)++;
            
            nextc = gr->parent;
            
            while(nextc->next != NULL){
                nextc = nextc->next;
            }
            nextc->next = cl;
            
            tempo = gr->parent;
            while(tempo != NULL){
                printf("--->%s ", tempo->name);
                tempo = tempo->next;
            }
            printf("\n");
            if(gr->num_users > 1){
                sprintf(send_msg, "%s join the group!\n", cl->name);
                ret = send_all(cl->groupId, send_msg, cl->sockID);
            }
            ret = pthread_create(&tid, NULL, client_management, (void*)cl);
            if(ret != 0){
                printf("thread create error\n");
                exit(-1);
                
            }   
            
            break;
        
    }
    
  
}


void* addClient(void* sockfd){
    
    int choose, ch, id, ret;
    char nickname[64];
    char groupname[64];
    char int_buf[6];
    char answer[128];
    char send_msg[MESSAGE_SIZE];

    groups *tmp;
    groups *newg;
    groups *gr;
    clients *nextc;
    
   
    nextc = NULL;
    tmp = NULL;
    
    
    clients *cl = (clients*)malloc(sizeof(clients));
    if(cl == NULL){
        
        printf("malloc error\n");
        exit(-1);
        
    }
    
    printf("%ld connected\n", (long)sockfd);

    
    cl->sockID = (long)sockfd;
    cl->next = NULL;
    
    
    memset(nickname, 0, sizeof(char)*(strlen(nickname)+1));
    
    ret = Readline(cl->sockID, nickname, 63);
    printf("ret = %d\nstrlen(nickname) = %ld\n", ret, strlen(nickname));
    memset(cl->name, 0, sizeof(char)*(strlen(cl->name)+1));

    strncpy(cl->name, nickname, strlen(nickname)-1);
    printf("nickname: %s\n", cl->name);
    fflush(stdout);
    
    //risposta al client di successo inserimento nickname
    memset(answer, 0, 128);
    sprintf(answer, "nickname %s is ok!\n", cl->name);
    Writeline(cl->sockID, answer, strlen(answer));
    
    
    
    //aspetta di ricever un comando
    memset(int_buf, 0, 3);
    Readline(cl->sockID, int_buf, 3);
    choose = atoi(int_buf);
    
    clients *tempo;
    
    switch(choose){
            
        case 1:
            //crea nuovo gruppo
            
            newg = (groups*)malloc(sizeof(groups));
            if(newg == NULL){
                printf("malloc error\n");
                exit(-1);
            }
            
            
            memset(groupname, 0, 64);
            Readline(cl->sockID, groupname, 64);

            strncpy(newg->name, groupname, strlen(groupname)-1);
            printf("%s\n", newg->name);
            
            
            newg->next = NULL;
            newg->prev = NULL;
            newg->num_users = 1;
            
            
            if(group_head == NULL){
                
                group_head = newg;
                newg->id_group = 0;
                
                
            }else{
                
                tmp = group_head;
                id = 0;
                while(1){
                    
                    id++;
                    if(tmp->next == NULL) break;
                    
                    /*Controllo se non ci siano buchi, ovvero se fra un nodo ed un altro si sia stato
                     cancellato un nodo in precedenza. In questo modo il nuovo gruppo prenderà il posto
                     di quello vecchio riutilizando l'id*/
                    if((tmp->next->id_group - tmp->id_group) > 1){
                        tmp->next->prev = newg;
                        newg->next = tmp->next;
                        newg->prev = tmp;
                        tmp->next = newg;
                        id++;
                        newg->id_group = id;
                        goto exit_loop;
                    }
                    
                    
                    tmp = tmp->next;
                    
                }
                
                tmp->next = newg;
                newg->prev = tmp;
                newg->id_group = id;
                
                
            }

exit_loop:
            
            cl->groupId = newg->id_group;
            newg->parent = cl;

            printf("group: %d user: %s\n", cl->groupId, cl->name);
            //printf("%s, %d\n", (newg->parent)->name, newg->num_users);
            
            
            
            ret = pthread_create(&tid, NULL, client_management, (void *)cl);
            if(ret != 0){
                printf("thread create error\n");
                exit(-1);
            }
            
            
            break;
            
        case 2:
            
            //join a group
            
            gr = group_head;
            if(gr == NULL){
                printf("sono qui\n");
                memset(answer, 0, 128);
                sprintf(answer, "option_not_available\n");
                Writeline(cl->sockID, answer, strlen(answer));
                break;
            }


            while(gr != NULL){
                
                memset(answer, 0, 128);
                sprintf(answer, "(%d) %s\n", gr->id_group, gr->name);
                Writeline(cl->sockID, answer, strlen(answer));
                gr = gr->next;
                
            }
            memset(answer, 0, 128);
            sprintf(answer, ":-finito\n");
            Writeline(cl->sockID, answer, strlen(answer));
            
            
            Readline(cl->sockID, int_buf, 6);
            ch = atoi(int_buf);
            printf("received command %d\n", ch);
            
            
            cl->groupId = ch;
            
            gr = group_head;
            
            while(gr->id_group != ch){
                gr = gr->next;
            }
            
            (gr->num_users)++;
            
            nextc = gr->parent;
            
            while(nextc->next != NULL){
                nextc = nextc->next;
            }
            nextc->next = cl;
            
            tempo = gr->parent;
            while(tempo != NULL){
                printf("--->%s ", tempo->name);
                tempo = tempo->next;
            }
            
            if(gr->num_users > 1){
                sprintf(send_msg, "%s join the group!\n", cl->name);
                ret = send_all(cl->groupId, send_msg, cl->sockID);
            }
            ret = pthread_create(&tid, NULL, client_management, (void*)cl);
            if(ret != 0){
            printf("thread create error\n");
            exit(-1);
             
            }
            
            break;

    }


}


// Returns hostname for the local computer
void checkHostName(int hostname)
{
    if (hostname == -1)
    {
        perror("gethostname");
        exit(1);
    }
}


void checkHostEntry(struct hostent *hostentry)
{
    if (hostentry == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
}

// Converts space-delimited IPv4 addresses
// to dotted-decimal format
void checkIPbuffer(char *IPbuffer)
{
    if (NULL == IPbuffer)
    {
        perror("inet_ntoa");
        exit(1);
    }
}

// Driver code
char *getIP(struct hostent *he){

    char hostbuffer[256];
    char *IPbuffer;
    int hostname;
    
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);
    
    // To retrieve host information
    he = gethostbyname(hostbuffer);
    checkHostEntry(he);
    
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)he->h_addr));
    
    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);
    
    return IPbuffer;
}






int main(int argc, char *argv[]){
    
    int       list_s;                /*  listening socket          */
    int       conn_s;                /*  connection socket         */
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    struct    sockaddr_in their_addr;
    char      buffer[MAX_LINE];      /*  character buffer          */
    char     *endptr;                /*  for strtol()              */
    int       sin_size;
    int       ret;
    char     *szAddress;
    struct    hostent *he;

    //he = NULL;
    
    ParseCmdLineSERVER(argc, argv, &endptr);

    szAddress = getIP(he);
    printf("IP server: %s\n", szAddress);
    fflush(stdout);
    
    port = strtol(endptr, &endptr, 0);
    if ( *endptr ){
        fprintf(stderr, "server: porta non riconosciuta.\n");
        exit(EXIT_FAILURE);

    }
    
    printf("Server in ascolto sulla porta: %d.\n", port);
    
    /*  Create the listening socket  */
    
    if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        fprintf(stderr, "server: errore nella creazione della socket.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Set all bytes in socket address structure to zero, and
     fill in the relevant data menbers   */
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);

    
    
    
    /*  Bind our socket addresss to the
     listening socket, and call listen()  */
    
    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        fprintf(stderr, "server: errore durante la bind.\n");
        exit(EXIT_FAILURE);
    }
    
    
    
    if ( listen(list_s, LISTENQ) < 0 ){
        fprintf(stderr, "server: errore durante la listen.\n");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        
        /*  Wait for a connection, then accept() it  */
        
        sin_size = sizeof(struct sockaddr_in);
        if ( (conn_s = accept(list_s, (struct sockaddr *)&their_addr, &sin_size) ) < 0 ){
            fprintf(stderr, "server: \n");
            exit(EXIT_FAILURE);
        }
        
        printf("server: connessione da %s, sock %d\n", inet_ntoa(their_addr.sin_addr), conn_s);
        /*  Retrieve an input line from the connected socket
         then simply write it back to the same socket.     */
        
        ret = pthread_create(&tid, NULL, addClient, (void*)(long)conn_s);
        if (ret != 0){
            printf("server: impossibile creare il thread. Errore: %d\n", ret);
            exit(EXIT_FAILURE);
        }
        
        
    }
    //se esce da questo while vuol dire che il main viene chiuso: non deve attendere nessuno
    //dei thread
    //pthread_join(tid, &status);
    
}