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
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierUtilisateur.h"


int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;

void afficheTab();

MYSQL* connexion;
void HandlerSIGINT(int);
void AjoutEtRecherche(MESSAGE m);
int main()
{


    int i,k,j;
    MESSAGE m;
    //MESSAGE reponse;
    char requete[200];
    MYSQL_RES  *resultat;
    MYSQL_ROW  tuple;
    PUBLICITE pub;
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
   

    // Creation des ressources
    fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
    if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
    {
      perror("(SERVEUR) Erreur de msgget");
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

    
    

    while(1)
    {
    	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
      if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
      {
        perror("(SERVEUR) Erreur de msgrcv");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
      }

      switch(m.requete)
      {
        case CONNECT :  
                        fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                        for(int i=0; i < 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre ==0)
                          {
                            
                            tab->connexions[i].pidFenetre = m.expediteur;
                            printf("je suis dans CONNECT \n");
                            i=6;

                          }
                           afficheTab();
                        }
                        break; 

        case DECONNECT :  
                        fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);

                        for(int i=0; i < 6; i++)
                        {
                          if(tab->connexions[i].pidFenetre==m.expediteur)
                          {
                            printf("je suis dans DECONNECT\n");
                              tab->connexions[i].pidFenetre=0;
                              i=6;
                          }
                        }

                        afficheTab();

                        break; 




        case LOGIN :  
                        fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);
                        
                        AjoutEtRecherche(m);
                        kill(m.expediteur, SIGUSR1);
                        

                      break; 

        case LOGOUT :  
                
                        fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                        for(i=0; i < 6; i++)
                        {
                          printf("i=%d\n",i);
                          if(tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            printf("YYVVYytTY\n");
                            strcpy(tab->connexions[i].nom,"");
                            i=6;
                           
                          }
                          
                        }
                        break;


        case ACCEPT_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                        break;

        case REFUSE_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
                        break;

        case SEND :  
                        fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                        break; 

        case UPDATE_PUB :
                        fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                        break;

        case CONSULT :
                        fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                        break;

        case MODIF1 :
                        fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                        break;

        case MODIF2 :
                        fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                        break;

        case LOGIN_ADMIN :
                        fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                        break;

        case LOGOUT_ADMIN :
                        fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                        break;

        case NEW_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                        break;

        case DELETE_USER :
                        fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                        break;

        case NEW_PUB :
                        fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
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
}

void AjoutEtRecherche(MESSAGE m)
{
  int fd, verif, ret;
    UTILISATEUR vecteur[100];
    int type, i;
    int valRet, val;
    MESSAGE reponse;

    type = m.expediteur;
    if(strcmp(m.data1, "1")==0 )
    {
        printf("etape1\n");
        verif = estPresent(m.data2);
        printf("verif %d\n", verif);
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
            printf("utilisateur deja estPresent");
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
                       

}

