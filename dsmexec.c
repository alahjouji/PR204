#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL; 

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
   /* on traite les fils qui se terminent */
   /* pour eviter les zombies */
}



int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {       
     pid_t pid; 
     int num_procs = 0;
     int i;
     FILE* file_proc;
     char ligne[128];
     int j;
     int sockfd; 
     int sock_cli;
     char buffer[MAXBUF];
     struct sockaddr_in servaddr;
     struct sockaddr_in sockaddr_cli;
     socklen_t length=sizeof(struct sockaddr_in);  
     char buffer1[MAXBUF];
     int a;
     int timeouts=0;
     
     /* Mise en place d'un traitant pour recuperer les fils zombies*/      
     /* XXX.sa_handler = sigchld_handler; */

     /* lecture du fichier de machines */
     /* 1- on recupere le nombre de processus a lancer */
     /* 2- on recupere les noms des machines : le nom de */
     /* la machine est un des elements d'identification */

     // Ouverture du fichier machine_file 
     printf("\nrecuperation des noms des machines distantes\n\n");
     file_proc=fopen(argv[1],"r");

    // recuperation du nombre de lignes que contient le fichier (nombre de processus)
    while( fgets ( ligne, sizeof ligne, file_proc) != NULL ) {
      num_procs++;
    }
      printf("nombre de processus est %d\n\n",num_procs);

    //allocation dynamique du tableau gerant les infos d'identification des processus dsm
    proc_array = malloc(sizeof(dsm_proc_t) * num_procs);
    
    //ramener l'indicateur de position d fichier au debut
    rewind(file_proc);

    //recuperer les noms des machines qui correspondent aux differents processus
    for (j=0;j<=num_procs; j++){
      fgets ( ligne, MAXBUF, file_proc);
      memset(buffer, 0, MAXBUF);
      strcpy(buffer,ligne );
      strncpy(proc_array[j].connect_info.nom_machine, buffer, strlen(buffer)-1);
      proc_array[j].connect_info.rank=j+1;
    }

    //affichage des noms des machines
    for (i =0 ; i<num_procs; i++)
      printf("nom de la machine %i : %s\n",i, proc_array[i].connect_info.nom_machine);



     /* creation de la socket d'ecoute */
    sockfd=creer_socket();

    //recuperation des infos de la socket dans une structure sockaddr
    if( getsockname(sockfd, (struct sockaddr*) &servaddr, &length) == -1){
      perror("getsockname");
      exit(EXIT_FAILURE);
    }

    //recuperation du numero de port a la socket locale et du nom de la machine ou s'execute dsmexec
    char char_port[6];
    int port_exec=ntohs(servaddr.sin_port);
    sprintf(char_port,"%d",port_exec);
    printf("port d'attachement: %d\n\n", port_exec);
    char nom[MAXBUF];
    gethostname(nom, MAXBUF);
    printf("nom de la machine sur laquel s'execute dsmexec : %s\n\n",nom);


     /* + ecoute effective */ 
     if(listen(sockfd,BACKLOG) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }

      //l'ensemble de descripteurs de fichiers a surveiller avec poll
      struct pollfd * fds=malloc(num_procs*sizeof(struct pollfd));

      printf("\ncreation des processus locaux et redirections des tubes\n\n");

     /* creation des fils */
     for(i = 0; i < num_procs ; i++) {
        //recuperation du rang du ocessus sous forme de chaine de caracteres
	char char_rank[MAXBUF];
	sprintf(char_rank, "%d", proc_array[i].connect_info.rank);

	/* creation du tube pour rediriger stdout */
        if (pipe(proc_array[i].tube_out) != 0) {
	   perror("pipe_out");
	   exit( EXIT_FAILURE);
        } 



	/* creation du tube pour rediriger stderr */

        if (pipe(proc_array[i].tube_err) != 0) {
	   perror("pipe_err");
	   exit(EXIT_FAILURE);
        } 

	
	pid = fork();

	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
        } 
	
	if (pid == 0) { /* fils */	
	   
	   /* redirection stdout */	      
	   close(proc_array[i].tube_out[0]);   
	   dup2(proc_array[i].tube_out[1],STDOUT_FILENO);
	   close(proc_array[i].tube_out[1]);

	   /* redirection stderr */	      	      
	   close(proc_array[i].tube_err[0]);
	   dup2(proc_array[i].tube_err[1],STDERR_FILENO);
	   close(proc_array[i].tube_err[1]);

	   /* Creation du tableau d'arguments pour le ssh */ 
	   char* newargv[6] ; 

	
	newargv[0] = "ssh";
        //nom de la machine sur laquelle se fera le ssh
	newargv[1] = proc_array[i].connect_info.nom_machine;
        //chemin vers l'executable de dsmwrap
	newargv[2] = "~/prog_syst/bin/dsmwrap";
        //nom de la machine sur laquelle s'execute le dsmexec
	newargv[3] = nom;
        //numero de port de la socket locale
	newargv[4] = char_port;
        //rang du processus dsm qui execute dsmwrap dans la machine distante
	newargv[5] = char_rank;

       //nom du programme a executer
	newargv[6] = argv[2];

	//arguments du programme a executer
	for (i=0; i<argc-2; i++) 
	  newargv[7+i] = argv[i+3];

	newargv[6 + argc -1] = NULL;



	   /* jump to new prog : */
	    execvp("ssh",newargv); 

	} else  if(pid > 0) { /* pere */		      
	   /* fermeture des extremites des tubes non utiles */
	  close(proc_array[i].tube_out[1]);
	  close(proc_array[i].tube_err[1]);
          //ajout de l'extremite en lecture du tube de reditection de stdout a l'ensemble des descripteurs de fichiers a surveiller
          fds[i].fd = proc_array[i].tube_out[0];
	  fds[i].events=POLLIN;
	   num_procs_creat++;	
                 
	}
     }

     printf("nombre de processus effectivement cree: %d\n\n",num_procs_creat);
     

     printf("recuperation des donnees des processus distants\n\n");
   
    for(i = 0; i < num_procs ; i++){
	/* on accepte les connexions des processus dsm */
      sock_cli = accept(sockfd,(struct sockaddr *)&sockaddr_cli, &length);
	
	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */

      memset(buffer, 0, MAXBUF); 
      read_pipe(proc_array[i].tube_out[0], buffer);
      printf("Nom de la machine distante : %s\n",buffer);

	
	/* On recupere le pid du processus distant  */
      memset(buffer, 0, MAXBUF);
      read_pipe(proc_array[i].tube_out[0], buffer);
	
      printf("pid du processus distant : %s\n", buffer);
	
	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */
      memset(buffer, 0, MAXBUF);
      read_pipe(proc_array[i].tube_out[0], buffer);
	
      printf("port de la socket d'ecoute du processus distant : %s\n",buffer);
      //stockage du numero du port dans la table des infos d'identification	
      proc_array[i].connect_info.port = atoi(buffer);

      // On récupère le rang du processus distant et le numero de la socket d'ecoute
      memset(buffer, 0, MAXBUF);
      read_pipe(proc_array[i].tube_out[0], buffer);
      printf("rang du processus distant : %s\n", buffer);
      printf("numero de le socket de communication avec le lanceur: %d\n", sock_cli);
      //stockage du numero de la socket dans la table des infos d'identification
      proc_array[i].connect_info.sock_cli = sock_cli;

      printf("\n");
	
     }

     //recuperer le nombre de processus sous forme d'une chaine de caracteres 
     char char_num_procs[MAXBUF];
     sprintf(char_num_procs, "%d\n", num_procs);

     /* envoi du nombre de processus aux processus dsm*/
     
     /* envoi des rangs aux processus dsm */
     
     /* envoi des infos de connexion aux processus */

    //printf("envoi des donnees aux processus distants\n");
    for (i=0; i< num_procs; i++) {


      //la socket d'ecoute distante a laquelle les infos seront envoyees
      sock_cli = proc_array[i].connect_info.sock_cli;

      //envoi du nobre de processus distants
      write_message(sock_cli, char_num_procs);
 
      //envoi du rang du processus
      memset(buffer, 0, MAXBUF);
      sprintf(buffer, "%d", proc_array[i].connect_info.rank);
      write_message(proc_array[i].connect_info.sock_cli, buffer);
	
	  
      for( j=0; j< num_procs; j++) {

	memset(buffer, 0, MAXBUF);

	//envoi du rang de chacun des processus distants
	sprintf(buffer, "%d", proc_array[j].connect_info.rank);
	write_message(proc_array[i].connect_info.sock_cli, buffer);

	memset(buffer, 0, MAXBUF);
	//Envoi du nom de chacune des machines distantes
	write_message(proc_array[i].connect_info.sock_cli, proc_array[j].connect_info.nom_machine);

	//Envoi du numéro de port a laquel chaque socket d'ecoute distante est ratachee
	sprintf(buffer, "%d", proc_array[j].connect_info.port);
	write_message(proc_array[i].connect_info.sock_cli, buffer);

      }
    }

    printf("\nlecture dans les tubes:\n");
   

     /* gestion des E/S : on recupere les caracteres */
     /* sur les tubes de redirection de stdout */  

     // tant qu'on n'a pas atteint le temps limite on surveille l'ensemble des extremites en lecture des tubes ou le stdout est redirige
     //si i y a des donnees dedans on les affiche
     while(timeouts<1000000){
		a=poll(fds,num_procs,-1);
		for(i=0;i<num_procs_creat;i++){
			if(fds[i].revents==POLLIN){
				do{
				a=read(proc_array[i].tube_out[0],buffer1,MAXBUF*sizeof(char));
				}while((a==-1)&&(errno==EINTR));
				fprintf(stdout,"[%s] : ",proc_array[i].connect_info.nom_machine);
				fprintf(stdout, "%s\n", buffer1);
			}
		}
		timeouts++;
	}

        

     
     /* on attend les processus fils */
     wait(NULL);

      printf("\nfermeture des descripteurs et de la socket d'ecoute\n");

     /* on ferme les descripteurs proprement */
         for (i=0; i< num_procs; i++) {
      close(proc_array[i].tube_out[0]);
      close(proc_array[i].tube_out[1]);
      close(proc_array[i].tube_err[0]);
      close(proc_array[i].tube_err[1]);
    }
     /* on ferme la socket d'ecoute */
     close (sockfd);

     fprintf(stdout, "\nfin du programme\n");
  }   
   exit(EXIT_SUCCESS);  
}

