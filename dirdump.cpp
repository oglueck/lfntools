// Longfilename DIR
// stack-checking off

#include <stdio.h> 
#include <stdlib.h> 
#include <conio.h>
#include <string.h>               
#include "lfn.cpp"     
#include <math.h>

dword totbytes,totfiles,totdirs,lines;
byte locdate;


void DumpMaskedDirCD(const struct DPB *b,struct longdirentry *pDir,char *mask,char *path)
{    
 struct FATfile f;
 size_t bufsize;
 void *buffer;
 FILE *dump;
 
 if (pDir==NULL) return;
 bufsize=b->sectors_per_cluster*b->bytes_per_sector;
 buffer=malloc(bufsize); 
 dump=fopen("\\dir.dmp","w");
 
 f.cluster=pDir->start_cluster;
 f.sector=firstSectorOfCluster(b,f.cluster);        
 dword pos=0;
 while (pos < pDir->length) {
  readCDSectors(b,f.sector,b->sectors_per_cluster, buffer);
  fwrite(buffer, bufsize, 1, dump); 
  pos += bufsize;
 }
 fclose(dump);
 free(buffer);
}

void DumpMaskedDir(const struct DPB *b,struct longdirentry *pDir,char *mask,char *path)
{              
 if (b->CD==1) {
  DumpMaskedDirCD(b, pDir, mask, path);
  return;
 };

 struct FATfile f;
 size_t bufsize;
 void *buffer;
 FILE *dump;
 
 if (pDir==NULL) return;
 bufsize=b->sectors_per_cluster*b->bytes_per_sector;
 buffer=malloc(bufsize); 
 dump=fopen("\\dir.dmp","w");
 
 f.cluster=pDir->start_cluster;
 f.sector=firstSectorOfCluster(b,f.cluster);        
 int eof=0;
 while (eof==0) {
  readSector(b->drive,f.sector,b->sectors_per_cluster, buffer);
  fwrite(buffer, bufsize, 1, dump); 
  eof=getNextCluster(b, &f);
 }
 fclose(dump);
 free(buffer);
}


void main(int argc, char *argv[], char *envp)
{
 struct DPB Disk;      
 struct longdirentry f;
 char drv,path[MAX_PATH_SIZE],mask[MAX_PATH_SIZE],*temp,x[MAX_PATH_SIZE];
 int no;
 unsigned int slash,len;
 
 temp=NULL;
 no=1;
 
 useVMCache=0;

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
  if (temp==NULL)
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
 DumpMaskedDir(&Disk,&f,mask,x); 
 free(temp);
 flushCache();  
 printf("Directory %s dumped to \\dir.dmp", f.name);
}
