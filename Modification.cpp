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
#include <fcntl.h>
#include "protocole.h"
#include "FichierUtilisateur.h"

int idQ,idSem;
union semun
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
} arg;

int main()
{
  char nom[40];
  int fd;
  int cmpt=0;
  UTILISATEUR util;
  struct sembuf operation;
  unsigned short val;
  MESSAGE reponse;

  // Recuperation de l'identifiant de la file de messages
  if((idQ=msgget(CLE, 0))==-1)
  {
    perror("Erreur msgget Modification");
    exit(1);
  }
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());

  // Recuperation de l'identifiant du sémaphore
  if((idSem=semget(CLE, 0, 0))==-1)
  {
    perror("Erreur semget Modification");
    exit(1);
  }


  MESSAGE m;
  long type=getpid();
  // Lecture de la requête MODIF1
  if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), type, 0)==-1)
  {
    perror("Erreur msgrcv Modification");
    exit(1);
  }
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());

  val=semctl(idSem, 0, GETVAL);
  
  printf("val=> %d\n", val);
  
  if(val==-1)
  {
    perror("Erreur de semctl Modification");
  }
  else
  {
    if(val ==0)
    {
      reponse.expediteur=getpid();
      reponse.type= m.expediteur;
      reponse.requete = MODIF2;
      strcpy(reponse.data1,"KO");
      strcpy(reponse.data2,"KO");
      strcpy(reponse.texte,"KO");
      if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur msgsnd dans semctl");
      }
    

    }
    else
    {
      printf("Valeur de semaphore avant =>%hd\n", val);
      operation.sem_num=0;
      operation.sem_op=-1;
      operation.sem_flg=0;
      if(semop(idSem, &operation,1)==-1)
      {
        perror("Erreur semop Modification");
      }
      val=semctl(idSem, 0, GETVAL);
      printf("Valeur de semaphore=> apres %hd\n", val);
      // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateut est déjà en train de modifier)

      // Connexion à la base de donnée
      MYSQL *connexion = mysql_init(NULL);
      fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
      if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
      {
        fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
        exit(1);  
      }

      // Recherche des infos dans la base de données
      fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(),m.data1);
      strcpy(nom,m.data1);
      MYSQL_RES  *resultat;
      MYSQL_ROW  tuple;
      char requete[200];
     
      if(m.requete == MODIF1)
      {
        sprintf(requete,"select * from UNIX_FINAL where lower(nom) like lower('%s')", m.data1);
        mysql_query(connexion,requete);
        resultat = mysql_store_result(connexion);
        if(resultat == NULL)
        {
          fprintf(stderr, "Erreur mysql: %s\n", mysql_error(connexion));
        }
        else
        {
        reponse.type= m.expediteur;
          reponse.expediteur=getpid();
          if((tuple = mysql_fetch_row(resultat))!=NULL) // user existe forcement
          {
            
            reponse.requete = MODIF1;
            strcpy(reponse.data2,tuple[2]);
            strcpy(reponse.texte,tuple[3]);
            strcpy(reponse.data1, "OK");
          }
          else
          {
            reponse.requete = MODIF1;
            strcpy(reponse.data2,"KO");
            strcpy(reponse.texte,"KO");
            strcpy(reponse.data1, "KO");
          }
          
          // Construction et envoi de la reponse
          if(msgsnd(idQ, &reponse, sizeof(MESSAGE)- sizeof(long), 0)==-1)
          {
            perror("Erreur msgsnd");
            exit(1);
          }
          fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());
        
        }
        // Attente de la requête MODIF2
      }

      fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
      if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), type, 0)==-1)
      {
        perror("Erreur msgrcv Modification");
        exit(1);
      }
      if(m.requete == MODIF2)
      {
          // Mise à jour base de données
        fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),nom);
    
        sprintf(requete,"UPDATE UNIX_FINAL SET gsm = '%s' where lower(nom) like ('%s') ", m.data2, tuple[1]);
        mysql_query(connexion,requete);
        sprintf(requete,"UPDATE UNIX_FINAL SET email = '%s' where lower(nom) like ('%s') ", m.texte, tuple[1]);

        if(mysql_query(connexion,requete)!=0)
        {
           fprintf(stderr, "Erreur MySQL : %s\n", mysql_error(connexion));
        }
      
        // Mise à jour du fichier si nouveau mot de passe
        char mdp[50];
        strcpy(mdp, m.data1);
        if(strcmp(mdp,"")!=0)
        {
        
          if((fd= open("utilisateurs.dat", O_RDWR))==-1)
          {
            perror("Erreur de open()");
            exit(1);
          }

          while(read(fd, &util, sizeof(UTILISATEUR))==sizeof(UTILISATEUR))
          {
              
          
            if(strcmp(util.nom, tuple[1])==0)
            {

              util.hash = hash(m.data1);
              lseek(fd,cmpt, SEEK_SET);
              write(fd, &util, sizeof(UTILISATEUR));
              break;
            }
            cmpt+=sizeof(UTILISATEUR);

          }
        }
      }
      // Deconnexion BD
      mysql_close(connexion);

      // Libération du semaphore 0
      operation.sem_op=+1;
      if(semop(idSem, &operation,1)==-1)
      {
        perror("Erreur semop Modification");
      }
      val=semctl(idSem, 0, GETVAL);
      printf("Valeur de semaphore=> apres liberation %hd\n", val);
      fprintf(stderr,"(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  
      exit(0);
    }

  }
  return 0;
     
}