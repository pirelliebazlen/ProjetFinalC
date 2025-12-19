#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string>
#include <stdio.h>

int idQ;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    if((idQ= msgget(CLE, 0))==-1)
    {
        perror("erreur msgget Admin");
        exit(0);
    }

    // Envoi d'une requete de connexion au serveur
    MESSAGE m;
    long type;
    m.type=1;
    m.expediteur = getpid();
    m.requete=LOGIN_ADMIN;
    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
        perror("erreur de msgsnd");
        exit(0);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());
    memset(&m, 0, sizeof(MESSAGE));
    type=getpid();
    if(msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long),type, 0 )==-1)
    {
        perror("erreur msgrcv Admin");
        exit(0);
    }
    if(strcmp(m.data1, "OK")==0)
    {
        QApplication a(argc, argv);
        WindowAdmin w;
        w.show();
        return a.exec();
    }
   return 0;
    
}
