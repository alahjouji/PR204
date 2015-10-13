#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <poll.h>
/* autres includes (eventuellement) */

#define BACKLOG 15
#define MAXBUF 1024

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;//rang du processus
   char nom_machine[MAXBUF];//nom de la machine ou tourne le processus
   int port;// numero de port de la socket d'ecoute
   int dsm_pid;//pid du processus dsm
   int sock_cli;//numero de la socket de connexion avec le lanceur

   /* a completer */
};


typedef struct dsm_proc_conn dsm_proc_conn_t; 

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {   
  pid_t pid;
  dsm_proc_conn_t connect_info;
  int tube_out[2];//tube de redirection de stdout
  int tube_err[2];//tube de redirection de stderr
};
typedef struct dsm_proc dsm_proc_t;


int creer_socket();
int write_message(int socket, char * message);
void read_pipe(int tube, char * buffer) ;
