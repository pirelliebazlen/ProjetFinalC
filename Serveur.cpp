#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <cerrno>
#include <setjmp.h>
#include <sys/sem.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierUtilisateur.h"


int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;
int pid, idfils, idfils2, idfils3;
int pidAdministrateur=0;
union semun {
    int val;               // Pour SETVAL
    struct semid_ds *buf;  // Pour IPC_STAT, IPC_SET
    unsigned short *array; // Pour SETALL
   
}arg;

void afficheTab();

MYSQL* connexion;
void HandlerSIGINT(int sig);
void HandlerSIGCHLD(int sig);
void Login(MESSAGE m);
void RemoveUserOnTheTable(MESSAGE message, MESSAGE reponse);
void RefuseUser(MESSAGE m);
void acceptUser(MESSAGE m);
void envoisMessage(MESSAGE m);
void NewUser(MESSAGE m);
void DelUser(MESSAGE m);
void loginAdmin(MESSAGE m);
void publicit(MESSAGE m);
int main()
{
    int i,k,j;
    
    MESSAGE m;
    MESSAGE reponse;
    char requete[200];
    MYSQL_RES  *resultat;
    MYSQL_ROW  tuple;
    PUBLICITE pub;
    char taille[200];
    char * pShm;
   
    // Connection à la BD
    connexion = mysql_init(NULL);
    if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
    {
      fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
      exit(1);  
    }

    // Armement des signaux
    struct sigaction A;
    A.sa_handler= HandlerSIGINT;
    A.sa_flags=0;
    sigemptyset(&A.sa_mask);

    if(sigaction(SIGINT, &A, NULL)==-1)
    {
      perror("Erreur sigaction");
      exit(0);
    }
    struct sigaction B;
    B.sa_handler = HandlerSIGCHLD;
    sigemptyset(&B.sa_mask);
    B.sa_flags = 0;
    if (sigaction(SIGCHLD,&B,NULL) ==-1)
    {
      perror("Erreur de sigaction");
      exit(1);
    }
   
    // Creation des ressources
    fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
    if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
    {
      perror("(SERVEUR) Erreur de msgget");
      exit(1);
    }

    if((idSem = semget(CLE, 1, IPC_CREAT | IPC_EXCL | 0600)) ==-1)
    {
      perror("Erreur Creation Semaphore\n");
      exit(1);
    }
    printf("Semid=> %d\n", idSem);

    arg.val=1;
    if(semctl(idSem, 0, SETVAL, arg)==-1)
    {
      perror("Erreur de semctl Semaphore");
      exit(1);
    }

    // Initialisation du tableau de connexions
    fprintf(stderr,"(SERVEUR %d) Initialisation de la table des connexions\n",getpid());
    tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 
  
    for (int i=0 ; i<6 ; i++)
    {
      tab->connexions[i].pidFenetre = 0;
      strcpy(tab->connexions[i].nom,"");

      for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
      tab->connexions[i].pidModification = 0;
    }
    tab->pidServeur1 = getpid();
    tab->pidServeur2 = 0;
    tab->pidAdmin = 0;
    tab->pidPublicite = 0;

    afficheTab();

    // Creation du processus Publicite
    if((idShm= shmget(CLE,200 * sizeof(char),IPC_CREAT|IPC_EXCL|0600))==-1)
    {
      perror("erreur shmget");
      exit(0);
    }
    printf("idshm=> %d\n", idSem);

    if((pShm = (char*)shmat(idShm, NULL,0))==(char*)-1)
    {
      perror("Erreur de shmat");
      exit(1);
    }
    printf("pShm %p\n",pShm);
    if((idfils=fork()) == -1)
    {
      perror("Erreur fork\n");
      exit(0);
    }

    printf("idfils %d\n", idfils);
    if(idfils==0)
    {
      if(execlp("Publicite","Publicite", NULL)==-1)
      {
        perror("Erreur execlp");
        exit(1);
      }
    
    }
    //printf("mem partager Serveur=> %s\n", pShm);

    while(1)
    {
    	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
      if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
      {
        perror("(SERVEUR) Erreur de msgrcv");
        if (errno == EINTR) continue;   //EINTR “Interrupted system call"
        msgctl(idQ,IPC_RMID,NULL);
        shmctl(idShm,IPC_RMID, NULL);
        exit(1);
      }

      switch(m.requete)
      {
        case CONNECT :  
                        fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                        for(i=0; i < 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre ==0)
                          {
                            
                            tab->connexions[i].pidFenetre = m.expediteur;
                            i=6;

                          }
                           
                        }
                        
                        break; 

        case DECONNECT :  
                        fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);

                        for(i=0; i < 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre==m.expediteur)
                          {
                            printf("je suis dans DECONNECT\n");
                              tab->connexions[i].pidFenetre=0;
                              strcpy(tab->connexions[i].nom, "");
                              tab->connexions[i].pidModification=0;
                              i=6;
                          }
                        }

                        afficheTab();

                        break; 

        case LOGIN :  
                        fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);
                        
                        Login(m);

                      break; 

        case LOGOUT :  
                
                        fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                        
                        RemoveUserOnTheTable(m, reponse);
                        break;


        case ACCEPT_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                        acceptUser(m);

                        break;

        case REFUSE_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);

                        RefuseUser(m);
                        break;

        case SEND :  
                        fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                        envoisMessage(m);
                        break; 

        case UPDATE_PUB :
                        fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                        tab->pidPublicite = m.expediteur;
                        for(i=0; i< 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre!=0)
                          {
                            kill(tab->connexions[i].pidFenetre, SIGUSR2);
                            printf("SIGUSR2 envoyé");
                          }
                        }
                        break;

        case CONSULT :
                        fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                        if((idfils2=fork())==-1)
                        {
                            perror("Erreur fork()");
                            exit(0);
                        }
                        if(idfils2==0)
                        {
                          if(execlp("Consultation","Consultation",NULL)==-1)
                          {
                            perror("Erreur execlp");
                            exit(1);
                          }
                          

                        }
                        m.type=idfils2;
                        if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
                        {
                          perror("Erreur msgsnd SERVEUR to CONSULTATION");
                          exit(1);
                        }
                        break;

        case MODIF1 :
                        fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                        if((idfils3=fork())==-1)
                        {
                            perror("Erreur fork()");
                            exit(0);
                        }
                        
                        if(idfils3==0)
                        {
                          if(execlp("Modification", "Modification", NULL)==-1)
                          {
                            perror("Erreur execlp Modification");
                            exit(1);
                          }
                           
                        }
                        for(i=0; i < 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre ==m.expediteur)
                          {

                            tab->connexions[i].pidModification=idfils3;
                            m.type=idfils3;
                            strcpy(m.data1, tab->connexions[i].nom);
                  
                            if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
                            {
                              
                              perror("Erreur msgsnd SERVEUR to CONSULTATION");
                              exit(1);
                            }
                            i=6;
                          }
                        }
                      

                        break;

        case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                        for(i=0; i < 6; i++)
                        {
                            if(tab->connexions[i].pidFenetre ==m.expediteur)
                            {
                               m.type=tab->connexions[i].pidModification;
                              
                               if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
                               {
                                  perror("Erreur msgsnd SERVEUR to Modification");
                                  exit(1);
                               }
                               i=6;
                            } 
                        }
                        break;

        case LOGIN_ADMIN :
                        fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                        loginAdmin(m);
                        break;

        case LOGOUT_ADMIN :
                        fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                        pidAdministrateur=0;
                        break;

        case NEW_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                        NewUser(m);
                        break;

        case DELETE_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                        DelUser(m);
                        break;

        case NEW_PUB :
                        fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                        publicit(m);
                        break;

      }
        afficheTab();
    }
  
}


void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
  {
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  }
  fprintf(stderr,"\n");
}
void HandlerSIGINT(int sig)
{

  if(msgctl(idQ, IPC_RMID, NULL)==-1)
  {
    perror("HandlerSIGINT: Erreur msgctl");
    exit(1);
  }
  if(shmctl(idShm,IPC_RMID, NULL)==-1)
  {
    perror("HandlerSIGINT: Erreur shmctl");
    exit(1);

  }
  if (semctl(idSem,0,IPC_RMID) ==-1)
  {
    perror("Erreur de semctl (3)");
    exit(1);
  }
  //kill(tab->pidPublicite, SIGINT);

}

void HandlerSIGCHLD(int sig)
{
  int id , status, i;
  pid_t pid;
  
  id = wait(&status);
  if(id==idfils3)
  {

    for(i=0; i < 6; i++)
    {
      if(tab->connexions[i].pidModification == id)
      {
        tab->connexions[i].pidModification =0;
        i=6;
    
      }
    }
  }

}
void loginAdmin(MESSAGE m)
{
  MESSAGE reponse;
  if(pidAdministrateur==0)
  {
    pidAdministrateur=m.expediteur;
    reponse.type=m.expediteur;
    reponse.expediteur=getpid();
    reponse.requete=LOGIN_ADMIN;
    strcpy(reponse.data1, "OK");
    if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd SERVEUR LOGIN_ADMIN");
      exit(0);
    }
    printf("LOGIN_ADMIN OK\n");

  }
  else
  {
    reponse.type=m.expediteur;
    reponse.expediteur=getpid();
    reponse.requete=LOGIN_ADMIN;
    strcpy(reponse.data1, "KO");
    if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd SERVEUR LOGIN_ADMIN");
      exit(0);
    }
    printf("LOGIN_ADMIN K.O\n");

  }
}
void NewUser(MESSAGE m)
{
  int verif, valRetAjout;
  MESSAGE reponse;
  char GSM[20];
  char email[100];
  char requete[256];

  verif= estPresent(m.data1);
  printf("verif=> %d\n", verif);
  if(verif==0)
  {
    valRetAjout=ajouteUtilisateur(m.data1, m.data2);
    if(valRetAjout==1)
    {

      fprintf(stderr, "Ajout ok");
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      reponse.requete=NEW_USER;
      strcpy(reponse.data1, "OK");
      strcpy(reponse.texte, "Ajout avec succés");
      strcpy(GSM, "---");
      strcpy(email, "---");
      sprintf(requete, "insert into UNIX_FINAL values (NULL,'%s', '%s', '%s');", m.data1, GSM, email);
      mysql_query(connexion, requete);
    }
    else
    {
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      reponse.requete=NEW_USER;
      strcpy(reponse.data1, "KO");
      strcpy(reponse.texte, "utilisateur n'a pas pu etre enregistré");
    }
    
  }
  else
  {
    if(verif==-1) //quand fichier pas encore recrée
    {
        valRetAjout=ajouteUtilisateur(m.data1, m.data2);
        if(valRetAjout==1)
        {

          fprintf(stderr, "Ajout ok");
          reponse.expediteur=getpid();
          reponse.type=m.expediteur;
          reponse.requete=NEW_USER;
          strcpy(reponse.data1, "OK");
          strcpy(reponse.texte, "Ajout avec succés");
          strcpy(GSM, "---");
          strcpy(email, "---");
          sprintf(requete, "insert into UNIX_FINAL values (NULL,'%s', '%s', '%s');", m.data1, GSM, email);
          mysql_query(connexion, requete);
        }
        else
        {
          reponse.expediteur=getpid();
          reponse.type=m.expediteur;
          reponse.requete=NEW_USER;
          strcpy(reponse.data1, "KO");
          strcpy(reponse.texte, "utilisateur n'a pas pu etre enregistré");
        }
    }
    else
    {
      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      reponse.requete=NEW_USER;
      strcpy(reponse.data1, "KO");
      strcpy(reponse.texte, "utilisateur n'a pas été trouver");
    }
  }

  if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long),0)==-1)
  {
    perror("Erreur msgsnd NewUser");
    exit(0);
  }
}


void DelUser(MESSAGE m)
{
  int fd, cmpt=0;
  UTILISATEUR util;
  MESSAGE reponse;
  char requete[256];
  fd= open(FICHIER_UTILISATEURS, O_RDWR);
  if(fd==-1)
  {
    perror("Erreur d'ouverture de fichier\n");
    exit(0);
  }
  while(read(fd, &util, sizeof(UTILISATEUR))==sizeof(UTILISATEUR))
  {
    
    if(strcmp(util.nom, m.data1)==0)
    {
      strcpy(util.nom, "");
      util.hash=0;
      lseek(fd, cmpt, SEEK_SET);
      write(fd, &util, sizeof(UTILISATEUR));

      sprintf(requete, "DELETE from UNIX_FINAL where lower(nom) like lower('%s')", m.data1);
      mysql_query(connexion, requete);

      reponse.expediteur=getpid();
      reponse.type=m.expediteur;
      reponse.requete=DELETE_USER;
      strcpy(reponse.data1, "OK");
      strcpy(reponse.texte, "utilisateur supprimer");
      if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long),0)==-1)
      {
        perror("Erreur msgsnd NewUser");
        exit(0);
      }
      cmpt=0;
      break;
    }
    cmpt+=sizeof(UTILISATEUR);
  }

  if(cmpt> 0)
  {
    reponse.expediteur=getpid();
    reponse.type=m.expediteur;
    reponse.requete=DELETE_USER;
    strcpy(reponse.data1, "KO");
    strcpy(reponse.texte, "utilisateur n'a pas été trouver");
    if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long),0)==-1)
    {
      perror("Erreur msgsnd NewUser");
      exit(0);
    }
  }
  close(fd);

}


void Login(MESSAGE m)
{
  int fd, verif,j, ret;
  UTILISATEUR vecteur[100];
  int type, i;
  int valRet, val;
  char *nom2;
  char *nom1;
  MESSAGE reponse;
  MESSAGE autre;

  type = m.expediteur;
  if(strcmp(m.data1, "1")==0 )
  {
    verif = estPresent(m.data2);
    if (verif<=0)
    {
      valRet = ajouteUtilisateur(m.data2, m.texte);
      if(valRet == 1)
      {
        fprintf(stderr, "Ajout ok");
        reponse.type = m.expediteur;
      
        reponse.expediteur=1;
        reponse.requete= 3;
        strcpy(reponse.data1, "OK");
        strcpy(reponse.texte, "utilisateur enregistré avec succés");
        
        for(i=0; i < 6; i++)
        {

          if(tab->connexions[i].pidFenetre == m.expediteur)
          {
            strcpy(tab->connexions[i].nom, m.data2);
          }
        }
      }
      else
      {
        reponse.type = m.expediteur;
        reponse.expediteur=1;
        reponse.requete= 3;
        strcpy(reponse.data1, "KO");
        strcpy(reponse.texte, "l'utilisateur n'a pas pu etre enregistré");
      }
                             
    }
    else
    {
      if(verif >1)
      {
      
        reponse.type = m.expediteur;
        reponse.expediteur=1;
        reponse.requete= 3;
        strcpy(reponse.data1, "KO");
        strcpy(reponse.texte, "l'utilisateur n'a pas pu etre enregistré");
      }
      if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgrcv\n");
        exit(1);
      }
                          
    }
                            
  }
  if(strcmp(m.data1, "0")==0 )
  {

    verif = estPresent(m.data2);
    if (verif>0)
    {
      val = verifieMotDePasse(verif, m.texte);
                    
      if(val == 1)
      {
        printf("Ajout ok\n");
        reponse.type = m.expediteur;
        reponse.expediteur=1;
        reponse.requete= 3;
        strcpy(reponse.data1, "OK");
        strcpy(reponse.texte, "ReBonjour cher utilisateur");

        for(i=0; i < 6; i++)
        {

          if(tab->connexions[i].pidFenetre == m.expediteur)
          {
            strcpy(tab->connexions[i].nom, m.data2);
          }
        }
      }
      else
      {
        reponse.type = m.expediteur;
        reponse.expediteur=1;
        reponse.requete= 3;
        strcpy(reponse.data1, "KO");
        strcpy(reponse.texte, "Mot de passe incorrect");
      }

    }
    else
    {
          reponse.type = m.expediteur;
          reponse.expediteur=1;
          reponse.requete= 3;
          strcpy(reponse.data1, "KO");
          strcpy(reponse.texte, "utilisateur introuvable");
    }
                         
  }
  ret= listeUtilisateurs(vecteur);

  if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0)==-1)
  {
    perror("Erreur de msgrcv\n");
    exit(1);
  }
  kill(m.expediteur, SIGUSR1);

  if(strcmp(reponse.data1, "OK")==0)
  {
        
    reponse.expediteur= 1;
    reponse.requete = 5;
    for(i=0; i< 6; i++)
    {

      if(tab->connexions[i].pidFenetre!=0 && strcmp(tab->connexions[i].nom, "")!=0)
      {
      
        reponse.type = m.expediteur;
        ret = strcmp(tab->connexions[i].nom, m.data2); 
        
        if(ret!=0)
        {
          
          strcpy(reponse.data1,tab->connexions[i].nom);
          if(msgsnd(idQ, &reponse, sizeof(MESSAGE)- sizeof(long), 0) == -1)
          {
            perror("Erreur de msgrcv reponse\n");
            exit(1);
          }
          kill(m.expediteur, SIGUSR1);

          //Envois aux autres
          autre.expediteur= 1;
          autre.requete = 5;
          autre.type = tab->connexions[i].pidFenetre;
          strcpy(autre.data1,m.data2);
          if(msgsnd(idQ, &autre, sizeof(MESSAGE)- sizeof(long), 0) == -1)
          {
            perror("Erreur de msgrcv reponse\n");
            exit(1);
          }
          
          kill(tab->connexions[i].pidFenetre, SIGUSR1);
        }

      }
      if(tab->connexions[i+1].pidFenetre==0)
      {
        i=6;
      } 
    }
  }
}

void RemoveUserOnTheTable(MESSAGE message, MESSAGE reponse)
{
  int i, j;
  char nom[50];

   for(i=0; i < 6; i++)
   {
     printf("i=%d\n",i);
     if(tab->connexions[i].pidFenetre == message.expediteur)
     {
        strcpy(nom,tab->connexions[i].nom);
        strcpy(tab->connexions[i].nom,"");
        tab->connexions[i].pidModification=0;
        i=6;
                           
     }
                          
   }
  for(i=0; i < 6; i++)
  {
    if(tab->connexions[i].pidFenetre != 0 && tab->connexions[i].pidFenetre !=message.expediteur)
    {
      reponse.expediteur = 1;
      reponse.type = tab->connexions[i].pidFenetre;
      reponse.requete = 6;
      strcpy(reponse.data1,nom);
      if(msgsnd(idQ, &reponse, sizeof(MESSAGE)-sizeof(long), 0) ==-1)
      {
        perror("erreur de msgsnd\n");
        exit(1);

      }
      kill(tab->connexions[i].pidFenetre, SIGUSR1);
    }
  }

  for(i=0; i < 6; i++)
  {
  
      for(j=0; j < 5; j++)
      {
        if(tab->connexions[i].autres[j]==message.expediteur)
        {
          tab->connexions[i].autres[j]=0;
        }
      }
    
  }


}
void RefuseUser(MESSAGE m)
{

  int i, j, pid;
  for(i=0; i < 5; i++)
  {
    if(strcmp(tab->connexions[i].nom, m.data1)==0)
    {
      pid = tab->connexions[i].pidFenetre;
                            
      i=6;
    }
  }
  for(i=0; i < 5; i++)
  {
    if(tab->connexions[i].pidFenetre == m.expediteur)
    {
      for(j=0; j < 5; j++)
      {
        if(tab->connexions[i].autres[j]==pid)
        {
          tab->connexions[i].autres[j]=0;
        }
      }
    }
  }
}

void acceptUser(MESSAGE m)
{
  int i, j, pid=0;

  for(i=0; i < 5; i++)
  {
      if(strcmp(tab->connexions[i].nom, m.data1)==0)
      {
         pid = tab->connexions[i].pidFenetre;                    
      }
                        
  }

  for(i=0; i < 5; i++)
  {
      if(tab->connexions[i].pidFenetre == m.expediteur)
      {
        for(j=0; j< 5; j++)
        {
          if(tab->connexions[i].autres[j] ==0 )
          {
        
            tab->connexions[i].autres[j] = pid;
            i=5;
            j=5;
          }

        }
      }
                          
  }
}

void envoisMessage(MESSAGE m)
{
  int i, j;
  MESSAGE transfer;
  for(i=0; i < 5; i++)
  {
    if(tab->connexions[i].pidFenetre == m.expediteur)
    {
      strcpy(transfer.data1, tab->connexions[i].nom);
      i=5;
    }
  }
  strcpy(transfer.texte, m.texte);
  
  for(i=0; i < 5; i++)
  {
   
    for(j=0; j <5; j++)
    {
        
        if(tab->connexions[i].autres[j] == m.expediteur && tab->connexions[i].autres[j]!=0)
        {
          
          transfer.expediteur=1;
          transfer.type=tab->connexions[i].pidFenetre;
          transfer.requete= m.requete;
          if(msgsnd(idQ, &transfer, sizeof(MESSAGE)-sizeof(long), 0)==-1)
          {
            perror("erreur msgsnd");
            exit(1);
          }
          kill(tab->connexions[i].pidFenetre, SIGUSR1);
          j=5;
        }
    }
    if(tab->connexions[i].pidFenetre==0)
    {
      i=5;
    }
  }  
}
void publicit(MESSAGE m)
{
  PUBLICITE pub, pubEcris;
  int fd, trouve=0, cmpt=0;
  fd = open("publicites.dat", O_RDWR|O_CREAT, 0777);
  if(fd==-1)
  {
    perror("Erreur d'ouverture");
    
  }
  else
  {
    cmpt=cmpt-sizeof(PUBLICITE);
    while(read(fd, &pub, sizeof(PUBLICITE))==sizeof(PUBLICITE))
    {
      printf("pub=> %s\t nbSecondes=> %d\n", pub.texte, pub.nbSecondes);
      if(strcmp(pub.texte, m.texte)==0)
      {
        trouve=1;
      }
      cmpt+=sizeof(PUBLICITE);
    }
    if(trouve==0)
    {
      strcpy(pubEcris.texte, m.texte);
      pubEcris.nbSecondes = atoi(m.data1);
      write(fd, &pubEcris, sizeof(MESSAGE));
    }
    if(cmpt==0)
    {
      kill(tab->pidPublicite, SIGUSR1);
    }
    close(fd);
  }

  
}