// Longfilename DIR
// stack-checking off

#include <stdio.h> 
#include <stdlib.h> 
#include <conio.h>
#include <string.h>               
#include "lfn.cpp"

void PrintHelp(void)
{ 
 #ifdef LANG_EN
  printf("This is Odi's Dir for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: ldir [drive:][path][filename] [/a][/b][/s][/p][/c][/i][/tn][/?]\n\n");
  printf("Display (selected) files of a directory.\n");
  printf("Note: Dates are printed as day.month.year\n");
  printf("Switches:\n");
  printf("/a\tDisplays also hidden files.\n");
  printf("/b\tUse short output format\n");
  printf("/s\tWork on subdirectories\n");
  printf("/p\tpause on page breaks\n"); 
  printf("/c\tDisable cache\n");
  printf("/i\tForce ISO-9660 file system\n");
  printf("/tn\tUse CD data track n\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Dir fr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glck\nDies ist freie Software unter der GPL. Siehe readme Datei fr Details.\n\n");
  printf("LDIR [Laufwerk:][Pfad][Dateiname] [/a][/b][/s][/p][/c][/i][/tn][/?]\n\n");
  printf("Zeigt (ausgew„hlte) Dateien eines Verzeichnisses.\n");
  printf("Hinweis: Datumsangaben werden als TT.MM.JJJJ ausgegeben.\n");
  printf("Optionen:\n");
  printf("/a\tauch versteckte Dateien anzeigen.\n");
  printf("/b\tkurzes Ausgabeformat\n");
  printf("/s\tUnterverzeichnisse einbeziehen\n"); 
  printf("/p\tseitenweise anzeigen\n");
  printf("/c\tCache abschalten\n");
  printf("/i\tauf CD-ROMs immer ISO-9660 Dateisystem bentzen\n");
  printf("/tn\tBentze CD-Daten Track (Session) Nummer n\n");
 #endif
 exit(0);
}

int switch_all,switch_short,switch_test,switch_subdir,switch_page;
dword totbytes,totfiles,totdirs,lines;
byte locdate;

#pragma optimize("gel",off) //cannot optimize asm
byte getDateFormat() {
 byte a[40];
 __asm {
  mov ax,0x3800
  push ss
  pop ds
  lea dx,a
  int 0x21
 }     
 return a[0];
}
#pragma optimize("",on)

void checkPage()
{
 if ((switch_page!=0) && (lines>24)) printf("!!!! >24 !!!!");
 if ((switch_page!=0) && (lines>=24))
 { 
  #ifdef LANG_EN
   printf("--- Press key to continue ---");
  #endif
  #ifdef LANG_DE
   printf("--- Taste drcken ---");
  #endif
  lines=0;
  _getch();
  printf("\r                                         \r");
 }  

}

void PrintEntry(struct longdirentry *p,dword &files,dword &dirs,dword &bytes)
{             
 int dd,mm,yy;
        
 if ((p->atrib & F_DIR)==F_DIR)
 {
  if (switch_short==0) printf("%.8s.%.3s <DIR>       ",p->dosname,p->dosext);
  dirs++;
 }
 else
 {
  if ((p->atrib & F_LABEL)==F_LABEL)
  {
   if (switch_short==0) printf("%.8s%.3s  <LABEL>     ",p->dosname,p->dosext);
  }
  else    
  {
   if (switch_short==0) printf("%.8s.%.3s %10lu  ",p->dosname,p->dosext,p->length);
   files++;
   bytes+=p->length;
  }
 }
 if (switch_short==0) 
 {
  dd=(p->date & 0x1f);       
  mm=(p->date >> 5) & 0xf;
  yy=((p->date >> 9) & 0x7f)+1980;
  switch (locdate) {
   case 0: printf("%02u.%02u.%4u  ",mm,dd,yy); break;
   case 1: printf("%02u.%02u.%4u  ",dd,mm,yy); break;
   case 2: printf("%4u.%02u.%02u  ",yy,mm,dd); break;
  }
  printf("%02u:%02u  ",(p->time >> 11) & 0x1f,(p->time >> 5) & 0x3f);
 }          
 if (switch_test==1) printf("%lu ",p->start_cluster);
 printf("%s\n",p->name);
 lines++;
 checkPage();
}

void PrintMaskedDir(const struct DPB *b,struct longdirentry *pDir,char *mask,char *path)
{    
 struct longdirentry p; 
 struct dirpointer dp;
 dword files,dirs,bytes;
 int empty=1;
 
 if (pDir==NULL) return;
 dp.cluster=pDir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 dp.countdir=COUNTDIR_START;
 dp.countfile=COUNTFILE_START;
 dp.length=pDir->length;
 p=FindMask(b,mask,&dp,0);
 files=0;dirs=0;bytes=0;
 while (p.name[0]!=0)
 {                 
   if ((switch_all) || ((p.atrib & F_HIDDEN)==0))
   {                                             
    if (empty!=0)
    { 
     if (b->label[0]!=' ') 
     {       
      printf("\n");
      lines++;
      checkPage();
      #ifdef LANG_EN
       printf("Disk in drive %c: is labeled %.11s\n",b->drive+'A',b->label);
      #endif
      #ifdef LANG_DE
       printf("Datentr„ger in Laufwerk %c: mit Namen %.11s\n",b->drive+'A',b->label);
      #endif
      lines++;
      checkPage();
     }
     else      
     {
      printf("\n");
      lines++;
      checkPage();
      #ifdef LANG_EN
       printf("Disk in drive %c has no label\n",b->drive+'A');
      #endif
      #ifdef LANG_DE
       printf("Datentr„ger in Laufwerk %c hat keinen Namen\n",b->drive+'A');
      #endif 
      lines++;
      checkPage();
     }
     #ifdef LANG_EN 
      printf(" Directory of %s\n",path);
     #endif
     #ifdef LANG_DE
      printf(" Verzeichnis %s\n",path);
     #endif  
     lines++;
     checkPage();
     printf("\n");
     lines++;
     checkPage();
     empty=0;
    }
    PrintEntry(&p,files,dirs,bytes); 
   }
  p=FindMask(b,mask,&dp,0);
 }
 if ((empty!=0) && (switch_subdir==0))
  #ifdef LANG_EN
   printf("File not found.\n");
  #endif
  #ifdef LANG_DE
   printf("Datei nicht gefunden.\n");
  #endif
  lines++;
  checkPage();  
  
 if (empty==0)
 {
  #ifdef LANG_EN
   printf("%10lu Files\t%lu Bytes\n",files,bytes);
   lines++;
   checkPage();  
   printf("%10lu Directories\n",dirs);
   lines++;
   checkPage();  
  #endif 
  #ifdef LANG_DE
   printf("%10lu Dateien\t%lu Bytes\n",files,bytes);
   lines++;
   checkPage();  
   printf("%10lu Verzeichnisse\n",dirs);
   lines++;
   checkPage();  
  #endif
 }

 totbytes+=bytes;
 totfiles+=files;
 totdirs+=dirs;
}

void Recurse(const struct DPB *b,struct longdirentry *pDir,char *mask,char *path)
//Print all Subdirectories
{
 struct dirpointer dp;  
 struct longdirentry *p;
 char *path2;
 if ((switch_all) || ((pDir->atrib & F_HIDDEN)==0)) {
  PrintMaskedDir(b,pDir,mask,path);
  p=new longdirentry;
  if (p==NULL)
  { 
   #ifdef LANG_EN
    printf("Out of memory! Skip...\n");
   #endif
   #ifdef LANG_DE
    printf("Zuwenig Speicher! N„chste Datei...\n");
   #endif  
   lines++;
   checkPage();   
   return;
  } 
  path2=(char*)malloc(MAX_PATH_SIZE);
  if (path2==NULL)
  {
   #ifdef LANG_EN
    printf("Out of memory! Skip...\n");
   #endif
   #ifdef LANG_DE
    printf("Zuwenig Speicher! N„chste Datei...\n");
   #endif
   lines++;
   checkPage();
   delete p;
   return;
  }
  path2[0]=0;
  dp.cluster=pDir->start_cluster;
  dp.relsector=0;
  dp.entry_no=0;
  dp.countfile=COUNTFILE_START;
  dp.countdir=COUNTDIR_START;
  dp.length=pDir->length;
  do
  {
   do
   {
    *p=FindMask(b,NULL,&dp,0);
   } while ((((p->atrib & F_DIR)!=F_DIR) || (strcmp(p->name,".")==0) || (strcmp(p->name,"..")==0)) && (p->name[0]!=0));
   if (p->name[0]!=0)
   {
    strcpy(path2,path);
    if (path2[strlen(path2)-1]!='\\') strcat(path2,"\\");
    strcat(path2,p->name);
    strcat(path2,"\\");   
    Recurse(b,p,mask,path2);
   }
  } while (p->name[0]!=0);
  delete p;
  free(path2); 
 }
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
    case 'A': switch_all=1; 
              break;         
    case 'B': switch_short=1;       
              break;
    case 'S': switch_subdir=1;
              break;              
    case 'P': switch_page=1;
              break;          
    case 'X': switch_test=1;       //TEST
              break;                   
    case 'C': bypassCache=1;
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
 struct DPB Disk;      
 struct longdirentry f;
 char drv,path[MAX_PATH_SIZE],mask[MAX_PATH_SIZE],*temp,x[MAX_PATH_SIZE];
 int i,no;
 unsigned int slash,len;
 
 temp=NULL;
 no=argc;
 
 switch_all=0;
 switch_short=0;
 switch_test=0;
 switch_subdir=0;
 switch_page=0; 
 useVMCache=0;
 lines=0;
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else no=i;
 }

 locdate=getDateFormat(); 
 
 initLFNLib();
 drv=drive_of_path(argv[no]); 
 if (makeDPB(drv,Disk)==1) exit(1);  //emergency stop
 path[0]='\0';
 mask[0]='\0';
 x[0]='\0';    

 if (argv[no]!=NULL)
 {
  slash=strlast(argv[no],'\\');
  if (slash==0xffff) slash=strlast(argv[no],':'); //if no backslash we take the colon
  temp=seekPath(&Disk,argv[no],x,f,0);
  if ((temp==NULL) && (switch_subdir==0))
  {
   #ifdef LANG_EN
    printf("File not found.\n");
   #endif
   #ifdef LANG_DE
    printf("Datei nicht gefunden.\n");
   #endif
   return;
  }
  if (slash!=strlen(argv[no])-1) //we have no trailing backslash
  {
   if (slash!=0xffff) strcpy(mask,argv[no]+slash+1);
   else strcpy(mask,argv[no]); //no backslash at all
   len=strlen(mask);
   if (strcspn(mask,"*?")==len) //we have no wildcards in the mask
   {
    if ((f.atrib & F_DIR)!=0) //whole thing is an explicit directory, f is ok
    {
     mask[0]='\0';
    }
    else {
    //else it is an explicit file, we keep the mask and make f the current dir
    temp=seekPath(&Disk,NULL,x,f,1); //current dir
    if (temp==NULL) return;
    }
   }
   else {
    //else it is a file search mask
    if (slash!=0xffff) {
     if ((slash==2) && (argv[no][1]==':')) strncat(path,argv[no],3);
     else strncat(path,argv[no],slash);
    }
    if (temp!=NULL) free(temp);
    temp=seekPath(&Disk,path,x,f,0);
   }
  }
 }
 else 
 {
  temp=seekPath(&Disk,NULL,x,f,1); //current dir
  if (temp==NULL) return;
 } 
 
 totbytes=0;
 totfiles=0;
 totdirs=0;
 if (switch_subdir==1)
 {
  Recurse(&Disk,&f,mask,x);
   #ifdef LANG_EN
    printf("\nTotal:\n %10lu Files\t%lu Bytes\n%10lu Directories\n",totfiles,totbytes,totdirs);
   #endif
   lines+=2;
   checkPage();   
   #ifdef LANG_DE
    printf("\nTotal:\n %10lu Dateien\t%lu Bytes\n%10lu Verzeichnisse\n",totfiles,totbytes,totdirs);
   #endif   
   lines+=2;
   checkPage();   
 }
 else PrintMaskedDir(&Disk,&f,mask,x); 
 free(temp);
 flushCache();
 if (switch_test)
  printf("%u swaps",no_swaps);
}
