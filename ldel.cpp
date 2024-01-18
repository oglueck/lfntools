//Delete

#include "lfn.cpp"

int all,wipe,ask,recurse,del_count;

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's Del for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: LDEL [drive:][path]filename\n\n");
  printf("Delete one or more files.\n"); 
  printf("Switches:\n");                 
  printf("/s\tRecurse subdirectories\n");
  printf("/p\tAsk before each file / directory\n");
  printf("/a\tDelete also read-only files / directories.\n");
  printf("/f\tDelete all files / directories.\n");
  printf("/c\tDisable cache\n");     
 #endif
 #ifdef LANG_DE
  printf("Odi's Del fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("LDEL [Laufwerk:][Pfad]Dateiname\n\n");
  printf("Lîscht Dateien.\n"); 
  printf("Optionen:\n");                 
  printf("/s\tUnterverzeichnisse einbeziehen\n");
  printf("/p\tVor jeder Datei / Verzeichnis fragen\n");
  printf("/a\tAuch geschÅtzte (read-only) Dateien / Verzeichnisse lîschen.\n");
  printf("/f\tAlle Dateien / Verzeichnisse lîschen.\n");
  printf("/c\tCache abschalten\n");
 #endif
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
              exit(1);
              break;
    case 'A': all=1;break; 
    case 'F': all=1;wipe=1;break;
    case 'P': ask=1;break; 
    case 'S': recurse=1;break;
    case 'C': bypassCache=1;
              break;      
    case 'V': useVMCache=0;
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


void del(DPB *Disk, longdirentry *dir, char* filen) {
 struct longdirentry *f = (longdirentry*) malloc(sizeof(longdirentry));
 struct dirpointer *dp = (dirpointer*) malloc(sizeof(dirpointer));
 struct dirpointer *dp2 = (dirpointer*) malloc(sizeof(dirpointer));
 dp->cluster=dir->start_cluster; //setup search
 dp->relsector=0;
 dp->entry_no=0; 
 
 while (1)
 {
  *f=FindMask(Disk,filen,dp,0);              

  if (f->name[0]==0) {
   free(f);              
   free(dp2);
   free(dp);
   break; //no more matches
  }
  *dp2 = *dp;
  dp2->entry_no--;
  
  if ((f->atrib & F_DIR)!=0) continue;
  if (!wipe && ((f->atrib & (F_LABEL|F_HIDDEN|F_SYSTEM))!=0)) continue;
  if (!all && ((f->atrib & F_READ_ONLY)!=0)) continue;
  
  char c='y';
  if (ask)
  {
   #ifdef LANG_EN
    printf("Delete '%s' (y/n)?",f->name);
    scanf("%c",&c);
    if (c=='Y') c='y';
   #endif
   #ifdef LANG_DE
    printf("'%s' lîschen (j/n)?",f->name);
    scanf("%c",&c);
    if ((c=='J')  || (c=='j')) c='y';
   #endif
  }
  if (c=='y')                 
  {                                     
   if (f->start_cluster!=0) freeClusterChain(Disk,f);
   deleteDirentry(Disk,dir,dp2,f);    
   del_count++;
   #ifdef LANG_EN
    printf("%s deleted\n",f->name);
   #endif
   #ifdef LANG_DE
    printf("%s gelîscht\n",f->name);
   #endif
  }
 } //endless loop
}


//we need to be a bit conscious about stack here!
void recursiveDel(DPB *Disk, longdirentry *startdir, char *filen) {
 struct longdirentry *dir = (longdirentry*) malloc(sizeof(longdirentry));
 struct dirpointer *dp = (dirpointer*) malloc(sizeof(dirpointer));
 del(Disk, startdir, filen);     

 dp->cluster=startdir->start_cluster; //setup search
 dp->relsector=0;
 dp->entry_no=0; 
 
 while (1) {
  *dir=FindMask(Disk,NULL,dp,0);
  if (dir->name[0]==0) {
   free(dir);   
   free(dp);
   break; //end
  }
  if (((dir->atrib & F_DIR)!=0)) {
   if (!wipe && ((dir->atrib & (F_HIDDEN|F_SYSTEM))!=0)) continue;
   if ((strcmp(dir->name,".")==0) || (strcmp(dir->name,"..")==0)) continue;
   if (all || ((dir->atrib & (F_READ_ONLY))==0)) {
    char c='y';
    if (ask) {
     #ifdef LANG_EN
      printf("Process directory '%s' (y/n)?",dir->name);
      scanf("%c",&c);
      if (c=='Y') c='y';
     #endif
     #ifdef LANG_DE
      printf("'%s' durchsuchen (j/n)?",dir->name);
      scanf("%c",&c);
      if ((c=='J')  || (c=='j')) c='y';
     #endif
    }
    if (c=='y') {   
    
     recursiveDel(Disk, dir, filen);
     
     if (isDirEmpty(Disk, dir)) {
      char c='y';
      if (ask)
      {
       #ifdef LANG_EN
        printf("Directory '%s' is empty. Delete (y/n)?",dir->name);
        scanf("%c",&c);
        if (c=='Y') c='y';
       #endif
       #ifdef LANG_DE
        printf("Verzeichnis '%s' ist leer. Lîschen (j/n)?",dir->name);
        scanf("%c",&c);
        if ((c=='J')  || (c=='j')) c='y';
       #endif
      }
      if (c=='y')                 
      {                                     
       rmDir(Disk, dir, startdir);                                   
       del_count++;
       #ifdef LANG_EN
        printf("%s\\ deleted\n",dir->name);
       #endif
       #ifdef LANG_DE
        printf("%s\\ gelîscht\n",dir->name);
       #endif
      }
     }                                                               
    }   
   }
  } 
 }//while
}       


void main(int argc, char *argv[], char **envp)
{
 struct DPB Disk;        
 struct longdirentry dir;
 char drv,*path,*filen,x[MAX_PATH_SIZE];
 int i,no;
 all=0;
 ask=0; 
 recurse=0;
         
 if (argc==1)
 {
  #ifdef LANG_EN
    printf("delete which file?\n/? for help\n");
  #endif
  #ifdef LANG_DE
    printf("Sie mÅssen eine Datei angeben.\n/? fÅr Hilfe\n");
  #endif
  exit(1);         
 }
 no=argc;
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else no=i;
 }
 
 initLFNLib();
 if (argv[no]==NULL)
 {
  #ifdef LANG_EN
    printf("delete which file?\n/? for help\n");
  #endif
  #ifdef LANG_DE
      printf("Sie mÅssen eine Datei angeben.\n/? fÅr Hilfe\n");
  #endif
  exit(1);         
 }
 drv=drive_of_path(argv[no]);
 if (makeDPB(drv,Disk)==1) exit(1);      
 if (Disk.CD==1)
 {
  #ifdef LANG_EN
    printf("CD-ROM not writable!\n");
  #endif
  #ifdef LANG_DE
    printf("CD-ROM nicht beschreibbar!\n");
  #endif
  exit(1);
 }                  
 
 //split argument into spath\filen 
 int n=0;
 char *spath=argv[no];
 filen=strrchr(argv[no],'\\');  // a:\filename
 if (filen==NULL) {
  spath=NULL;
  filen=strrchr(argv[no],':'); // a:filename
  if (filen==NULL) {
   filen=argv[no];
  } else {
   filen++;
  }
 } else {                  
  n=strlast(argv[no],'\\')+1;
  spath=(char*) malloc(n+1);   //tiny memory leak
  strncpy(spath, argv[no],n);
  spath[n]=0;
  filen++;
 }                

 if ((filen==NULL) || (strlen(filen)==0)) {
  #ifdef LANG_EN
    printf("You must specify a file!\n");
  #endif
  #ifdef LANG_DE                
    printf("Sie mÅssen eine Datei angeben!\n");
  #endif
  exit(1);
 }
 
 //set dir to the directory          
 //path is the short path to the dir
 path=seekPath(&Disk,spath,x,dir,0); 
 if (path==NULL)
 { 
  #ifdef LANG_EN
    printf("Path not found!\n");
  #endif
  #ifdef LANG_DE                
    printf("Pfad nicht gefunden!\n");
  #endif
  exit(1);
 }

 if (strcmp(filen,"*")==0)
 {       
  char c;
  #ifdef LANG_EN
   printf("Delete all files (y/n)?");
   scanf("%c",&c);
   if ((c!='Y') && (c!='y')) exit(1);
  #endif
  #ifdef LANG_DE
   printf("Alle Dateien lîschen (j/n)?");
   scanf("%c",&c);
   if ((c!='J') && (c!='j')) exit(1);
  #endif
 }  

 lockDrive(Disk.drive);                
 del_count=0;
 if (recurse==1) {
  recursiveDel(&Disk, &dir, filen);
 } else {
  del(&Disk, &dir, filen);
 }
 if (del_count==0)
 {
  #ifdef LANG_EN
   printf("File not found!\n");
  #endif
  #ifdef LANG_DE                
   printf("Datei nicht gefunden!\n");
  #endif
 } else {
  #ifdef LANG_EN
   printf("%d Files deleted!\n", del_count);
  #endif
  #ifdef LANG_DE                
   printf("%d Dateien gelîscht!\n", del_count);
  #endif
 } 
 flushCache();
 unlockDrive(Disk.drive);
 free(path);
}
