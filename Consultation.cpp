#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <mysql.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "protocole.h"

int idQ,idSem;
union semun
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
} arg;

int main()
{
  // Recuperation de l'identifiant de la file de messages
  struct sembuf operations[2];
  MESSAGE m;
  MESSAGE reponse;
  unsigned short valSem;
  long type;
  if((idQ=msgget(CLE, 0))==-1)
  {
    perror("Erreur msgget");
    exit(1);
  }

  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());


  type=getpid();
  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), type, 0)==-1)
  {
    perror("Erreur msgrcv CONSULTATION");
    exit(1);
  }
  // Recuperation de l'identifiant du sémaphore
  printf("je suis la\n");
  if((idSem=semget(CLE,0,0))==-1)
  {
    perror("Erreur semget");
    exit(0);
  }
  

  valSem=semctl(idSem, 0, GETVAL);
  if(valSem==-1)
  {
    perror("Erreur de semctl Consultation");
  }
 
  if(valSem==0)
  {
    reponse.expediteur= getpid();
    reponse.type= m.expediteur;
    reponse.requete=CONSULT;
    strcpy(reponse.data1, "---attente---");
    if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long),0))
    {
      perror("Erreur msgsnd CONSULTATION");
    }
    
    kill(m.expediteur,SIGUSR1);
   
  }

  operations[0].sem_num=0;
  operations[0].sem_op=-1;
  operations[0].sem_flg=0;

  if(semop(idSem, operations,1)==-1)
  {
    perror("Erreur semop");
  }


  // Lecture de la requête CONSULT
  
  
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  
  // Tentative de prise bloquante du semaphore 0

  fprintf(stderr,"(CONSULTATION %d) Prise bloquante du sémaphore 0\n",getpid());

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(CONSULTATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  sprintf(requete,"select * from UNIX_FINAL where lower(nom) like lower('%s')", m.data1);
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);
  if(resultat==NULL)
  {
     fprintf(stderr, "Erreur MySQL : %s\n", mysql_error(connexion));
  }
  else
  {
    if((tuple = mysql_fetch_row(resultat)) != NULL)
    {
     
      // Construction et envoi de la reponse
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      strcpy(reponse.data1, "OK");
      strcpy(reponse.data2, tuple[2]);
      strcpy(reponse.texte, tuple[3]);
     
    }
    else
    {
  
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
    
      strcpy(reponse.data1, "KO");
    }
    reponse.requete = CONSULT;
    printf("pid client=>%d\n", m.expediteur);
    if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
        perror("Erreur msgsnd CONSULTATION");
        exit(1);
    }
    kill(m.expediteur, SIGUSR1);
    printf("kill envoyer au client\n");
  }

  
  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  operations[1].sem_num=0;
  operations[1].sem_op=+1;
  operations[1].sem_flg=0;

  if(semop(idSem, operations,1)==-1)
  {
    perror("Erreur semop");
  }
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());

  exit(1);
}