#include "common_impl.h"

int creer_socket() 
{
   int fd = 0;
   struct sockaddr_in servaddr;
   /* fonction de creation et d'attachement */
   /* d'une nouvelle socket */

    // Création de la socket
    if((fd=socket(AF_INET,SOCK_STREAM,0)) < 0) {
      perror("socket");
      exit(EXIT_FAILURE);
    }
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(0);
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);


    //rattachement de la socket
    if(bind(fd,(struct sockaddr*) &servaddr, sizeof(servaddr)) <0) {
      perror("bind");
      exit(EXIT_FAILURE);
    }


   
   return fd;
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */



int write_message(int socket, char * message) {

    int size=strlen(message);
 
    //envoi de la taille du message envoye
    write(socket, &size, sizeof(int));

    //envoi du message
    write(socket,message, size);	
	
return 0;
}

void read_pipe(int pipe, char * buffer) {

  int i;
  char c[1];

  for (i=0; i<MAXBUF; i++) {
    //lecture a partir du tube caractere par caractere
    read(pipe, c, 1);
    if ((c[0] == '\n')) {
      buffer[i] = '\0';       
      break;
    }
	   
    else
      buffer[i] = c[0];
  }

}

