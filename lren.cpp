// lren


#include "lfn.cpp"

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's Ren for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme.txt file for details.\n\n");
  printf("Usage: lren [drive:][path]filename newname [/?]\n\n");
  printf("Rename a file or directory. Files can not be moved.\n");
  printf("There are no switches.\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Ren fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("LREN [Laufwerk:][Pfad]Dateiname Neuername [/?]\n\n");
  printf("Umbenennen einer Datei. Dateien kînnen nicht verschoben werden.\n");
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
 int i,oldpath,newname,slash;
 char *temp=NULL;
 struct longdirentry f,fsave,g,d,h;
 struct dirpointer dp;
 char mask[MAX_PATH_SIZE],path[MAX_PATH_SIZE],x[MAX_PATH_SIZE];

 oldpath=newname=0;
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else 
   if (oldpath==0) oldpath=i;    //last parameter other than switch
   else newname=i;
 }

 if (newname==0)
 {
  #ifdef LANG_EN
   printf("Too few parameters!"); 
  #endif
  #ifdef LANG_DE
   printf("Zu wenig Parameter!"); 
  #endif
  exit(1);
 }       
 useVMCache=0;
 initLFNLib();
 drv=drive_of_path(argv[oldpath]); 
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
 
 if (strpbrk(argv[oldpath],"*?")!=NULL)
 {        
  #ifdef LANG_EN
   printf("Sorry, no wildcards (?,*) allowed\n"); 
  #endif
  #ifdef LANG_DE
   printf("Sorry, keine Jokerzeichen (?,*) erlaubt\n"); 
  #endif
  exit(1);
 }
 
 lockDrive(Disk.drive);
 
 if (argv[oldpath]!=NULL)
 {
  temp=seekPath(&Disk,argv[oldpath],x,f,1);
  if (temp==NULL)
  {        
   #ifdef LANG_EN
    printf("File not found\n"); 
   #endif
   #ifdef LANG_DE
    printf("Datei nicht gefunden\n");
   #endif
   return;
  }
  else //get its directory
  {
   slash=strlast(argv[oldpath],'\\');
   mask[0]=0;
   path[0]=0;
   if (slash==0xffff)
   {
    strcpy(mask,argv[oldpath]);
    slash=0;   
   }
   else
   {
    strcpy(mask,&argv[oldpath][slash+1]);
    strncpy(path,argv[oldpath],slash+1);
    path[slash+1]=0;
   }
   free(temp);
   temp=seekPath(&Disk,path,x,d,1);
  }
 }
 else 
 {
  temp=seekPath(&Disk,"",x,d,1);
  if (temp==NULL)
  {
   #ifdef LANG_EN
    printf("Could not get current path!?! Strange...\n");
   #endif
   #ifdef LANG_DE
    printf("Fehler beim holen des aktuellen Pfades! Seltsam...\n");
   #endif
   return;
  }
 }
 free(temp);
 
 if (strpbrk(argv[newname],"/\\:*?\"<>|")!=NULL)  //forbidden characters
 {
  #ifdef LANG_EN
   printf("A filename must not contain any of the following characters /\\:*?\"<>|\n"); 
  #endif
  #ifdef LANG_DE
   printf("Ein Dateiname darf keines der folgenden Zeichen enthalten /\\:*?\"<>|\n"); 
  #endif
  unlockDrive(Disk.drive);
  exit(1);
 }
 //d=directory, f=oldfile, g=newfile
 dp.cluster=d.start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 g=FindMask(&Disk,argv[newname],&dp,1); //new name already exists?
 
 dp.cluster=d.start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 h=FindMask(&Disk,f.name,&dp,1); //find Direntry
 
 if ((g.name[0]!=0) && (h.start_cluster!=g.start_cluster))
 {
  #ifdef LANG_EN
   printf("Filename already exists!\n"); 
  #endif
  #ifdef LANG_DE
   printf("Es gibt schon eine Datei mit diesem Namen!\n");
  #endif
  exit(1);
 }
  
 dp.entry_no--;
 deleteDirentry(&Disk,&d,&dp,&f);
 
 fsave=f;  //back it up
 f.name[0]=0;
 strcpy(f.name,argv[newname]); //leave dosname as is
 ascii2uni((unsigned char*)f.name, (unsigned int*)f.uniname);
 f.dosname[0]=0;
 int result=insertDirentry(&Disk,&d,&f,PROTECT_ALL);
 if (result!=0) { 
  #ifdef LANG_DE
   printf("Datei existiert bereits. Abbruch.\n");
  #endif
  #ifdef LANG_EN
   printf("File already exists. Abort.\n");
  #endif
  f=fsave;
  result=insertDirentry(&Disk,&d,&f,PROTECT_ALL);  //rollback
  if (result!=0) {
   #ifdef LANG_DE
    printf("Die Quelldatei konnte nicht wiederhergestellt werden! Sorry.\n");
   #endif
   #ifdef LANG_EN
    printf("The original file could not be restored! Sorry.\n");
   #endif
   printf("Startcluster: %lu; L‰nge: %lu\n",f.start_cluster,f.length);
  }
 }
 else {
  printf("Ok\n");
 }
 
 unlockDrive(Disk.drive);
}