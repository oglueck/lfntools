// lmd

#include "lfn.cpp"

void PrintHelp(void)
{

 #ifdef LANG_EN
  printf("This is Odi's Md for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: md [drive:][path\\]newdir [/?]\n\n");
  printf("Create a new directory.\n");
  printf("There are no switches.\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Md fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("MD [Laufwerk:][Pfad\\]Verzeichnisname [/?]\n\n");
  printf("Neues Verzeichnis erstellen.\n");
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
 struct longdirentry dir, f;
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
   printf("Must specify directory name!");
  #endif
  #ifdef LANG_DE                 
   printf("Geben Sie den Namen des Verzeichnisses an!"); 
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
   }
   else
    strcpy(name,argv[dpath]);
  }
  else 
  {
   strncpy(path,argv[dpath],slash+1);
   path[slash+1]=0;
   strcpy(name,&argv[dpath][slash+1]);
  } 
 }
 else 
 {
  #ifdef LANG_EN
   printf("Must specify directory name!"); 
  #endif
  #ifdef LANG_DE
   printf("Geben Sie den Namen des Verzeichnisses an!"); 
  #endif
  exit(1);
 }
 temp=seekPath(&Disk,argv[dpath],x,dir,1);
 if (temp!=NULL)
 {
  #ifdef LANG_EN
   printf("Directory already exists!\n");
  #endif
  #ifdef LANG_DE
   printf("Verzeichnis existiert bereits!\n"); 
  #endif
  free(temp);
  exit(1);
 }
 temp=seekPath(&Disk,path,x,dir,1);
 if ((dir.name[0]==0) && (Disk.fat_type==FAT32)) //root
 {
  dir.start_cluster=0;
 }
 strcpy(path,temp);
 free(temp); 
 
 int result = makeDirectory(&Disk, &dir, name, &f);
 switch (result) {
  case 0: break;
  
  case 1:
   #ifdef LANG_EN
    printf("Could not allocate space for new directory. Disk full.\n");
   #endif
   #ifdef LANG_DE
    printf("Konnte Verzeichnis nicht erstellen. Laufwerk voll.\n");  
   #endif     
   exit(1);
   break;
  
  case 2:
   #ifdef LANG_EN
    printf("Could not complete! 1 Cluster lost. Use Scandisk.\n"); 
   #endif
   #ifdef LANG_DE
    printf("Es ist ein Fehler aufgetreten! 1 Cluster verloren. Scandisk aufrufen!\n"); 
   #endif
   exit(1);
   break;
  
  case 3:
   #ifdef LANG_EN
    printf("Could not initialize new directory! New directory corrupt!\n");
   #endif
   #ifdef LANG_DE
    printf("Konnte neues Verzeichnis nicht initialisieren! Neues Verzeichnis unbrauchbar!\n");
   #endif  
   exit(1);
   break;
  
 }
 printf("Ok.\n");
 unlockDrive(Disk.drive);
}