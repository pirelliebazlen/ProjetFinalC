#include "FichierUtilisateur.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

int estPresent(const char* nom)
{
  // TO DO
  int fd;
  UTILISATEUR maStruct;
  int cpt =0;
  int r;
  if((fd= open(FICHIER_UTILISATEURS, O_RDONLY))==-1)
  {
    printf("je n'arrive pas a ouvrir le F\n");
    
    return fd;
  }
  else
  {

      while((r =read(fd,&maStruct ,sizeof(UTILISATEUR)))== sizeof(UTILISATEUR))
      {
        cpt++;
        if(strcmp(maStruct.nom, nom)==0)
        {
          
          close(fd);
          return cpt;
        }

      }
      if (r==-1)
      {
        perror("Erreur lors de la lecteur");

      }

      close(fd);

    }
    return 0;
    
  }
   

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  // TO DO
  int hash=0, i =0;

  while(i < strlen(motDePasse))
  {

    hash += (int)motDePasse[i] * (i+1);
    i++;
  }
  hash = hash%97;
  return hash;
}

////////////////////////////////////////////////////////////////////////////////////
int ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  // TO DO
  int fd;
  UTILISATEUR *util;
  UTILISATEUR tmp;
  int cmpt=0;
  int ha;
  int trouve;
  if((fd=open(FICHIER_UTILISATEURS, O_RDWR|O_CREAT, 0777))==-1)
  {
    printf("Erreur");
    return -1;
    
  }
  else
  {

    util = (UTILISATEUR*) malloc(sizeof(UTILISATEUR));
    if(util== NULL)
    {
      printf("erreur malloc\n");
      return -2;
    }
    else
    {
      while(read(fd, &tmp, sizeof(UTILISATEUR))==sizeof(UTILISATEUR))
      {
        

        if(tmp.hash==0)
        {
        
          strcpy(util->nom, nom);
          util->hash = hash(motDePasse);
          lseek(fd, cmpt, SEEK_SET);
          write(fd, util, sizeof(UTILISATEUR));
          trouve=0;
          break;
        }
        cmpt+=sizeof(UTILISATEUR);
      }

      if(trouve!=0)
      {
      
        strcpy(util->nom, nom);
        util->hash = hash(motDePasse);
        write(fd, util, sizeof(UTILISATEUR));
      }
      
      close(fd);
      free(util);
      printf(" Nouvel utilisateur créé : bienvenue ! \n");
      return 1;
  
    }
    
   return 0;
  }

}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  // TO DO
  UTILISATEUR util;
  int fd;
  int hash1;
  if((fd=open(FICHIER_UTILISATEURS, O_RDONLY))==-1)
  {
    return -1;
  }
  else
  {

   
    int valSEEK=lseek(fd, pos*sizeof(UTILISATEUR), SEEK_SET);
    printf("valSEEK=>%d\n", valSEEK);
    read(fd, &util, sizeof(UTILISATEUR));
    printf("nom=>%s----hash=%d\n", util.nom, util.hash);
    close(fd);

    hash1=hash(motDePasse);
    printf("hash1 %d\n hash %d\n", hash1, util.hash);
    if(hash1 == util.hash)
    {
      return 1;
    }
  }
  close(fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  // TO DO
  int fd, cmpt=0, i=0;
  UTILISATEUR util;
  if((fd=open(FICHIER_UTILISATEURS, O_RDONLY))==-1)
  {
    printf("j'ouvre pas \n");
    return fd;
    
  }
  else
  {
    while(read(fd, &util, sizeof(UTILISATEUR))== sizeof(UTILISATEUR))
    {
     
      cmpt++;
    }

    lseek(fd, 0, SEEK_SET);
    while(i <= cmpt && read(fd, &util, sizeof(UTILISATEUR))== sizeof(UTILISATEUR))
    {
      //read(fd, &util, sizeof(UTILISATEUR));
      vecteur[i]=util;
      printf("nom=>%s----hash=>%d\n", util.nom, util.hash);
      i++;

    }
    close(fd);
    return cmpt;

  }
  return 0;
}
