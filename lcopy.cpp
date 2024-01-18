#include <conio.h> 
#include <stdio.h>
#include "lfn.cpp"

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's Copy for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: lcopy [drive:][path]sourcefile [drive:][path][destinationfile][/?]\n\n");
  printf("Copy selected files.\n");
  printf("Switches:\n");
  printf("/a\tinclude hidden files\n");
  printf("/s\tRecurse subdirectories\n");
  printf(" /d\tDo not create subdirectories when recursing (/s)\n");
  printf(" /e\tDo not copy empty directories\n");
  printf("/y\tDisable confirmation if target already exists\n");
  printf("/r\tOverwrite read-only files\n");
  printf("/k\tWhen copying from CD, keep ISO-9660 / Joliet short names.\n");
  printf("/c\tDisable cache\n"); 
  #ifdef VMEMORY    
   printf("/v\tDisable use of EMS and XMS\n");
  #endif  
  printf("/b\tDo not cancel on key stroke\n");
  printf("/i\tForce ISO-9660 file system\n");
  printf("/tn\tUse CD data track n\n");
  printf("\n\nUse single asterix (*) to select all files.\nTo copy only directory structure use LCOPY . <dest> /S\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Copy fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe liesmich Datei fÅr Details.\n\n");
  printf("LCOPY [Laufwerk:][Pfad]Quelldatei [Laufwerk:][Pfad][Zieldatei][/?]\n\n");
  printf("Kopiert Dateien.\n");
  printf("Optionen:\n");
  printf("/a\tauch versteckte Dateien kopieren\n");
  printf("/s\tUnterverzeichnisse einbeziehen\n");
  printf(" /d\tKeine Unterverzeichnisse neu erstellen (/s)\n");
  printf(" /e\tKeine leeren Verzeichnisse kopieren\n");
  printf("/y\tKein Nachfragen wenn Zieldatei schon existiert\n");
  printf("/r\töberschreibt schreibgeschÅtzte Dateien\n");
  printf("/k\tBeim Kopieren von CD kurze Namen von ISO-9660 / Joliet behalten\n");
  printf("/c\tCache abschalten\n");  
  #ifdef VMEMORY    
   printf("/v\tKein XMS und EMS verwenden\n");
  #endif
  printf("/b\tAbbrechen durch Tastendruck verhindern\n");
  printf("/i\tauf CD-ROMs immer ISO-9660 Dateisystem benÅtzen\n");
  printf("/tn\tBenÅtze CD-Daten Track (Session) Nummer n\n");
  printf("\n\nBenÅtzen Sie einen einzelnen Stern (*) um alle Dateien zu kopieren.\nUm nur die Verzeichnisstruktur zu replizieren, benÅtzen Sie LCOPY . <dest> /S\n");
 #endif

 exit(0);
}

int switch_hid,switch_sub,switch_cancel,switch_nomd,switch_noconfirm,switch_ro,switch_keepcdalias,switch_noemptydir;
dword countall,dst_start_cluster;

void md(struct DPB *Disk,struct longdirentry *dir,struct longdirentry *p);

int CopyFiles(struct DPB *Source,struct longdirentry *sdir,char *smask,
              struct DPB *Dest,struct longdirentry *ddir, char *dmask)
//return 1 if cancelled            
{ 
 #define BUF_SIZE 32768 //32Kb
 #define MAX_BUFS 20
 struct dirpointer dp;
 struct longdirentry sf,df;
 struct FATfile dfat,sfat;
 void *DATA[MAX_BUFS];          
 word no_bufs;
 size_t size,sbpc,dbpc;
 dword count; //# kopierte Dateien
 dword dclusters,sclusters,sccount; //# cluster der Quell/Zieldateien
 word sc,dc,i,j; //# cluster die in DATA platz haben
 int done,doneall,key;
 int protect_ro;
 if ((switch_ro!=0) && (switch_noconfirm!=0)) {
  protect_ro=0;
 }
 
 //Bytes pro Cluster? 2,4,8,16,32,64K
 sbpc=Source->sectors_per_cluster*Source->bytes_per_sector; 
 dbpc=Dest->sectors_per_cluster*Dest->bytes_per_sector;           
 size=BUF_SIZE;
 DATA[0]=malloc(size);
 if (DATA[0]==NULL)
 {                   
  //Reserviere Maximum 
  if (sbpc>dbpc) size=sbpc;
  else size=dbpc;   
  DATA[0]=malloc(size);
  if (DATA[0]==NULL)
  {
   #ifdef LANG_EN
    printf("Could not allocate memory for one cluster!\n");   
   #endif
   #ifdef LANG_DE
    printf("Nicht genug Speicher fÅr einen einzigen Cluster!\n");   
   #endif
   exit(1);
  }              
  #ifdef LANG_EN
   printf("Low memory. Using smaller buffer of %u bytes.\n",size);
  #endif
  #ifdef LANG_DE
   printf("Wenig freier Speicher. BenÅtze kleinere Puffer von %u Bytes.\n",size);  
  #endif
 }        
 no_bufs=1;
 for (i=1;i<MAX_BUFS;i++)
 {
  DATA[i]=malloc(size);
  if (DATA[i]==NULL) break;
  no_bufs=i;  
 }           
 no_bufs++;
 sc=size/sbpc;
 dc=size/dbpc;
 
 dp.cluster=sdir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 dp.countdir=COUNTDIR_START;
 dp.countfile=COUNTFILE_START; 
 dp.length=sdir->length;  
 sf=FindMask(Source,smask,&dp,0);
 count=0;
 while (sf.name[0]!=0)
 {
  if ((switch_cancel==1) && (_kbhit()!=0))      //Anyone wants to cancel the job?
  {
   #ifdef LANG_EN
    printf("Cancel (y/n)?");
   #endif
   #ifdef LANG_DE
    printf("Abbrechen (j/n)?");
   #endif

   do     
   {  
    key=_getche();
    key=toupper(key);
   }
   #ifdef LANG_EN
    while ((key!='N') && (key!='Y'));
   #endif
   #ifdef LANG_DE
    while ((key!='N') && (key!='J'));
   #endif        
   
   #ifdef LANG_EN   
    if (key=='Y')
   #endif
   #ifdef LANG_DE
    if (key=='J')
   #endif
   {         
    for (i=0;i<no_bufs;i++) free(DATA[i]);
    return 1;
   }
  }
  if ((sf.atrib & (F_DIR|F_LABEL))==0) //kopiere keine Dirs und Labels
  {
   if (((sf.atrib & F_HIDDEN)==0) || (switch_hid==1)) //versteckte normalerweise nicht kopieren
   {
    printf("%s\n",sf.name);
    df=sf; //copy direntry 
    if ((Source->CD==1) && (switch_keepcdalias==0)) {
     df.dosname[0]=0;
    }
    
    //modify direntry according to new fs geometry
    sfat.cluster=sf.start_cluster;
    if ((sfat.cluster!=0) && (sf.length!=0)) //file not empty
    {
     sfat.sector=firstSectorOfCluster(Source,sf.start_cluster);
     dfat.cluster=getNextAvailClusterNo(Dest); //create file
     if (dfat.cluster==0)
     {
      printf("Disk full.\n");
      for (i=0;i<no_bufs;i++) free(DATA[i]);
      killCache();
      unlockDrive(Dest->drive);
      exit(1);
     }
     dfat.sector=firstSectorOfCluster(Dest,dfat.cluster);
     df.start_cluster=dfat.cluster; //start cluster merken
     sclusters=sf.length/sbpc;
     if (sf.length % sbpc!=0) sclusters++; //ceil
     dclusters=df.length/dbpc;
     if (df.length % dbpc!=0) dclusters++; //floor
     
     if (extendFile(Dest,&dfat,dclusters,W_FILE)!=dclusters)   //allocate space
     {
      printf("Disk full. %s not copied.\n",sf.name);
      freeClusterChain(Dest,&df); 
      for (i=0;i<no_bufs;i++) free(DATA[i]);
      killCache();    
      unlockDrive(Dest->drive);
      exit(1);
     } 
     dfat.cluster=df.start_cluster;
     dfat.sector=firstSectorOfCluster(Dest,dfat.cluster);
    }    
    
    int dontcopy=0;
    //make direntry first
    if ((sfat.cluster==0) || (sf.length==0))  df.start_cluster=0;    
    df.atrib|=F_ARC; //set archive bit
    df.checksum=0; //neue checksum berechnen
    if (dmask[0]!=0) {
     strcpy(df.name,dmask);
     df.uniname[0]=0;
    }                
    if (df.uniname[0]==0) {
     ascii2uni((unsigned char*)df.name, (unsigned int*)df.uniname);
    }
    
        
    int protect_ro=PROTECT_ALL;  //be pessimistic
    if (switch_noconfirm!=0) protect_ro=PROTECT_RO; //we do not warn on normal files
    if ((switch_ro!=0) && (switch_noconfirm!=0)) {
     protect_ro=PROTECT_NO; //we dont need any protection if the user tells us
    }
    int result=insertDirentry(Dest,ddir,&df,protect_ro); //try to insert the file
    if (result!=0)  
    {           
     if (protect_ro==PROTECT_NO) { //file was not protected
      #ifdef LANG_EN
       printf("Target file is not writable. File not copied.\n");
      #endif
      #ifdef LANG_DE
       printf("Zieldatei nicht beschreibbar. Nicht kopiert.\n");
      #endif
      dontcopy=1;
     }
     else {
      if (switch_noconfirm!=0) {
       //dont confirm                                                               
       if (result==4) {  //normal file
         result=insertDirentry(Dest,ddir,&df,PROTECT_RO); //try to insert the file
       }
       else {
        result=0;
        dontcopy=1;
       }
      }
      else {
       //confirm
       #ifdef LANG_DE 
        switch (result) {
         case 4: printf("Datei existiert bereits."); break;
         case 3: printf("Datei ist schreibgesch¸tzt."); break;
         case 2: printf("Datei ist eine Systemdatei."); break;
         case 1: printf("Zieldatei nicht beschreibbar."); break;
        }
       #endif
       #ifdef LANG_EN
        switch (result) {
         case 4: printf("File already exists."); break;
         case 3: printf("File is read-only."); break;
         case 2: printf("File is a system file."); break;
         case 1: printf("Target not writable."); break;
        }
       #endif
       if (((result>1)  && (switch_ro!=0)) ||
           ((result==4) && (switch_ro==0))) {  //do we let the user choose at all if to overwrite?
        #ifdef LANG_DE 
         printf(" öberschreiben? [j/n] ");
        #endif
        #ifdef LANG_EN
         printf(" overwrite? [y/n] ");
        #endif
        key='\0';
        do     
        {        
         key=_getche();
         key=toupper(key);
        }
        #ifdef LANG_EN
         while ((key!='N') && (key!='Y'));
        #endif
        #ifdef LANG_DE
         while ((key!='N') && (key!='J'));
        #endif        
        printf("\n");
        #ifdef LANG_EN   
         if (key=='Y')
        #endif
        #ifdef LANG_DE
         if (key=='J')
        #endif
        {
         result=insertDirentry(Dest,ddir,&df,PROTECT_NO); //force insert file
        }
        else {
         result=0;
         dontcopy=1;
        }
                 
       }//if do confirm
       else { 
        #ifdef LANG_DE
         printf(" Nicht kopiert.\n");  //user had no choice
        #endif
        #ifdef LANG_EN
         printf(" Not copied.\n");  //user had no choice
        #endif
        result=0;
        dontcopy=1;
       }
      }//if no confirmation
     }//if protected
     if (result!=0) {
      #ifdef LANG_EN
       printf("Target file is not writable (%u). File not copied.\n", result);
      #endif
      #ifdef LANG_DE
       printf("Zieldatei nicht beschreibbar (%u). Nicht kopiert.\n", result);
      #endif
      dontcopy=1;    
     }
     else {
      if (dontcopy==0) count++;
     }
    } //if failed to insert
    else {
     if (dontcopy==0) count++;
    }           
    
    if (dontcopy==1) {
     freeClusterChain(Dest, &df); //reclaim allocated space
    }
    
    //copy data now
    if ((sfat.cluster!=0) && (sf.length!=0) && (dontcopy==0)) //file not empty
    {
     
     sccount=0;
     doneall=0;
     do
     { 
       done=0;
       for (j=0;(j<no_bufs) && (doneall!=1);j++)
       {
        for (i=0;(i<sc) && (doneall!=1);i++)
        {
         if (Source->CD==1)
          readCDSectorsP(Source,sfat.sector,1,((void*)&((char*)DATA[j])[i*sbpc]));
         else
          readSectorP(Source->drive,sfat.sector,Source->sectors_per_cluster,((void*)&((char*)DATA[j])[i*sbpc])); //ganzen cluster lesen ohne cache
         sccount++;
         switch (Source->CD)
         {
          case 0:
           doneall=getNextCluster(Source,&sfat);
          break;
          
          case 1:
           sfat.cluster++;
           sfat.sector++;
           if (sccount==sclusters) doneall=1;
          break;
         }
        }
       }  
       
       done=0;
       for (j=0;(j<no_bufs) && (done!=1);j++) 
       {
        for (i=0;(i<dc) && (done!=1);i++)
        {
         writeSectorP(Dest->drive,dfat.sector,Dest->sectors_per_cluster,((void*)&((char*)DATA[j])[i*dbpc]),W_FILE); //ganzen cluster schreiben ohne cache
         done=getNextCluster(Dest,&dfat);
        }
       }         
       
     } while (doneall!=1); 
    } //if non-empty file
    
   }
  }
  sf=FindMask(Source,smask,&dp,0);
 }
 for (i=0;i<no_bufs;i++) free(DATA[i]); 
 if (switch_sub==0) printf("%lu file(s) copied\n",count);
 else countall+=count;
 return 0;
}             

void md(struct DPB *Disk,struct longdirentry *dir,struct longdirentry *p)  
//make dir p in dir
{
 struct longdirentry f;
 struct dirpointer dp;
 
 dp.cluster=dir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0; 
 dp.countdir=COUNTDIR_START;
 dp.countfile=COUNTFILE_START;
 dp.length=dir->length;   
 f=FindMask(Disk,p->name,&dp,1);
 if (f.name[0]!=0) {
  *p = f;
  return;  //return existing dir          
 }

 //allocate a new dir   
 int result = makeDirectory(Disk, dir, p);

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
}

int Recurse(struct DPB *Source,struct longdirentry *sdir,char *smask,
              struct DPB *Dest,struct longdirentry *ddir, char *dmask)
//return 1 if cancelled              
{
 struct dirpointer dp;  
 struct longdirentry *p, *q;
 int cancel=0;
 
 if (CopyFiles(Source,sdir,smask,Dest,ddir,dmask)==1) return 1;
 p=new longdirentry; 
 q=new longdirentry; 
 if ((p==NULL) || (q==NULL))
 {
  printf("Out of memory!Skip...\n");
  return 1;
 } 
 dp.cluster=sdir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 dp.countdir=COUNTDIR_START;
 dp.countfile=COUNTFILE_START;
 dp.length=sdir->length;   
 do
 {
  do
  {
   *p=FindMask(Source,NULL,&dp,0);
  } while ((((p->atrib & F_DIR)!=F_DIR) || (p->dosname[0]=='.')) && (p->name[0]!=0));
  if (p->name[0]!=0)  {                
   if ((Source->drive!=Dest->drive) || (p->start_cluster!=dst_start_cluster)) //skip destination dir
   {
    if (!(((p->atrib & F_HIDDEN)==F_HIDDEN) && (switch_hid==0))) {                 
     if (switch_noemptydir && isDirEmpty(Source,p)) {
      #ifdef LANG_EN
       printf("Skipped empty directory: %s\\\n", p->name);        
      #endif
      #ifdef LANG_DE                                
       printf("Leeres Verzeichnis Åbersprungen: %s\\\n", p->name);
      #endif
     } else {
      printf("%s\\\n",p->name);
      if (switch_nomd==1) {
       cancel=Recurse(Source,p,smask,Dest,ddir,dmask);
      } else {                   
       *q = *p;
       md(Dest,ddir,q);
       cancel=Recurse(Source,p,smask,Dest,q,dmask);
      }//if no md
     } //empty
    }// if not hidden
   } //if not skip
   else {
    #ifdef LANG_EN
     printf("Skipped destination directory.\n"); 
    #endif
    #ifdef LANG_DE                                
     printf("Zielverzeichnis Åbersprungen.\n");
    #endif
   }
  }
 } while ((p->name[0]!=0) && (cancel!=1));
 delete p;
 delete q;
 return cancel;
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
    case 'A': switch_hid=1;
              break;              
    case 'S': switch_sub=1;
              break;           
    case 'D': switch_nomd=1;
              break;        
    case 'E': switch_noemptydir = 1;
              break;  
    case 'Y': switch_noconfirm=1;
              break;
    case 'R': switch_ro=1;
              break;
    case 'C': bypassCache=1;
              printf("Cache disabled.\n");
              break;
    case 'V': useVMCache=0;
              printf("Virtual cache disabled.\n");
              break;
    case 'B': switch_cancel=0;
              break;           
    case 'K': switch_keepcdalias=1;
              break;
    case 'I': forceIso=1;
              break;
    case 'T': i++;
              forceTrack=atoi(&arg[i]);
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

void main(int argc, char *argv[], char *envp)
{
 struct DPB *Source,*Dest;      
 int sno,dno,i,drv,slash;
 char *spath,*dpath;
 char path[MAX_PATH_SIZE],smask[MAX_LFN_SIZE],dmask[MAX_LFN_SIZE],x[MAX_LFN_SIZE];
 struct longdirentry sdir,ddir;
 
 Source=new DPB;
 Dest=new DPB;
 if ((Dest==NULL) || (Source==NULL))
 {
  #ifdef LANG_EN
   printf("Out of memory!\n");
  #endif
  #ifdef LANG_DE              
   printf("Zuwenig Speicher!\n"); 
  #endif
  exit(1);
 }
                                                                      
 sno=dno=0;         
 switch_hid=0; 
 switch_sub=0;    
 switch_noemptydir=0;
 switch_cancel=1;
 switch_nomd=0;
 switch_noconfirm=0;
 switch_ro=0;  
 switch_keepcdalias=0;
 
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else 
   if (sno==0) sno=i;
   else dno=i;
 }
 
 if (sno==0)
 {
  #ifdef LANG_EN
   printf("No source specified.\n");  
  #endif
  #ifdef LANG_DE
   printf("Sie mÅssen eine Quelldatei angeben.\n");  
  #endif
  exit(1);
 }

 initLFNLib();            
 drv=drive_of_path(argv[sno]); 
 if (makeDPB(drv,*Source)==1) exit(1); 
 
 if (dno==0)
 {
  drv=drive_of_path(NULL); //current drive
 }
 else
 {
  drv=drive_of_path(argv[dno]);
 }
 if (makeDPB(drv,*Dest)==1) exit(1);
 if (Dest->CD==1)
 {
  #ifdef LANG_EN
   printf("CD-ROM not writable!\n"); 
  #endif
  #ifdef LANG_DE
   printf("CD-ROM nicht beschreibbar!\n"); 
  #endif
  exit(1);
 }

 char *ppath=path;
 path[0]=0;
 smask[0]=0;

 slash=strlast(argv[sno],'\\');
 if (slash!=0xffff)
 {
  strncpy(path,argv[sno],slash+1);
  path[slash+1]=0;
  strcpy(smask,&argv[sno][slash+1]);
 }
 else
 {               
  if (argv[sno][1]==':') {
   ppath=NULL;
   strcpy(smask, argv[sno]+2);
  } else {
   path[0]=0;
   strcpy(smask,argv[sno]);
  }
 }
 spath=seekPath(Source,ppath,x,sdir,1);
 if (spath==NULL)
 {
  #ifdef LANG_EN
   printf("Source path not found!\n");  
  #endif
  #ifdef LANG_DE
   printf("Quellpfad nicht gefunden!\n");
  #endif
  exit(1);
 }
 if (smask[0]==0)
 {
  #ifdef LANG_EN
   printf("Must specify a source file!\n"); 
  #endif
  #ifdef LANG_DE
   printf("Sie mÅssen eine Quelldatei angeben!\n"); 
  #endif
  exit(1);
 }

 path[0]=0;
 dmask[0]=0;
 if (dno==0) dpath=seekPath(Dest,NULL,x,ddir,1);   
 else dpath=seekPath(Dest,argv[dno],x,ddir,1);
 if ( (dpath==NULL) ||
      ((dpath!=NULL) && ((ddir.atrib & F_DIR)==0)) )
 {
  if (dpath!=NULL) free(dpath);
  slash=strlast(argv[dno],'\\');
  if (slash!=0xffff)
  {
   strncpy(path,argv[dno],slash+1);
   path[slash+1]=0;
   strcpy(dmask,&argv[dno][slash+1]);
  }
  else
  {
   path[0]=0;
   strcpy(dmask,argv[dno]);
  }
  dpath=seekPath(Dest,path,x,ddir,1);
  if (dpath==NULL)
  { 
   #ifdef LANG_EN
    printf("Destination path not found!\n");
   #endif
   #ifdef LANG_DE                            
    printf("Zielpfad nicht gefunden!\n");
   #endif
   exit(1);
   //TO DO: create path instead of aborting, here.
  }
  if (strpbrk(dmask,"*?")!=NULL)
  {
   #ifdef LANG_EN
    printf("Destination must not have wildcards!\n");  
   #endif
   #ifdef LANG_DE
    printf("Zielpfad darf keine Jokerzeichen enthalten!\n");   
   #endif
   exit(1);
  }
  if ((dmask[0]!=0) && (strpbrk(smask,"*?")!=NULL))
  {
   #ifdef LANG_EN
    printf("Cannot copy multiple files to single file\n"); 
   #endif
   #ifdef LANG_DE
    printf("Kann nicht mehrere Dateien in eine einzige kopieren\n"); 
   #endif
   exit(1);
  }
 }

 lockDrive(Dest->drive);
 if (switch_sub==1)
 {
  countall=0;
  dst_start_cluster=ddir.start_cluster;
  Recurse(Source,&sdir,smask,Dest,&ddir,dmask);
  #ifdef LANG_EN
   printf("%lu file(s) copied\n",countall);
  #endif
  #ifdef LANG_DE
   printf("%lu Datei(en) kopiert\n",countall);
  #endif
 }
 else CopyFiles(Source,&sdir,smask,Dest,&ddir,dmask);
 #ifdef LANG_EN
  printf("Flushing cache...\n");
 #endif
 #ifdef LANG_DE
  printf("Cache schreiben...\n");
 #endif
 killCache();
 unlockDrive(Dest->drive);

 delete Source;
 delete Dest;
 free(spath);
 free(dpath); 
}
