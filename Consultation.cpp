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

int main()
{
  // Recuperation de l'identifiant de la file de messages
  if((idQ=msgget(CLE, 0))==-1)
  {
    perror("Erreur msgget");
    exit(1);
  }

  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
  printf("idQ Consultation=> %d\n", idQ);
  // Recuperation de l'identifiant du sémaphore
  /*if((idSem=semget(CLE, 0, 0))==-1)
  {
    perror("Erreur semget CONSULT");
  }*/


  MESSAGE m;
  long type;
  type=getpid();
  // Lecture de la requête CONSULT
  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), type, 0)==-1)
  {
    perror("Erreur msgrcv CONSULTATION");
    exit(1);
  }
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  printf("m.data1 =%s\n", m.data1);
  printf("m.data2 =%s\n", m.data2);
  printf("m.texte =%s\n", m.texte);
  printf("m.expediteur =%d\n", m.expediteur);
  
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
  MESSAGE reponse;
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
      printf("tupe[0]=>%s\n", tuple[0]);
      printf("tupe[1]=>%s\n", tuple[1]);
      printf("tupe[2]=>%s\n", tuple[2]);
      printf("tupe[3]=>%s\n", tuple[3]);
      // Construction et envoi de la reponse
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      strcpy(reponse.data1, "OK");
      strcpy(reponse.data2, tuple[2]);
      strcpy(reponse.texte, tuple[3]);
     
    }
    else
    {
      printf("Null");
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      printf("m.exp = %d\n", m.expediteur);
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
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());

  exit(1);
}