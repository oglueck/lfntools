// lrd

                   
#include <time.h>                   
#include "lfn.cpp"

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's Rd for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: rd [drive:][path\\]directory [/?]\n\n");
  printf("Removes a directory.\n");
  printf("There are no switches.\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Rd fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("RD [Laufwerk:][Pfad\\]Verzeichnis [/?]\n\n");
  printf("Entfernt ein Verzeichnis.\n");
  printf("Es gibt keine Optionen.\n");
 #endif
 exit(0);
}

void ParseParams(char *arg)
{  
 int i;
 for (i=0;arg[i]=='/';i++)
 {                      
   i++;
   switch (toupper(arg[i]))
   {
    case 'H':
    case '?': PrintHelp();
              break;   
    default:
     #ifdef LANG_EN
      printf("Invalid switch.\n");
     #endif
     #ifdef LANG_DE
      printf("Unbekannte Option.\n");
     #endif
     exit(1);
    break;
   }
 }
}

void main(int argc,char *argv[],char* envp[])
{
 struct DPB Disk;
 byte drv;                 
 int i,dpath,slash;
 char *temp=NULL;
 struct longdirentry dir,f;
 char name[MAX_PATH_SIZE],path[MAX_PATH_SIZE],x[MAX_PATH_SIZE];

 dpath=0;
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else 
   dpath=i;    //last parameter other than switch
 }

 if (dpath==0)
 {
  #ifdef LANG_EN
   printf("Too few parameters!");
  #endif
  #ifdef LANG_DE
   printf("Geben Sie ein Verzeichnis an!");
  #endif
  exit(1);
 }       
 useVMCache=0;
 initLFNLib();
 drv=drive_of_path(argv[dpath]); 
 if (makeDPB(drv,Disk)==1) exit(1); //emergency stop
 if (Disk.CD==1)
 {
  #ifdef LANG_EN
   printf("CD-ROM not supported!\n");
  #endif
  #ifdef LANG_DE
   printf("CD-ROM nicht unterstÅtzt!\n");
  #endif
  exit(1);
 }
 lockDrive(Disk.drive);
                 
 //argument is split into path\name
 if (argv[dpath]!=NULL)
 {
  slash=strlast(argv[dpath],'\\');
  path[0]=0;
  name[0]=0;
  if (slash==0xffff)
  {
   if (argv[dpath][1]==':')
   {
    strncpy(path,argv[dpath],2); 
    path[2]=0;
    strcpy(name,&argv[dpath][2]);
   } else {
    strcpy(name,argv[dpath]);
   }
  } else {
   strncpy(path,argv[dpath],slash+1);
   path[slash+1]=0;
   strcpy(name,&argv[dpath][slash+1]);
  } 
 }
 else 
 {
  #ifdef LANG_EN
   printf("Too few parameters!");
  #endif
  #ifdef LANG_DE
   printf("Geben Sie ein Verzeichnis an!");
  #endif
  exit(1);
 }           
 //set f to the directory to remove
 temp=seekPath(&Disk,argv[dpath],x,f,1);
 if (temp==NULL)
 {
  #ifdef LANG_EN
   printf("Directory does not exist!\n");
  #endif
  #ifdef LANG_DE
   printf("Verzeichnis existiert nicht!\n");
  #endif
  exit(1);
 }
 free(temp);       
 //set dir to the parent directory of f
 temp=seekPath(&Disk,path,x,dir,1);
 strcpy(path,temp);
 free(temp);        
 
 if (!isDirEmpty(&Disk, &f)) 
 {
  #ifdef LANG_EN
   printf("Directory not empty. Delete all files first.\n");
  #endif
  #ifdef LANG_DE
   printf("Das Verzeichnis ist nicht leer. Lîschen Sie erst alle Dateien.\n");
  #endif
  exit(1);
 }          
 
 rmDir(&Disk, &f, &dir);

 printf("Ok.\n");
 unlockDrive(Disk.drive);
}