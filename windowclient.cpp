#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include <sys/types.h>
 #include <sys/ipc.h>
 #include <sys/msg.h>
#include "dialogmodification.h"
#include <unistd.h>
 #include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm;
char *pShm;
#define TIME_OUT 120
int timeOut = TIME_OUT;

void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);
void HandlerALRM(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    
    ui->setupUi(this);
    ::close(2);
    logoutOK();


    // Recuperation de l'identifiant de la file de messages
     if((idQ = msgget(CLE, 0))==-1)
    {
      perror("Erreur msgget CLIENT");
      exit(0);

    }
    printf("idQ=> %d\n", idQ);
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    if((idShm=shmget(CLE, 0,0))==-1)
    {
      perror("Erreur shmget Client");
      exit(0);
    }

    // Attachement à la mémoire partagée

    if((pShm = (char*)shmat(idShm, NULL, 0))==(char*)-1)
    {
      perror("Erreur shmat Client");
      exit(0);
    }
    // Armement des signaux

    struct sigaction A;
    A.sa_handler = handlerSIGUSR1;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    if(sigaction(SIGUSR1, &A, NULL)==-1)
    {
      perror("Erreur sigaction");
      exit(0);
    }

    A.sa_handler = handlerSIGUSR2;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    if(sigaction(SIGUSR2, &A, NULL)==-1)
    {
      perror("Erreur sigaction");
      exit(0);
    }
    
    A.sa_handler = HandlerALRM;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    if(sigaction(SIGALRM, &A, NULL)==-1)
    {
      perror("Erreur sigaction");
      exit(0);
    }
   
    // Envoi d'une requete de connexion au serveur
    MESSAGE mes;
    mes.type = 1;
    mes.expediteur= getpid();
    mes.requete = 1;
    if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd");
      exit(1);
    }
  
    
}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauChecked()
{
  if (ui->checkBoxNouveau->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  //printf("Text %s\n", Text);
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveau->setEnabled(false);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->checkBoxNouveau->setEnabled(true);
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue /////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Clic sur la croBidonFichierPubix de la fenêtre ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
    // TO DO
     //Envoi une requete deconnection   "MON COMMENTAIRE"

    MESSAGE mes;
    mes.type = 1;
    mes.expediteur= getpid();
    mes.requete = 2;
    if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd");
      exit(1);
    }


    QApplication::exit();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    // TO DO
    MESSAGE mes;

    int check;
    mes.type = 1;
    mes.expediteur= getpid();
    mes.requete = 3;
    check = isNouveauChecked();
    mes.data1[0]= '0'+ check;

    strcpy(mes.data2, getNom());
    strcpy(mes.texte, getMotDePasse());
    if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
        perror("Erreur msgsnd");
        exit(1);
    }
    
    alarm(1);
  
}

void WindowClient::on_pushButtonLogout_clicked()
{
    // TO DO
     MESSAGE mes;
    mes.type = 1;
    mes.expediteur= getpid();
    mes.requete = 4;

    if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd");
      exit(1);
    }

    logoutOK();
    alarm(1);
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
    // TO DO
    MESSAGE mes;
    mes.type = 1;
    mes.expediteur= getpid();
    mes.requete = 9;
    strcpy(mes.texte, getAEnvoyer()); 
    printf("%s\n", mes.texte);
    if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd");
      exit(1);
    }
  

    alarm(1);

}

void WindowClient::on_pushButtonConsulter_clicked()
{
  // TO DO
  MESSAGE mes;
  mes.type=1;
  mes.expediteur=getpid();
  mes.requete=CONSULT;
  strcpy(mes.data1,getNomRenseignements());
  if(msgsnd(idQ,  &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
  {
    perror("Erreur msgsnd CONSULT");
    exit(1);
  }
  printf("CONSULT envoyer\n");
  alarm(1);
}

void WindowClient::on_pushButtonModifier_clicked()
{
  
  // TO DO
  // Envoi d'une requete MODIF1 au serveur
    MESSAGE m;
    m.type = 1;
    m.expediteur= getpid();
    m.requete = MODIF1 ;
    if(msgsnd(idQ,  &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd MODIF1");
      exit(1);
    }


  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...
  long type=getpid();
  int ret;
  memset(&m, 0, sizeof(MESSAGE));
  if((ret=msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long),type, 0))==-1)
  {
    perror("Erreur msgrcv Client");
    exit(0);
  }
  
  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  setMotDePasse(getMotDePasse());
  setGsm(m.data2);
  setEmail(m.texte);

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),"",m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  // ...
  memset(&m, 0, sizeof(MESSAGE));
  if(strcmp(gsm, m.data2)!=0)
  {
     strcpy(m.data2, gsm);
  }
  if(strcmp(email, m.texte)!=0)
  {
     strcpy(m.texte, email);
  }
  if(strlen(motDePasse)!=0)
  {
    strcpy(m.data1, motDePasse);
 
  }
  
  m.type = 1;
  m.expediteur= getpid();
  m.requete = MODIF2 ;
  if(msgsnd(idQ,  &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
  {
    perror("Erreur msgsnd MODIF2");
    exit(1);
  }
  
  alarm(1);
  
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
  MESSAGE m;
  
  m.expediteur= getpid();
  m.type = 1;
  strcpy(m.data1, getPersonneConnectee(1));

    if (checked)
    {
        ui->checkBox1->setText("Accepté");
     
        // TO DO (etape 2)
        m.requete = 7;
        if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
        {
          perror("Erreur de msgsnd\n");
          exit(1);
        }
    }
    else
    {
        m.requete = 8;
        ui->checkBox1->setText("Refusé");

        // TO DO (etape 2)
        if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
        {
          perror("Erreur de msgsnd\n");
          exit(1);
        }
    }
    alarm(1);
    timeOut= TIME_OUT;
    w->setTimeOut(timeOut);
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
  MESSAGE m;
  
  m.expediteur= getpid();
  m.type = 1;
  strcpy(m.data1, getPersonneConnectee(2));
  if (checked)
  {
    ui->checkBox2->setText("Accepté");
   
    // TO DO (etape 2)
    m.requete = 7;
    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur de msgsnd\n");
      exit(1);
    }
  }
  else
  {
    ui->checkBox2->setText("Refusé");
        // TO DO (etape 2)
    m.requete = 8;
    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur de msgsnd\n");
      exit(1);
    }
  }
  alarm(1);
  timeOut= TIME_OUT;
  w->setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    MESSAGE m;
  
    m.expediteur= getpid();
    m.type = 1;
    strcpy(m.data1, getPersonneConnectee(3));
    if (checked)
    {
      ui->checkBox3->setText("Accepté");
      
      // TO DO (etape 2)
      m.requete = 7;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }

    }
    else
    {
        ui->checkBox3->setText("Refusé");
        // TO DO (etape 2)
        m.requete = 8;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }
    }
    alarm(1);
    timeOut= TIME_OUT;
    w->setTimeOut(timeOut);
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    MESSAGE m;
    m.expediteur= getpid();
    m.type = 1;
    strcpy(m.data1, getPersonneConnectee(4));
    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        // TO DO (etape 2)
      m.requete = 7;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        // TO DO (etape 2)
      m.requete = 8;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }
    }
    alarm(1);
    timeOut= TIME_OUT;
    w->setTimeOut(timeOut);
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
    MESSAGE m; 
    m.expediteur= getpid();
    m.type = 1;
    strcpy(m.data1, getPersonneConnectee(5));
    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        // TO DO (etape 2)
        m.requete = 7;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        // TO DO (etape 2)
        m.requete = 8;
      if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0)==-1)
      {
        perror("Erreur de msgsnd\n");
        exit(1);
      }
    }
    alarm(0);
    timeOut= TIME_OUT;
    w->setTimeOut(timeOut);
    alarm(1);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    int cmpt, i;
    long type = getpid();
    const char *nom;
    ssize_t ret;

    while((ret=msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long), type, IPC_NOWAIT)) >0)
    {
      printf("req=>%d\n", m.requete);
  
      switch(m.requete)
      {

        case LOGIN :
                    if (strcmp(m.data1,"OK") == 0)
                    {
                  
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      w->dialogueMessage("Login...",m.texte);
                      // ...
                    }
                    else w->dialogueErreur("Login...",m.texte);
                    alarm(1);
                    timeOut=TIME_OUT;
                    break;

        case ADD_USER :
                    // TO DO
                    
                    for(i=1; i < 6; i++)
                    {
                      nom = w->getPersonneConnectee(i);
                      if(strcmp(nom, "")==0)
                      {
                        cmpt = i;
                        i=6;
                      }
                    
                    }
                  
                    w->setPersonneConnectee(cmpt, m.data1);
                    break;

        case REMOVE_USER :
                    // TO DO
                    for(i=1; i < 6; i++)
                    {
                      nom = w->getPersonneConnectee(i);
                      if(strcmp(m.data1, nom)==0)
                      {
                        w->setPersonneConnectee(i, "");
                        i=6;
                      }
              
                    }
                    break;

        case SEND :
                    // TO DO
                    w->ajouteMessage(m.data1, m.texte);
                    break;

        case CONSULT :
                  // TO DO
                  
                    if(strcmp(m.data1, "OK")==0)
                    {
                      w->setEmail(m.texte);
                      w->setGsm(m.data2);
                    }
                    else
                    {
                      if(strcmp(m.data1,"KO")==0)
                      {
                        w->dialogueErreur("NON TROUVE", "Utilisateur non trouver");
                      }
                      else
                      {
                      
                        w->setGsm(m.data1);
                        w->setEmail(m.data1);
                      }
                    }
                  break;

      }
  

    }
    
}
void HandlerALRM(int sig)
{
  MESSAGE mes;
  

  timeOut--;
  w->setTimeOut(timeOut);
  if(timeOut ==0)
  {
    mes.type=1;
    mes.expediteur=getpid();
    mes.requete=4;
    if(msgsnd(idQ, &mes, sizeof(MESSAGE)-sizeof(long), 0)==-1)
    {
      perror("Erreur msgsnd handlerALARM");
      exit(1);
    }
    w->logoutOK();
  }
    //sleep(1);


  alarm(1);
}
void handlerSIGUSR2(int sig)
{
  printf("je suis dans SIGUSR2");
  w->setPublicite(pShm);
}
