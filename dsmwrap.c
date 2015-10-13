#include "common_impl.h"

int main(int argc, char **argv)
{ 
   /* processus intermediaire pour "nettoyer" */
   /* la liste des arguments qu'on va passer */
   /* a la commande a executer vraiment */
  char nom_dsm[MAXBUF];
  int cli_sock;
  int sock_dsm;
  int port_dsm ;
  struct sockaddr_in servaddr;
  socklen_t length;
  int i=0,j;
  int rc;

   /* creation d'une socket pour se connecter au */
   /* au lanceur et envoyer/recevoir les infos */
   /* necessaires pour la phase dsm_init */   


  struct hostent *addr_srv;
  struct in_addr addr;
  addr_srv = gethostbyname(argv[1]);// recuperer dans une structure hostent l'addresse ip du lanceur a partir de son nom
  addr.s_addr = *(u_long *) addr_srv->h_addr_list[0]; //recuperer dans une structure in_addr qui correspond au nom de la machine distante
  
  struct addrinfo hints;

  memset(&hints, 0, sizeof(struct addrinfo)); // initialisation structure addrinfo
  hints.ai_socktype = SOCK_STREAM; // type : socket TCP

 
  struct addrinfo* returned_infos; 

// renvoie une ou plusieurs structures addrinfo, chacune d'entre elles contenant une adresse Internet dans returned_infos
  rc = getaddrinfo(inet_ntoa(addr), argv[2], &hints, &returned_infos);
  if (rc != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return -1;
  }


  struct addrinfo* rp;
  //on parcourt toutes les addrinfo jusqu'a trouver l'addresse a laquelle on peut se connecter (celle actuellement en ecoute)
  for (rp = returned_infos; rp != NULL; rp = rp->ai_next) {
    //creation de la socket avec laquelle on a recevoir des donnees au lanceur
    cli_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (cli_sock == -1) {
      continue;
    }
		
    if (connect(cli_sock, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; 
    }
		
    close(cli_sock); 
    cli_sock = -1;

 }

  freeaddrinfo(returned_infos);

  if(cli_sock < 0) {
    perror("Connexion echouee");
    exit(EXIT_FAILURE);
  }
   
   /* Envoi du nom de machine au lanceur */
   gethostname(nom_dsm, MAXBUF);
   printf("%s\n", nom_dsm);

   /* Envoi du pid au lanceur */
   pid_t pid = getpid();
   printf("%d\n", pid);

   /* Creation de la socket d'ecoute pour les */
   /* connexions avec les autres processus dsm */
   sock_dsm=creer_socket();



  if(listen(sock_dsm, BACKLOG) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

   length=sizeof(struct sockaddr_in);
   if( getsockname(sock_dsm, (struct sockaddr*) &servaddr, &length) == -1) {
    perror("getsockname");
    exit(EXIT_FAILURE);
   }

   /* Envoi du numero de port au lanceur */
   /* pour qu'il le propage a tous les autres */
   /* processus dsm */
   port_dsm = ntohs(servaddr.sin_port);
   printf("%d\n", port_dsm);
	

  //envoi du rang au lanceur
  printf("%s\n", argv[3]);



  /// Flush ///
  fflush(stdout);
  fflush(stderr);

    //recuperer le nombre d'arguments du programme a executer
    while(argv[i+5]!=NULL){
	  i=i+1;
    }

  char* newargv[i+1] ;

  //tableau d'arguments du programme a executer
  for (j=0; j<i; j++) 
	  newargv[j] = argv[5+j];

   newargv[i]=NULL;




  //recuperation du chemin de l'executable du programme a executer
  char *chemin = "~/prog_syst/bin/"; 
  strcat(chemin, argv[4]);


   /* On execute la commande demandee */
   execvp(chemin,newargv);
   return 0;
}
