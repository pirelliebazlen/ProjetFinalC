#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd;
char *pShm;
void HandlerSIGINT(int sig);
int main()
{
  
  MESSAGE m;
  // Armement des signaux
  struct sigaction A;
  A.sa_handler = HandlerSIGINT;
  A.sa_flags = 0;
  sigemptyset(&A.sa_mask);
  if(sigaction(SIGINT, &A, NULL)==-1)
  {
    perror("erreur sigaction");
    exit(1);
  }
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  
  if((idQ= msgget(CLE, 0))==-1)
  {
    perror("Erreur shmget Publicite");
    exit(1);
  }
 
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if((idShm= shmget(CLE, 0, 0))==-1)
  {
      perror("erreur shmget Publicite ");
      exit(1);
  }
  // Attachement à la mémoire partagée
  
  if((pShm = (char*)shmat(idShm, NULL,0))==(char*)-1)
  {
    perror("Erreur de shmat");
    exit(1);
  }

  // Ouverture du fichier de publicité
  if((fd = open("publicites.dat", O_RDONLY))==-1)
  {
    perror("Erreur ouverture fichier");
    exit(1);
  }


  while(1)
  {
  	PUBLICITE pub;
    // Lecture d'une publicité dans le fichier
    if(read(fd, &pub, sizeof(PUBLICITE))<0)
    {
      perror("Erreur read Publicite");
      exit(0);
    }
    if(read(fd, &pub, sizeof(PUBLICITE))==0)
    {
      lseek(fd, 0, SEEK_SET);
    }
    if(read(fd, &pub, sizeof(PUBLICITE))>0)
    {
      // Ecriture en mémoire partagée
      strcpy(pShm, pub.texte);
      printf("mem partagée => %s\n", pShm);
      // Envoi d'une requete UPDATE_PUB au serveur
      m.type=1;
      m.expediteur= getpid();
      m.requete = 10;
      if(msgsnd(idQ,  &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur msgsnd Publicite");
        exit(1);
      }
    }
    
    //Attente de ...
    sleep(pub.nbSecondes);


  }
  close(fd);
}
void HandlerSIGINT(int sig)
{

}
