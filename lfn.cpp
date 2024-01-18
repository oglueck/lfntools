/* lfn.cpp : Longfilenames for DOS
 * compile in LARGE modell (FAR pointers)
 *
 * Copyright (C) 1999 Ortwin Glueck
 *
 * GNU General Public License applies 
 *
 */                              

//Define language
#define LANG_EN
//#define LANG_DE

//vmemory.h present ?
#define VMEMORY


// includes
#include <stdio.h>
#include <dos.h>
#include "unicode2.inc"
#include <dos.h>   
#include <direct.h>
#include <stdlib.h>
#include <malloc.h> 
#include <string.h>
#include <ctype.h>  
#include <time.h> 

#ifdef VMEMORY
 #include <vmemory.h>
#endif
#include "lfn.h"
#ifdef LANG_DE
 #include "lfnmsgde.inc"
#endif
#ifdef LANG_EN
 #include "lfnmsgen.inc"
#endif    

#define LOG

//Functions

void initLFNLib(void) {
 initUnicodeLib();
 initCache();
}

#pragma optimize("gel",off) //cannot optimize asm

void getDOSver(struct dosver *v)
{ 
 byte a,b,c;
 __asm{
  mov ax,0x3000
  int 0x21
  mov [a],al
  mov [b],ah
  mov [c],bh
 }
 v->main=a;
 v->sub=b;
 v->oem=c;
}
      

int directAccessDenied(char drive) {
//tell if drive is remote and if we can make direct i/o
//0: allowed
//1: notallowed
 int att,e;
     
 e=1;
 __asm{
  mov ax,0x4409
  mov bl, [drive]
  inc bl
  int 0x21
  jc err
  mov [att],dx
  mov [e],0
 err:
  nop  
 }
 if (e==1) {  
  return 1; //error
 }
 else {    
  return (att & 0x8100); //bit 9 prohibits direct i/o, bit 15 is a subst
 }
}
      
int readSectorP(char drive,dword sector,word count, void *buffer) //extended version
//Read a physical sector (uncached)
{ 
 struct secblk x;
 x.sector=sector;
 x.no_sectors=count;
 x.pBuffer=buffer;
 int cy;
 
 if (win98==1) //use fat32 compatible read
 {                           
  __asm{
  mov  si,0   //read
  mov  cx, 0xffff
  lea  bx, [x]
  mov  dl, [drive]
  inc  dl
  mov  ax, 0x7305
  int 0x21  
  mov [cy],ax
  jc  err
  mov [cy],0
  err: 
  nop  
  }
  if (cy!=0)
  {
   if (cy==1) win98=0; //function not supported
   else
   {
    printf(msg[0],drive,sector,sector+count-1); 
    return 1;
   }
  }
  else return 0;
 }
 //use normal read, also if first fails. (NO ELSE)
 __asm{
   mov al,[drive]
   mov cx,0xffff     
   lea bx,[x]
   int 0x0025
   add sp,2
   jc err2
   mov [cy],0
   jmp quit
   err2:
   mov [cy],1
   quit:
   nop
  }
  if (cy==1)
  {
   printf(msg[1],drive,sector,sector+count-1); 
   return 1;
  }
  else return 0;
}

int writeSectorP(char drive,dword sector,word count, void *buffer,word typ) //extended version
//write a physical sector (uncached)
//type: 0x2000 =FAT, 0x4000 =DIR, 0x6000 =FILE
{
 struct secblk x;
 x.sector=sector;
 x.no_sectors=count;
 x.pBuffer=buffer;
 int cy;
 
 if (win98==1) //use fat32 compatible write
 {
  __asm{
  mov  cx, [typ]
  or   cx,0x0001 //write
  mov  si,cx
  mov  cx, 0xffff
  lea  bx, [x]
  mov  dl, [drive]
  inc  dl
  mov  ax, 0x7305
  int 0x21  
  mov [cy],ax
  jc  err
  mov [cy],0
  err: 
  nop  
  }
  if (cy!=0)
  {
   if (cy==1) win98=0; //function not supported
   else
   {
    printf(msg[2],drive,sector,sector+count-1); 
    return 1;
   }
  }
  else return 0;
 }
  __asm{
  mov al,[drive]
  mov cx,0xffff     
  lea bx,[x]
  int 0x0026
  add sp,2
  jc err2
  mov [cy],0
  jmp quit
  err2:
  mov [cy],1
quit:
  nop
 }
 if (cy)
 {
  printf(msg[3],drive,sector,sector+count-1); 
  return 1;
 }
 return 0;
}

//Cache
            
int no_swaps=0;

#ifdef VMEMORY            
cachepage __far* getCacheArea(int i) {
 if ((current_area_no!=i) && (current_area_no!=-1)) {
  _vunlock(cacheArea[current_area_no], current_area_dirty);
 }     
 if ((current_area_no==i) && (current_area_no!=-1)) {
  return current_area;
 }
 current_area_no=i;
 current_area_dirty=_VM_CLEAN;
 current_area=(cachepage __far*) _vlock(cacheArea[i]);
 no_swaps++;
 if (current_area==NULL) {
  printf(msg[53]);   
  _vheapterm();
  exit(1); //panic
 }
 return current_area;
}
#endif

//Cache must be initialized BEFORE any call to makeDPB for a CD-ROM!
void initCache(void)
{      
 int i,j;
 
 if (bypassCache==1) return;
 if (cacheWasSetup==1) return;
 
 //virtual memory
#ifdef VMEMORY
 if (useVMCache!=0) {
  useVMCache=_vheapinit(2000,4096, _VM_EMS | _VM_XMS); //use max 64K DOS mem
  if (useVMCache!=0) {
   no_cache_areas=0;
   for (i=0;i<MAX_CACHE_AREAS;i++) {
    cacheArea[i]=_vmalloc(MAX_CACHE_AREA_SIZE);     //allocate space        
    unsigned long size=_vmsize(cacheArea[i]);
    
    if (cacheArea[i]==_VM_NULL) {
     if (i==0) { 
      printf(msg[52]);
      _vheapterm();
      useVMCache=0;  
      goto noVM_label;
     }
     break;
    }     
    no_cache_areas++;
   }//for
    
   //create quick lookup table                  
   if ((long)no_pages_per_area*no_cache_areas>32766) {
    printf(msg[54]);
    _vheapterm();
    exit(1);
   }
   
   int no_pages=no_pages_per_area*no_cache_areas;
   vcachelookup = (vcacheinfo*) malloc(sizeof(vcacheinfo)*no_pages);
   //printf("%lu mem",(unsigned long)sizeof(vcacheinfo)*no_pages);
   if (vcachelookup==NULL) {
    printf(msg[4]); 
    _vheapterm();
    exit(1); //panic
   }
   for (i=0;i<no_pages;i++) {
    vcachelookup[i].empty=1;
   }
   
   current_area_no=-1;
   current_area=NULL; 
   current_area_dirty=_VM_CLEAN; 
                 
   for (i=0;i<no_cache_areas;i++) {
    unsigned long size=_vmsize(cacheArea[i]);
    cachepage __far *p = getCacheArea(i);
    for (j=0;j<no_pages_per_area;j++) {
     p[j].dirty=EMPTY;
    }                  
    current_area_dirty=_VM_DIRTY;
   }           
   no_free_pages=no_pages_per_area*no_cache_areas;
   total_cache_pages = no_free_pages;      
  }
 }
 
noVM_label:
#else
 useVMCache = 0;
#endif
      
 // no virtual memory
 if (useVMCache==0) {
  for (i=0;i<MAX_CACHE_PAGES;i++) {
   cache[i]=(struct cachepage*)malloc(sizeof(cachepage));
   if (cache[i]==NULL) {
    printf(msg[4]);
    exit(1);
   }
   cache[i]->dirty=EMPTY;
  }      
  no_free_pages=MAX_CACHE_PAGES;                                           
  total_cache_pages=no_free_pages;
 }
 for (i=0;i<MAX_DRIVES;i++) bpsd[i]=MAX_SECTOR_SIZE;
 timestamp=0;
 cacheWasSetup=1;
}
 
int isInCache(char drive, dword sector,word *page)
//1 if sector is in cache
{  
 int i;
                      
 if (useVMCache!=0) {
#ifdef VMEMORY 
  int no_pages=no_pages_per_area*no_cache_areas;
  for (i=0;i<no_pages;i++) {
   if ((vcachelookup[i].empty==0) && (vcachelookup[i].drive==drive) && (vcachelookup[i].sector==sector)) {
    *page=i;
    return 1;
   }
  }          
#endif
 }
 else {
  //no VMcache
  for (i=0;i<MAX_CACHE_PAGES;i++)
  {
   if ((cache[i]->dirty!=EMPTY) && (cache[i]->drive==drive) && (cache[i]->sector==sector))
   {
    *page=i;
    return 1;
   }
  }
 }//useVMcache
 return 0;
} 

word getEmptyPage(void)
{      
 int i,j;
      
 if (no_free_pages==0) return 0xFFFF;
      
 if (useVMCache!=0) {   
#ifdef VMEMORY 
  //seek in current area
  cachepage __far *p = current_area; 
  if (p!=NULL) {
   for (j=0;j<no_pages_per_area;j++) {
    if (p[j].dirty==EMPTY) return no_pages_per_area*current_area_no+j;
   }
  }
                        
  //seek all areas                      
  int no_pages=no_pages_per_area*no_cache_areas;
  for (i=0;i<no_pages;i++) {
   if (vcachelookup[i].empty==1) {
    return i;
   }
  } 
#endif  
 }
 else {
  //no VMcache
  for (i=0;i<MAX_CACHE_PAGES;i++)
   if (cache[i]->dirty==EMPTY) return i;
 }
 return 0xFFFF;
}

void discardPage(word page)
{                                         
 int i,j;
 if (useVMCache!=0) {
#ifdef VMEMORY 
  i=page/no_pages_per_area;
  j=page % no_pages_per_area;
  cachepage __far *p=getCacheArea(i);
  if (p[j].dirty==EMPTY) return;
  if (p[j].dirty==DIRTY) {
   writeSectorP(p[j].drive, p[j].sector,1,p[j].data, p[j].typ);
  }
  p[j].dirty=EMPTY;
  current_area_dirty=_VM_DIRTY;
  vcachelookup[page].empty=1;
#endif  
 }
 else {       
  //no VMcache
  if (cache[page]->dirty==EMPTY) return;
  if (cache[page]->dirty==DIRTY)
   writeSectorP(cache[page]->drive, cache[page]->sector,1,cache[page]->data, cache[page]->typ);
  cache[page]->dirty=EMPTY;
 }
 no_free_pages++;
}

word discardOldestPage(void)
{                 
 word page=0xFFFF;
 dword time=0xFFFFFFFF;
 int i;
 
 if (useVMCache!=0) {
#ifdef VMEMORY      
  int no_pages=no_pages_per_area*no_cache_areas;
  for (i=0;i<no_pages;i++) {
   if ((vcachelookup[i].empty==0) && (vcachelookup[i].timestamp<time)) {
    time=vcachelookup[i].timestamp;
    page=i;
   }
  }           
#endif
 }
 else {       
  //no VMcache 
  for (i=0;i<MAX_CACHE_PAGES;i++)
  {
   if ((cache[i]->dirty!=EMPTY) && (cache[i]->timestamp<time))
   {
    time=cache[i]->timestamp;
    page=i;
   }
  } 
 }
 if (page!=0xFFFF) discardPage(page);
 return page;
} 

//discards the oldest 20% of the pages. Call if cache is full and you need space.
word discardOldPages(void)
{
  int toclear = total_cache_pages / 5;
  word page = 0xFFFF;
  for (int i=0; i<toclear; i++) {
    page = discardOldestPage();
  }        
  return page;
}

void flushCache(void)
{      
 int i;
 
 if (cacheWasSetup==0) return;
 if (useVMCache!=0) { 
#ifdef VMEMORY 
  for (i=0;i<no_cache_areas*no_pages_per_area;i++) discardPage(i);
#endif  
 }
 else {        
  //no VMcache 
  for (i=0;i<MAX_CACHE_PAGES;i++) discardPage(i);
 }
}

void killCache(void)
{      
 int i;
 
 if (cacheWasSetup==0) return;
 flushCache();
 if (useVMCache!=0) { 
#ifdef VMEMORY 
  if (current_area_no!=-1) {
   _vunlock(cacheArea[current_area_no], current_area_dirty);
   current_area_no=-1;                  
   current_area=NULL;
   current_area_dirty=_VM_CLEAN;
  }
  for (i=0;i<no_cache_areas;i++) _vfree(cacheArea[i]);
  _vheapterm();  
  no_cache_areas=0;
#endif  
 }
 else {        
  //no VMcache 
  for (i=0;i<MAX_CACHE_PAGES;i++)
   if (cache[i]!=NULL) free(cache[i]);
 }
 cacheWasSetup=0; 
}

int readSector(char drive,dword sector,word count, void *buffer)
//read a cached sector
{            
 word h,i,j;
 word page;
 if ((bypassCache==1) || (cacheWasSetup==0))
 {
  return readSectorP(drive,sector,count,buffer);
 }
 for (i=0;i<count;i++,sector++)
 {
  if (isInCache(drive, sector, &page))
  {             
   if (useVMCache!=0) {   
#ifdef VMEMORY   
    h=page/no_pages_per_area;
    j=page % no_pages_per_area;
    cachepage __far *p=getCacheArea(h);
    memcpy((void*)&(((char*)buffer)[i*bpsd[drive]]),p[j].data,bpsd[drive]);    
    timestamp++;
    p[j].timestamp=timestamp;
    current_area_dirty=_VM_DIRTY;
    vcachelookup[page].timestamp=timestamp;
#endif    
   }
   else {      
    //no VMcache 
    memcpy((void*)&(((char*)buffer)[i*bpsd[drive]]),cache[page]->data,bpsd[drive]);
    timestamp++;
    cache[page]->timestamp=timestamp;
   }
  }
  else
  {
   page=getEmptyPage();
   if (page==0xFFFF) page=discardOldPages();
   no_free_pages--;
   
   if (useVMCache!=0) {     
#ifdef VMEMORY   
    h=page/no_pages_per_area;
    j=page % no_pages_per_area;
    cachepage __far *p=getCacheArea(h);
    if (readSectorP(drive,sector,1,&p[j].data)!=0) {
     flushCache();
     return 1;
    }
    p[j].drive=drive;
    p[j].sector=sector;
    p[j].dirty=CLEAN;        
    timestamp++;
    p[j].timestamp=timestamp;
    current_area_dirty=_VM_DIRTY;
    vcachelookup[page].empty=0;
    vcachelookup[page].drive=drive;
    vcachelookup[page].sector=sector;
    vcachelookup[page].timestamp=timestamp;
    memcpy((void*)&(((char*)buffer)[i*bpsd[drive]]),p[j].data,bpsd[drive]);
#endif    
   }
   else {
    //no VMcache 
    if (readSectorP(drive,sector,1,&cache[page]->data)!=0)
    {
     flushCache();
     return 1;
    }
    cache[page]->drive=drive;
    cache[page]->sector=sector;
    cache[page]->dirty=CLEAN;        
    timestamp++;
    cache[page]->timestamp=timestamp;
    memcpy((void*)&(((char*)buffer)[i*bpsd[drive]]),cache[page]->data,bpsd[drive]);
   }
  }
 }
 return 0;
}

int writeSector(char drive,dword sector,word count, void *buffer,word typ)
//write a cached sector
{
 word h,i,j;
 word page;
 
 if ((bypassCache==1) || (cacheWasSetup==0))
 {
  return writeSectorP(drive,sector,count,buffer,typ);
 }
 for (i=0;i<count;i++,sector++)
 {
  if (isInCache(drive, sector, &page))
  {
   if (useVMCache!=0) {      
#ifdef VMEMORY   
    h=page/no_pages_per_area;
    j=page % no_pages_per_area;
    cachepage __far *p=getCacheArea(h);
    
    memcpy(p[j].data,(void*)&(((char*)buffer)[i*bpsd[drive]]),bpsd[drive]);
    timestamp++;
    p[j].timestamp=timestamp;
    p[j].dirty=DIRTY;     
    p[j].typ=typ; //we may have a page in cache whose type we do not know 
    current_area_dirty=_VM_DIRTY;
    vcachelookup[page].timestamp=timestamp;
#endif    
   }
   else {        
    //no VMcache 
    memcpy(cache[page]->data,(void*)&(((char*)buffer)[i*bpsd[drive]]),bpsd[drive]);
    timestamp++;
    cache[page]->timestamp=timestamp;
    cache[page]->dirty=DIRTY;     
    cache[page]->typ=typ; //we may have a page in cache whose type we do not know 
   }
  }
  else
  {
   page=getEmptyPage();
   if (page==0xFFFF) page=discardOldPages();
   no_free_pages--;
   
   if (useVMCache!=0) {           
#ifdef VMEMORY   
    h=page/no_pages_per_area;
    j=page % no_pages_per_area;
    cachepage __far *p=getCacheArea(h);
    memcpy(p[j].data,(void*)&(((char*)buffer)[i*bpsd[drive]]),bpsd[drive]);
    p[j].drive=drive;
    p[j].sector=sector;
    p[j].dirty=DIRTY;
    timestamp++;
    p[j].timestamp=timestamp;
    p[j].typ=typ;
    current_area_dirty=_VM_DIRTY;    
    vcachelookup[page].empty=0;
    vcachelookup[page].drive=drive;
    vcachelookup[page].sector=sector;
    vcachelookup[page].timestamp=timestamp;
#endif    
   }
   else {        
    //no VMcache 
    memcpy(cache[page]->data,(void*)&(((char*)buffer)[i*bpsd[drive]]),bpsd[drive]);
    cache[page]->drive=drive;
    cache[page]->sector=sector;
    cache[page]->dirty=DIRTY;
    timestamp++;
    cache[page]->timestamp=timestamp;
    cache[page]->typ=typ;
   }
  }
 } 
 return 0;
}


int lockDrive(byte drv)
{         
 int cy;
 if (noLocking==1) return 0; //not supported
 __asm{
  mov ax, 0x440D       
  mov bh, 0
  mov bl, [drv]
  inc bl   
  mov cx, 0x084A      
  mov dx, 0x0000
  int 0x21
  jc error
  mov [cy],0
  jmp quit
error:
  mov [cy],1
quit:
  nop
 }
 if (cy==1)
 {         
  printf(msg[5],drv+'A');
  exit(1);
 }
 return 0;
}

int unlockDrive(byte drv)
{
 int cy;
 flushCache();
 if (noLocking==1) return 0; //not supported
 __asm{
  mov ax, 0x440D     
  mov bl, [drv]
  inc bl
  mov cx, 0x086A        ; device category (must be 08h)
  int 0x21
  jc error
  mov [cy],0
  jmp quit
error:
  mov [cy],1
quit:
  nop
 }
 if (cy==1)
 {         
  printf(msg[6],drv);
  return 1;
 }
 return 0;
}

int isCDRomSupported(word drv)
//return nonzero if drv is a supported cd-rom drive
{    
 int stat;
 __asm{
  mov ax,0x150B
  mov cx,[drv]
  int 0x2f
  cmp bx,0xADAD
  je  mscd
  xor ax,ax
 mscd:
  mov [stat],ax
 }
 return stat;
}

int isCDRom(word drv)
//return 1 if drv is a cd-rom drive
{ 
 word start,num;
 __asm{
  mov ax,0x1500
  mov bx,0
  int 0x2f
  mov [num],bx
  mov [start],cx
 }
 if ((drv>=start) && (drv<start+num)) return 1;
 else return 0;
}

dword rba2lba(RedBookAddress rba)
{
 return (dword)rba.minute*4500+rba.second*75+rba.frame-150;
}

void InitReqHeader(BasicReqHeader *brh,byte len,byte cmd,byte unit)
{
 brh->length=len;
 brh->sub_unit=unit;
 brh->command=cmd;
 brh->status=0;
}

void InitInputHeader(InputReqHeader *irh,void _far *cb,word length,byte unit)
{
 InitReqHeader((BasicReqHeader*) irh,26,3,unit);
 irh->media_descriptor=0;
 irh->transfer_address=(dword)cb;
 irh->size=length;
 irh->start=0;
 irh->error=0;
}

void InitAudioDiskInfo(AudioDiskInfo *adi)
{
 adi->sub_function=0x0A;
}

void InitAudioTrackInfo(AudioTrackInfo *ati, byte track)
{
 ati->sub_function=0x0B;
 ati->track=track;
};

void SendDDRequest(char drive,void _far *rh)
{
 _asm
 {
  mov ax,0x1510
  mov cx,0
  mov cl,[drive]
  les bx,[rh]
  int 0x2F
 }
}

int GetAudioDiskInfo(char drive, unsigned char subunit,AudioDiskInfo *adi)
{
 struct InputReqHeader   i;
 unsigned int            status;
 char                    error;

 InitAudioDiskInfo(adi);
 InitInputHeader(&i, (void _far*)adi, sizeof(struct AudioDiskInfo), subunit);
 SendDDRequest(drive, (void _far*)&i);
 status = i.basic.status;
 if ((status & 0x8000) !=0)
 {
  error = status & 0x0F;
  printf(msg[7], error);
  return 0;
 }
 if ((status & 0x0200) !=0)
 {
    printf(msg[8]);
    return 0;
 }
 return 1;
}

int GetAudioTrackInfo(char drive, byte subunit, byte trackNo,AudioTrackInfo *ati)
{
 struct InputReqHeader   i;
 unsigned int            status;
 char                    error;

 InitAudioTrackInfo(ati, trackNo);
 InitInputHeader(&i, (void far*)ati, sizeof(struct AudioTrackInfo), subunit);
 SendDDRequest(drive, (void far*)&i);
 status = i.basic.status;
 if ((status & 0x8000) !=0)
 {
  error = status & 0x0F;
  printf(msg[9], error);
  return 0;
 }
 if ((status & 0x0200) !=0)
 {
  printf(msg[8]);
  return 0;
 }
 return 1;
}

dword DetermineVolStart(byte drive)
{
 AudioDiskInfo adi;
 AudioTrackInfo ati;
 byte track, dataTrack;

 if (GetAudioDiskInfo(drive,0,&adi)!=0)
 {
  if (forceTrack!=0)
  {
   if (forceTrack>adi.lastTrack)
   {
    printf(msg[10],forceTrack); 
    flushCache();
    exit(1);
    return 0;//quiet compiler
   }
   GetAudioTrackInfo(drive,0,forceTrack,&ati);
   if ((ati.trackInfo & DATA_TRACK)==0)
   {
    printf(msg[11],forceTrack);
    flushCache();
    exit(1);
    return 0;//quiet compiler
   }
   return rba2lba(ati.start);
  }
  if (adi.lastTrack>=adi.firstTrack)
  { 
   dataTrack=0;
   for (track=adi.firstTrack;track<=adi.lastTrack;track++) //Mixed-Mode
   {
    GetAudioTrackInfo(drive,0,track,&ati);
    if ((ati.trackInfo & DATA_TRACK)!=0) dataTrack=track;
   }
   if (dataTrack!=0)
   {
    GetAudioTrackInfo(drive,0,dataTrack,&ati);
    return rba2lba(ati.start);
   }
   else
   {
    printf(msg[12],'A'+drive);
    flushCache();
    exit(1);
    return 0; //quiet compiler
   }
  }
  else //CD-Extra
  {
   return rba2lba(adi.leadOutTrack)+11250;  //Gap is 2:30.00
  }
 } 
 flushCache();
 exit(1);
 return 0; //quiet compiler
}

int readCDSectorsP(const struct DPB *b,dword sector,word count,void _far *buffer)   
//does not work under Windows! (but we are in DOS mode, remember?)
{ 
 byte cy;
 word drive=b->drive;
 __asm{  
  mov ax,0x1508
  mov cx,[drive]
  les bx,[buffer]
  mov di,word ptr [sector]
  mov si,word ptr [sector+2]
  mov dx,[count]
  int 0x2F
  jc err
  mov [cy],0
  jmp noerr
 err: 
  mov [cy],1
 noerr:
  nop
 }
 if (cy!=0)
 {
  printf(msg[13]);
  return 1;
 } 
 else return 0;
}

int readCDSectors(const struct DPB* b,dword psector,word count,void _far *buffer)
//cached   
//a CD sector can consist of several cache pages since CD sectors are usually larger than MAX_SECTOR_SIZE
{     
 word h,i,j;
 word page,tpage;
 word pages_per_sector;
 dword csector, sector=psector;
 void *sbuf;
 
 if (bpsd[b->drive]>MAX_SECTOR_SIZE) {
  pages_per_sector=bpsd[b->drive] / MAX_SECTOR_SIZE;
  sector=psector*pages_per_sector;                     //span cd sector across several pages
 }
 else pages_per_sector=1;
 
 if ((bypassCache==1) || (cacheWasSetup==0)) {
  return readCDSectorsP(b,psector,count,buffer);
 }                                           
 
 for (i=0;i<count;i++)
 {          
  for (word p=0;p<pages_per_sector;p++,sector++)
  {
   if (isInCache(b->drive, sector, &page))
   {
    if (useVMCache!=0) { 
#ifdef VMEMORY    
     h=page/no_pages_per_area;
     j=page % no_pages_per_area;
     cachepage __far *cp=getCacheArea(h);
     memcpy((void*)&(((char*)buffer)[i*bpsd[b->drive]+p*MAX_SECTOR_SIZE]),cp[j].data,MAX_SECTOR_SIZE);
     timestamp++;
     cp[j].timestamp=timestamp;
     current_area_dirty=_VM_DIRTY;
     vcachelookup[page].timestamp=timestamp;
#endif     
    }
    else {
     //no VMcache
     memcpy((void*)&(((char*)buffer)[i*bpsd[b->drive]+p*MAX_SECTOR_SIZE]),cache[page]->data,MAX_SECTOR_SIZE);
     timestamp++;
     cache[page]->timestamp=timestamp;
    }
   }
   else
   {//Read sector and put in cache
    sbuf=malloc(b->bytes_per_sector);
    if (readCDSectorsP(b,psector,1,sbuf)!=0)
    {
     flushCache();
     free(sbuf);
     return 1;
    }                             
    csector=psector*pages_per_sector;
    for (word q=0;q<pages_per_sector;q++)
    {
     //put the whole sector into the cache but only pages which are not yet in
     if (!isInCache(b->drive,csector+q,&tpage)) {
      tpage=getEmptyPage();
      if (tpage==0xFFFF) tpage=discardOldestPage();
      no_free_pages--;
      
      if (useVMCache!=0) {      
#ifdef VMEMORY      
       h=tpage/no_pages_per_area;
       j=tpage % no_pages_per_area;
       cachepage __far *cp=getCacheArea(h);
       cp[j].drive=b->drive;
       cp[j].sector=csector+q;
       cp[j].dirty=CLEAN;        
       timestamp++;
       cp[j].timestamp=timestamp;
       memcpy(cp[j].data,(void*)&(((char*)sbuf)[q*MAX_SECTOR_SIZE]),MAX_SECTOR_SIZE);   
       current_area_dirty=_VM_DIRTY;
       vcachelookup[tpage].empty=0;
       vcachelookup[tpage].drive=b->drive;
       vcachelookup[tpage].sector=csector+q;
       vcachelookup[tpage].timestamp=timestamp;
#endif       
      }
      else {   
       //no VMcache  
       cache[tpage]->drive=b->drive;
       cache[tpage]->sector=csector+q;
       cache[tpage]->dirty=CLEAN;        
       timestamp++;
       cache[tpage]->timestamp=timestamp;
       memcpy(cache[tpage]->data,(void*)&(((char*)sbuf)[q*MAX_SECTOR_SIZE]),MAX_SECTOR_SIZE);   
      }     
     }
    }
    free(sbuf);
    
    isInCache(b->drive, sector, &page); //find the page we want
    if (useVMCache!=0) {
#ifdef VMEMORY     
     h=page/no_pages_per_area;
     j=page % no_pages_per_area;
     cachepage __far *cp=getCacheArea(h);
     memcpy((void*)&(((char*)buffer)[i*bpsd[b->drive]+p*MAX_SECTOR_SIZE]),cp[j].data,MAX_SECTOR_SIZE);
#endif     
    }
    else {   
     //no VMcache  
     memcpy((void*)&(((char*)buffer)[i*bpsd[b->drive]+p*MAX_SECTOR_SIZE]),cache[page]->data,MAX_SECTOR_SIZE);
    }
   } //if cached
  }// for p
 }// for i
 return 0;
}

#pragma optimize("",on)

int getVTOC(struct iso_primary_descriptor* desc, struct DPB *d)
//get the volumedescriptor of ISO or Joliet
//return 0 if ok                        
//usc2 in a SVD (not PVD!) identifies Joliet!
{ 
 int cy,usc2,joliet,sector;            
 d->cdfs=CD_NO;    
 joliet=0;
 sector=VTOC_START+1;
 do
 {
  cy=readCDSectorsP(d,sector+d->CD_vol_start,1,desc);
  if (cy!=0) return 1;      
  usc2= ((desc->esc_seq[0]=='%') && (desc->esc_seq[1]=='/'))
   && ((desc->esc_seq[2]=='@') || (desc->esc_seq[2]=='C') || (desc->esc_seq[2]=='E'));
  joliet = ((usc2==1) && (strncmp(desc->ID,ISO_STANDARD_ID,5)==0) && ((desc->vol_flags & 1)==0)
   && (desc->type_of_descriptor==2));
  sector++; 
 } while ((desc->type_of_descriptor!=0xff) && (joliet==0));  
 if ((forceIso==1) || (joliet==0)) //no Joliet Format
 {
  cy=readCDSectorsP(d,VTOC_START+d->CD_vol_start,1,desc);
  if (cy!=0) return 1;
  if (strncmp(desc->ID,ISO_STANDARD_ID,5)!=0) return 1;
  d->cdfs=CD_ISO;
 }
 else
  d->cdfs=CD_JOLIET;
 return 0;
}

int makeDPB(char drive,struct DPB &d)
//Build Disk Parameter Block
{                                      
 int plausible=1;
 char *sector=new char[MAX_SECTOR_SIZE];
 struct BPB *b=(struct BPB*)&sector[0x0B];
 struct BPB32 *b32=(struct BPB32*)&sector[0x0B];
 struct dosver ver;
 struct iso_primary_descriptor _far D;   
  
 getDOSver(&ver);
 if (ver.oem!=0xff) 
 {
  win98=0;  //PC-DOS does not support this!
  noLocking=1;
 }
 
 if ((100*ver.main+ver.sub)<710) win98=0; //is it >=Windows 98?
 if (ver.main<7) noLocking=1;  //old DOS does not support locking
 
 if (directAccessDenied(drive)!=0) {
  printf(msg[51]);
  exit(1);
 }
 
 if (isCDRomSupported(drive)!=0) 
 {
  d.CD=1;
  d.drive=drive;
  d.fat_type=CD;
  delete sector;
  d.CD_vol_start=DetermineVolStart(drive);
  if (getVTOC(&D,&d)!=0)             
  {
   printf(msg[15]);
   exit(1);
  }
  d.bytes_per_sector=D.block_size;
  bpsd[drive]=D.block_size;
  d.sectors_per_cluster=1;
  d.reserved_sectors=0;
  d.no_fats=0;
  d.next_cluster=0;
  d.no_root_entries=D.root_length;
  d.media_descriptor=0xF8;
  d.sectors_per_fat=0;
  d.first_data_sector=0;
  d.no_root_sectors=0;
  d.fat_entries_per_sector=0;
  d.dir_entries_per_sector=0;
  d.fs_info_sector=0;
  d.first_root_sector=D.root_start;
  d.first_root_cluster=D.root_start; //for compatibility
  d.no_sectors=D.volume_size;
  d.no_clusters=D.volume_size;
  if (d.cdfs==CD_ISO)
  {
   strncpy(d.label,D.cd_title,32);
   d.CD_iso_root=D.root_start;
  }
  else if (d.cdfs==CD_JOLIET)
  {
   b2lnstr((word*)D.cd_title,16);
   uni2ascii((unsigned int*)D.cd_title,(unsigned char*)d.label);
   readCDSectorsP(&d,16+d.CD_vol_start,1,&D); 
   d.CD_iso_root=D.root_start;
   d.CD_iso_root_length=D.root_length;
  };
  
  return 0;
 }
 else d.CD=0;
 d.CD_vol_start=0;
 
 if (readSector(drive,0,1,sector))
 {
  printf(msg[16]);
  delete sector;
  return 1;
 }
 
 //Let the sector tell us what fs it is
 if (!strncmp("FAT12",b->fat_type,5)) d.fat_type=FAT12;
 else if (!strncmp("FAT16",b->fat_type,5)) d.fat_type=FAT16;
 else if (!strncmp("FAT32",b32->fat_type,5)) d.fat_type=FAT32;
 else d.fat_type=NO; //not stated in the sector, we have to decide later

 d.drive=drive;
 d.next_cluster=2;
 d.bytes_per_sector=b->bytes_per_sector;
 bpsd[drive]=d.bytes_per_sector;
 d.sectors_per_cluster=b->sectors_per_cluster;
 d.reserved_sectors=b->reserved_sectors;
 d.no_fats=b->no_fats;
 d.no_root_entries=b->no_root_entries;
 d.media_descriptor=b->media_descriptor;
 if (b->sectors_per_fat==0)
  d.sectors_per_fat=b32->sectors_per_fat32;
 else
  d.sectors_per_fat=b->sectors_per_fat;
 if (b->no_sectors==0) d.no_sectors=b->no_large_sectors;
 else d.no_sectors=b->no_sectors;
 
 //plausibility check
 if ((d.bytes_per_sector<256) || (d.bytes_per_sector>65536) || (d.bytes_per_sector%256!=0) ||
     (d.sectors_per_cluster>1024) || (d.sectors_per_cluster==0) ||
     (d.no_fats>8)) {
   printf(msg[50]);  
   plausible=0;
 }    
 
 //FAT Type not yet clear
 if (d.fat_type==NO)
 {  
  if (d.no_sectors/d.sectors_per_cluster>268435456)
  {
   printf(msg[17]);
   delete sector;
   flushCache();
   exit(1);
  }
  if (d.no_sectors/d.sectors_per_cluster<4087) d.fat_type=FAT12;
  else if (d.no_sectors/d.sectors_per_cluster>65525) d.fat_type=FAT32;
  else d.fat_type=FAT16;
 }
 
 if ((d.fat_type==FAT12) || (d.fat_type==FAT16)) 
 {
  d.no_root_sectors=(word)d.no_root_entries*32/d.bytes_per_sector;
  d.first_root_sector=(dword)d.reserved_sectors+(dword)d.no_fats*d.sectors_per_fat;
  d.first_data_sector=(dword)d.first_root_sector+(dword)d.no_root_sectors;  
  d.first_root_cluster=0;
  strnset(d.label,0,sizeof(d.label));
  strncpy(d.label,b->volume_label,11);
 }
 else //FAT32
 {   
  d.no_root_sectors=0;
  d.first_root_cluster=b32->cluster_of_root;
  d.first_data_sector=(dword)d.reserved_sectors+(dword)d.no_fats*d.sectors_per_fat;
  d.fs_info_sector=b32->fs_info_sector;
  d.first_root_sector=d.first_data_sector+(dword)d.sectors_per_cluster*(d.first_root_cluster-2);
  strncpy(d.label,b->volume_label,11);
  
  //read FSInfo
  struct BIGFATBOOTFSINFO *fsi;           
  fsi=(struct BIGFATBOOTFSINFO*)malloc(b->bytes_per_sector);
  if (fsi==NULL)
  {
   printf(msg[27]);
   return 1;
  }
  readSector(d.drive,d.fs_info_sector,1,fsi);
  if ((fsi->start_signature==0x41615252) && (fsi->signature==0x61417272) && (fsi->end_signature==0xAA550000)) {
   d.next_cluster=fsi->next_free_cluster;
  }
  free(fsi);
 }
 d.CD_iso_root=d.first_root_sector;
 d.no_clusters=(d.no_sectors-d.first_data_sector)/d.sectors_per_cluster+2; //including 2 dummy clusters at beginning of FAT

 switch (d.fat_type)
 {
  case FAT12: d.fat_entries_per_sector=d.bytes_per_sector/3*2; break;  //may vary +/- 1 !!! Do not use!
  case FAT16: d.fat_entries_per_sector=d.bytes_per_sector/2;   break;
  case FAT32: d.fat_entries_per_sector=d.bytes_per_sector/4;  
              d.no_root_entries=0;
              break;
 }
 d.dir_entries_per_sector=d.bytes_per_sector/sizeof(struct direntry);
 delete sector; 
 if (plausible==0) return 1;
 return 0;
}

byte makeChecksum(char *aliasname)
{          
 byte sum,i;  
 for (sum = i = 0; i < 11; i++)
  sum = (((sum&1)<<7)|((sum&0xfe)>>1)) + aliasname[i];
 sum &= 0xff;
 return sum;
}

void unincpy(word *dest,word *source,word count)
{     
 word i;
 for (i=0;i<count;i++) dest[i]=source[i];
}

void lfnString(struct lfnentry &lfn,char *bs,word *ws) // 13 characters plus \0
{
 unincpy(&ws[0],&lfn.name1[0],5);
 unincpy(&ws[5],&lfn.name2[0],6);
 unincpy(&ws[11],&lfn.name3[0],2);
 ws[13]=0;
 uni2ascii((unsigned int*)ws,(unsigned char*)&bs[0]);
}

unsigned int strpos(const char *string,int len,char c)
{
 for (int i=0;i<len;i++)
 {
  if (string[i]==c) break;
  if (string[i]==0) break;
 }
 return i;
}

void b2lstr(word* s)
//big endian to little endian string
{ 
 word w;
 for (;*s!=0;s++)
 {
  w=*s & 0x00FF;
  *s=*s >> 8;
  w=w << 8;
  *s=*s | w;
 }
}

void b2lnstr(word* s,word n)
//big endian to little endian string
{ 
 word w,i;
 for (i=0;(*s!=0) && (i<n);s++,i++)
 {
  w=*s & 0x00FF;
  *s=*s >> 8;
  w=w << 8;
  *s=*s | w;
 }
}


////////////
//////////// Functions making the whole thing independent of FAT type
////////////

dword FATsector(const struct DPB *b,dword cluster)  
//returns sector containing the FAT entry
{
 switch (b->fat_type)
 {
   case FAT12: 
              return cluster*3/2/b->bytes_per_sector+b->reserved_sectors; //returns sector containing the first byte
              break;      
   case FAT16:
              return cluster/b->fat_entries_per_sector+b->reserved_sectors;
              break;
   case FAT32:        
              return cluster/b->fat_entries_per_sector+b->reserved_sectors;
              break;
 }
 return 0;
}

dword getFATval(const struct DPB *b,void *FAT,dword cluster)
{
 dword pos;
 word val;
 switch (b->fat_type)
 {
  case FAT12:
             pos=cluster/2*3; //abs position of triple byte
             if (cluster%2==0)
             {
              pos=pos % b->bytes_per_sector;
              val=((byte*)FAT)[pos];
              if (pos+1<b->bytes_per_sector)
               val+=((((byte*)FAT)[pos+1] & 0x0f) << 8); //even Cluster:  bc 0a 00
              else
               fat12of=1; //entry is in both sectors
             }
             else 
             {
              pos++;
              pos=pos % b->bytes_per_sector;
              val=((((byte*)FAT)[pos] & 0xf0) >> 4);
              if (pos+1<b->bytes_per_sector)
               val+=(((byte*)FAT)[pos+1] << 4); //odd Cluster: 00 c0 ab
              else
               fat12of=1; //entry is in both sectors
             }
             return (dword)val;
             break;
  case FAT16:
             pos=cluster%b->fat_entries_per_sector;
             return (dword)((word*)FAT)[pos];
             break;
  case FAT32:
             pos=cluster % b->fat_entries_per_sector;
             return (dword)((dword*)FAT)[pos];      //highes 4 bits are reserved and not part of the cluster number!
             break;             
 }
 return 0;
}

void setFATval(const struct DPB *b,void *FAT,dword cluster,dword val)
{ 
 dword pos;
 switch (b->fat_type)
 {                              
  case FAT12:
             if ((word)val==0xffff) val=0xfff;
             pos=cluster/2*3;  //abs position of triple byte
             if (cluster%2==0) //even: bc 0a 00
             {                     
              pos=pos % b->bytes_per_sector;
              ((byte*)FAT)[pos]=(byte)(val & 0xff);
              if (pos+1<b->bytes_per_sector)
              {
               ((byte*)FAT)[pos+1]&=0xf0;
               ((byte*)FAT)[pos+1]|=(byte)((val & 0x0f00) >> 8);
              }
              else
               fat12of=1; //entry is in both sectors
             }
             else               //odd: 00 c0 ab
             {                                      
              pos++;
              pos=pos % b->bytes_per_sector;
              ((byte*)FAT)[pos]&=0x0f;
              ((byte*)FAT)[pos]|=(byte)((val & 0x00f) << 4);
              if (pos+1<b->bytes_per_sector)
               ((byte*)FAT)[pos+1]=(byte)((val & 0xff0) >> 4);
              else
               fat12of=1; //entry is in both sectors
             }
             break;
  case FAT16:
             pos=cluster % b->fat_entries_per_sector;
             ((word*)FAT)[pos]=(word)val;
             break;
  case FAT32:
             pos=cluster % b->fat_entries_per_sector;
             ((dword*)FAT)[pos]=val;             
             break;             
 }
}

dword getFAT12valX(const struct DPB *b,dword cluster,dword val)
//get the rest of the FAT entry when fat12of occured
{
 void *FAT=malloc(b->bytes_per_sector);
 
 if (FAT==NULL)
 {
  printf(msg[18]);
  flushCache();
  exit(1);
 }
 readSector(b->drive,FATsector(b,cluster)+1,1,FAT);
 
 if (cluster % 2==0)
  val+=((((byte*)FAT)[0] & 0x0f) << 8); //even Cluster:  bc 0a 00 
 else
  val+=(((byte*)FAT)[0] << 4); //odd Cluster: 00 c0 ab 
  
 free(FAT);
 fat12of=0;
 return val;
}

void setFAT12valX(const struct DPB *b,dword cluster,dword val)
{
 void *FAT=malloc(b->bytes_per_sector);
 dword sector;
 
 if (FAT==NULL)
 {
  printf(msg[19],'A'+b->drive,cluster);
  flushCache();
  exit(1);
 }                                                 
 sector=FATsector(b,cluster)+1;                   
 readSector(b->drive,sector,1,FAT);
 
 if (cluster % 2==0)
 {
  ((byte*)FAT)[0]&=0xf0;
  ((byte*)FAT)[0]|=(byte)((val & 0x0f00) >> 8);
 }
 else
 {
  ((byte*)FAT)[0]=(byte)((val & 0xff0) >> 4); 
 }
 for (int fats=0;fats<b->no_fats;fats++)
  writeSector(b->drive,sector+fats*b->sectors_per_fat,1,FAT,W_FAT);     //update all fats
 
 free(FAT);
 fat12of=0;
}

dword getNextAvailClusterNo(struct DPB *b) //use 1st fat
{
 void* FAT=NULL;
 dword sector,current_sector;
 dword total,val; 
 
 FAT=(word*)malloc(b->bytes_per_sector);
 if (FAT==NULL)
 {
  printf(msg[20]);
  flushCache();
  exit(1);
 }
 
 if ((b->fat_type==FAT16) || (b->fat_type==FAT12) || (b->fat_type==FAT32))
 {           
  do
  {
   current_sector=0; //invalid value so 1st time sector is read always
   for (total=b->next_cluster;total<b->no_clusters;total++)
   {
    sector=FATsector(b,total);
    if (sector!=current_sector)
    {
     readSector(b->drive,sector,1,FAT);
     current_sector=sector;
    }
    val=getFATval(b,FAT,total);
    if (fat12of==1) val=getFAT12valX(b,total,val);
    if ((val & FAT_MASK)==0)
    {
     free(FAT);
     b->next_cluster=total;
     if (total==b->no_clusters-1) b->next_cluster=2;
     return total;
    }
   }     
   if (b->next_cluster==2) break;
   else b->next_cluster=2;
  } while (1==1); //if not found the 1st time, try once from beginning of disk.
  free(FAT);
 }
 else 
 {
  free(FAT);
  printf(msg[21]);
  flushCache();
  exit(1);
 }
 return 0;
}

dword firstSectorOfCluster(const struct DPB *b,dword cluster)
{
 if (cluster==0) return b->first_root_sector; //root
 if (b->CD==1) return cluster;
 return b->first_data_sector+(dword)b->sectors_per_cluster*(cluster-2);
}

int getNextCluster(const struct DPB *b,struct FATfile *f)
//search for next FAT entry starting with f
//if f is the last cluster in a chain or free, f is unchanged and return is 1
{
 word *FAT;
 dword FATval;
 dword sector;
 
 if ((b->fat_type==FAT16) || (b->fat_type==FAT12) || (b->fat_type==FAT32))
 {
  FAT=(word*)malloc(b->bytes_per_sector);
  if (FAT==NULL)
  {
   printf(msg[22]);
   flushCache();
   exit(1);
  }
  sector=FATsector(b,f->cluster);
  readSector(b->drive,sector,1,FAT);
  FATval=getFATval(b,FAT,f->cluster);
  if (fat12of==1) FATval=getFAT12valX(b,f->cluster,FATval);
  FATval &= FAT_MASK;
  free(FAT);
  if (((b->fat_type==FAT16) && (FATval>=EOF16)) ||  //EOF?
      ((b->fat_type==FAT12) && (FATval>=EOF12 )) ||
      ((b->fat_type==FAT32) && (FATval>=EOF32)))
  {
   return 1;
  }
  else
  {
   if (FATval==0) {
    return 1; //a hole in a chain or no chain
   }
   f->cluster=FATval; //continue in chain
   f->sector=firstSectorOfCluster(b,FATval);
   return 0;
  }
 } // if
 else
 {
  printf(msg[23]);
  flushCache();
  exit(1);
 }
 return 0;
}

char* shortname(const struct longdirentry* d) //returned string is static!!
{
 static char s[13];
 int i,j;
 for (i=0;(d->dosname[i]!=' ') && (i<8);i++) s[i]=d->dosname[i];
 if (d->dosext[0]!=' ')
 {
  s[i]='.';
  i++;
  for (j=0;(d->dosext[j]!=' ') && (j<3);j++) { s[i]=d->dosext[j];i++;}
 }
 s[i]=0;
 return s;
}

int match(const char *name,const char *mask) //exact match
{
 if (!strcmp("*",mask)) return 1;
 return !strcmpi(name,mask);
}

int WildComp(const char *I,const char *W) //match with wildcard search
{
 char subs[255],WildS[255],IstS[255];
 unsigned char i, j, l, p;
 char *ps;
 strcpy(WildS,W);
 strcpy(IstS,I);
 _strupr(IstS);
 _strupr(WildS);
 i = 0;
 j = 0;
while (i<=strlen(WildS))
{
 if (WildS[i]=='*')
 {
   if (i == strlen(WildS)) return 1; //found
   else // we need to synchronize
   { 
     l = i+1;
     while ((l < strlen(WildS)) && (WildS[l+1] != '*')) l++;
     strncpy(&subs[0],&WildS[i+1], l-i);
     subs[l-i]=0;
     if (subs[0]==0) return 1;
     ps= strstr(IstS,subs);
     if (ps!=NULL)
     {     
       p = ps-IstS;
       j = p-1;
     }
     else return 0;
   }
 }
 else
 if ((WildS[i]!='?') && ((strlen(IstS) < i) || (asciiUpcase[(unsigned char)WildS[i]]!=asciiUpcase[(unsigned char)IstS[j]]))) return 0;
 i++;
 j++;
}
return (j > strlen(IstS));
}

void catUniStrg(word *dest,const word *source)
{
 int i,j;
 for (i=0;dest[i]!=0;i++); //seek to end of source string
 for (j=0;source[j]!=0;j++) dest[i+j]=source[j]; //copy
 dest[i+j]=0; //end of string mark
}

void catUniStrgn(word *dest, const word *source, const int count)
{
 int i,j,n;
 for (i=0;dest[i]!=0;i++); //seek to end of source string
 for (j=0, n=0;(n<count) && (source[j]!=0);j++, n++) dest[i+j]=source[j]; //copy
 dest[i+j]=0; //end of string mark 
}

struct longdirentry FindMaskCD(const struct DPB *b,char *mask,struct dirpointer* start,int exact)
/*for rootdir: set start->cluster=0
 *wildcards allowed, no or empty mask is *
 *exact:bit 1 set: no wildcards allowed
 *      bit 2 set: use short names
 */
{ 
 if (mask==NULL) mask="*";
 if (mask[0]=='\0') mask="*";

 dword sector=start->cluster+start->relsector;
 byte *R;
 word pos=start->entry_no;
 dword no_sectors=start->length / b->bytes_per_sector; 
 if (((start->length % b->bytes_per_sector)==0) && (no_sectors!=0)) no_sectors--;
 dword last_sector=start->cluster+no_sectors;
 struct cddir *p;
 struct longdirentry f;                  
 char cname[MAX_LFN_SIZE];
 
 f.name[0]='\0';
 if (b->CD==0) return f;
 if (start==NULL) return f; //invalid pointer
            
 R=(byte*)malloc(b->bytes_per_sector);
 if (start->entry_no>=b->bytes_per_sector)
 {
  start->entry_no=0;
  pos=0;
  start->relsector++;
  sector++;
 }
 readCDSectors(b,sector,1,R);
 p=(cddir*)&R[pos];
 if ((p->entry_length==0) && (sector<last_sector)) //continue to next sector
 {
  sector++;
  start->relsector++;
  readCDSectors(b,sector,1,R);
  start->entry_no=0;
  pos=0;
  p=(cddir*)&R[pos];
 }

 while (p->entry_length!=0)
 {
  while (p->entry_length!=0)
  {                                                                     
   getCDDirEntry(b,p,&f,start->countfile,start->countdir);
   if ((exact & 2)==0) strcpy(cname,f.name); //lfn
   else strcpy(cname,shortname(&f)); //short
   if ((((exact & 1) ==0) && WildComp(cname,mask)) || (((exact & 1) ==1) && match(cname,mask)))
   {         
    start->entry_no+=p->entry_length;
    free(R);
    return f; //found
   }
   pos+=p->entry_length;
   start->entry_no=pos;
   if (pos>=b->bytes_per_sector)
   {
    if (sector==last_sector) break; //end of dir ?
    start->entry_no=0;
    pos=0;
    start->relsector++;
    sector++;
   }
   p=(cddir*)&R[pos];
  } //while
  if (sector==last_sector) break;  //end of directory?
  sector++;
  start->relsector++;
  readCDSectors(b,sector,1,R);
  start->entry_no=0;
  pos=0;  
  p=(cddir*)&R[pos];
 }
                   
 f.name[0]='\0';
 free(R);
 return f;
}

struct longdirentry FindMask(const struct DPB *b,char *mask,struct dirpointer* start,int exact)
//for fat12/fat16-rootdir: set start->cluster=0
//for fat32-rootdir: treat like a subdir
//wildcards allowed, LFN ONLY,
//mask: null or "" mask is *
//exact:bit 1 set: no wildcards allowed
//      bit 2 set: use short names
{     
 struct longdirentry pDir; 
 struct direntry *dir;
 struct lfnentry *lfn;
 struct FATfile f;
 void *data;
 word entries_per_sector,i;     
 char longfilename[MAX_LFN_SIZE],name[MAX_LFN_SIZE],cname[MAX_LFN_SIZE];
 word *longuniname,*uniname;
 byte checksum;
 int chain_end;

 if (b->CD==1) return FindMaskCD(b,mask,start,exact); 
 pDir.name[0]=0;
 pDir.dosname[0]=0;
 pDir.dosext[0]=0;
 if (mask==NULL) mask=&"*";
 if (mask[0]==0) mask="*";
 if (start==NULL) return pDir; //ungültiger Pointer

 data=malloc(b->bytes_per_sector);
 if (data==NULL)
 {
  printf(msg[24]);
  flushCache();
  exit(1);
 }
 entries_per_sector=b->bytes_per_sector/32;
 if ((b->fat_type==FAT32) && (start->cluster==0)) start->cluster=b->first_root_cluster;
 f.cluster=start->cluster;
 
 if (f.cluster==0) //root?
 {      
  f.sector=b->first_root_sector;
 }
 else f.sector=firstSectorOfCluster(b,start->cluster);
 
 longuniname=(word*)malloc(MAX_LFN_SIZE*sizeof(word));
 uniname=(word*)malloc(MAX_LFN_SIZE*sizeof(word));
 if ((longuniname==NULL) || (uniname==NULL))
 {
  printf(msg[25]);
  free(data);
  flushCache();
  exit(1);
 }
 longfilename[0]=0;
 longuniname[0]=0;
 checksum=0;
 do
 {
  do
  {
   if (readSector(b->drive,f.sector+start->relsector,1,data))
   {         
    printf(msg[26]);
    free(longuniname);
    free(uniname);
    free(data);
    flushCache();
    exit(1);
   };
   dir=&((struct direntry*)data)[start->entry_no];
   lfn=&((struct lfnentry*)data)[start->entry_no];
   for (i=start->entry_no;i<entries_per_sector;i++)
   {            
    start->entry_no++;
    if (dir->name[0]==0) //subdir end
    {
     pDir.name[0]=0;  
     pDir.dosname[0]=0;
     pDir.dosext[0]=0;
     free(longuniname);
     free(uniname);    
     free(data);
     return pDir;
    }
    if (dir->atrib!=F_LFN)            //short file name
    {
     if (longfilename[0]!=0) strcpy(pDir.name,longfilename);
     else pDir.name[0]=0;         
     pDir.uniname[0]=0;
     if (longuniname[0]!=0) catUniStrg(pDir.uniname,longuniname);
     if (((byte)dir->name[0]!=0xe5) && ((dir->atrib & F_LABEL) == 0)) //deleted or label?
     {
      strncpy(pDir.dosname,dir->name,8);
      strncpy(pDir.dosext,dir->ext,3);                         
      pDir.checksum=makeChecksum(pDir.dosname);
      if ((checksum!=0) && (checksum!=pDir.checksum)) {
       //checksums do not match, throw long name away.
       pDir.name[0]=0;
       pDir.uniname[0]=0;
      }
      if (pDir.name[0]==0) strcpy(pDir.name,shortname(&pDir));
      if ((exact & 2)==0) strcpy(cname,pDir.name); //lfn
      else strcpy(cname,shortname(&pDir)); //short
      if ((((exact & 1) ==0) && WildComp(cname,mask)) || (((exact & 1) ==1) && match(cname,mask)))
      {
       pDir.ftype=0;
       pDir.time=dir->time;
       pDir.date=dir->date;
       pDir.start_cluster=dir->start_cluster+((dword)dir->high_cluster << 16);
       pDir.length=dir->length;
       pDir.atrib=dir->atrib;
       strncpy(pDir.reserved,dir->reserved,8);
       free(longuniname);
       free(uniname);
       free(data);
       return pDir;
      }                         
     }
     longfilename[0]=0;                     
     longuniname[0]=0;
    }
    else  //long file name
    {
     if (lfn->count!=0xe5)   // deleted?
     {
      if ((checksum!=0) && (lfn->checksum!=checksum)) {
       //wrong checksum, start over
        longfilename[0]=0;
        name[0]=0;
        uniname[0]=0;
        longuniname[0]=0;
      }
      lfnString(*lfn,&name[0],uniname);
      strcat(name,longfilename);
      strcpy(longfilename,name);
      catUniStrg(uniname,longuniname);
      longuniname[0]=0;
      catUniStrg(longuniname,uniname);
      checksum=lfn->checksum;
     }
    }
    lfn++;
    dir++;
   } //for each entry
   start->entry_no=0;
   start->relsector++;
  } while (((f.cluster!=0) && (start->relsector<b->sectors_per_cluster)) ||
          ((f.cluster==0) && (start->relsector<b->no_root_sectors)));
  if (f.cluster==0)
  {
   pDir.name[0]=0;
   pDir.uniname[0]=0;
   pDir.dosname[0]=0;
   pDir.dosext[0]=0; 
   free(data);
   free(longuniname);
   free(uniname);
   return pDir; //root:not found  
  }
  start->relsector=0;
  chain_end=getNextCluster(b,&f);
  start->cluster=f.cluster;   
 } while (chain_end!=1);
 pDir.name[0]=0;
 pDir.uniname[0]=0;
 pDir.dosname[0]=0;
 pDir.dosext[0]=0;
 free(data);
 free(longuniname);
 free(uniname);
 return pDir; //not found
}

int updateFSInfo(const struct DPB *b,signed long allocated_clusters,dword last_cluster)
//FAT32 only
//allocated_clusters: relative number of newly allocated clusters, negative values are freed clusters
{
 struct BIGFATBOOTFSINFO *fsi;           
 if (b->fat_type!=FAT32) return 1;
 if (allocated_clusters==0) return 0;
 fsi=(struct BIGFATBOOTFSINFO*)malloc(b->bytes_per_sector);
 if (fsi==NULL)
 {
  printf(msg[27]);
  flushCache();
  exit(1);
 }
 readSector(b->drive,b->fs_info_sector,1,fsi);
 if ((fsi->start_signature==0x41615252) && (fsi->signature==0x61417272) && (fsi->end_signature==0xAA550000)) {
  if (fsi->no_free_clusters!=-1)
  {
   fsi->no_free_clusters-=allocated_clusters;
  }
  if (last_cluster!=0) fsi->next_free_cluster=last_cluster;
  writeSector(b->drive,b->fs_info_sector,1,fsi,W_OTHER);
 }
 free(fsi);
 return 0;
}

dword extendFile(struct DPB *b,struct FATfile* chain,dword count,int type)
//We implement a stupid disk cache to minimize (slow) direct disk access
//allocate additional count clusters in all fat copies for this file
//type=W_DIR: erase new clusters with 00
//return number of finally allocated clusters
{
 struct FATfile *last;
 dword clusterNo,recent,i;
 dword sector,fat_in_store,val;
 word *FAT,fats;
 word j;
 void *EMPTY=NULL;
 int dirty, wrapped, found;

 if ((b->fat_type!=FAT16) && (b->fat_type!=FAT12) && (b->fat_type!=FAT32))
 {            
  printf(msg[28]);
  flushCache();
  exit(1);
 }
 
 while (!getNextCluster(b,chain)) ; //seek end of chain
 last=chain;
 FAT=(word*)malloc(b->bytes_per_sector);
 EMPTY=malloc(b->bytes_per_sector);
 if ((FAT==NULL) || (EMPTY==NULL))
 {
  printf(msg[29]);
  flushCache();
  exit(1);
 }
 memset(EMPTY,0,b->bytes_per_sector);
 recent=0;
 fat_in_store=0;
 clusterNo=b->next_cluster-1;
 dirty=0;       
 wrapped=0;
 for (i=0;i<count;i++)
 {
  //clusterNo=next free cluster on drive
  int wrap=0;
  do
  {  
   do
   {
    clusterNo++;
    sector=FATsector(b,clusterNo);
    if (sector!=fat_in_store)
    {
     if (dirty==1) //WB dirty data
     {
      for (fats=0;fats<b->no_fats;fats++)
       writeSector(b->drive,fat_in_store+fats*b->sectors_per_fat,1,FAT,W_FAT); //update all fats
      dirty=0;
     }
     readSector(b->drive,sector,1,FAT); //get next fat sector
     fat_in_store=sector;
    }                                
    val=getFATval(b,FAT,clusterNo);
    found = ((val & FAT_MASK)==0);
    if (fat12of==1) val=getFAT12valX(b,clusterNo,val);
   } while ((!found) && (clusterNo<b->no_clusters));
   if (!found) {
    if (wrapped != 0) {
     wrap = 0;
     break;   
    } else {
     wrapped = 1;
     clusterNo = 2;
     wrap = 1;
    }
   }
  } while (wrap!=0); //try again from beginning if optimized search did not succeed
  
  if (!found) {  
   if (dirty==1) //WB dirty data
   {
    for (fats=0;fats<b->no_fats;fats++)
     writeSector(b->drive,fat_in_store+fats*b->sectors_per_fat,1,FAT,W_FAT); //update all fats
    dirty=0;
   }
   free(EMPTY);
   free(FAT);
   updateFSInfo(b,i,recent);
   return i;  //disk full
  }      
  //else allocate it                 
  b->next_cluster=clusterNo;
  val=getFATval(b,FAT,clusterNo);
  if (fat12of==1) val=getFAT12valX(b,clusterNo,val);
  setFATval(b,FAT,clusterNo,(val & INV_FAT_MASK) | EOF32); //reserved bits are preserved
  if (fat12of==1) setFAT12valX(b,clusterNo,(val & INV_FAT_MASK)|EOF32);
  recent=clusterNo;
  dirty=1;
  
  //link to previous
  if (last->cluster!=clusterNo)  //only if there IS one to link
  {
   sector=FATsector(b,last->cluster);
   //cache
   if (sector!=fat_in_store)
   {
    if (dirty==1) //WB dirty data
    {
     for (fats=0;fats<b->no_fats;fats++)
      writeSector(b->drive,fat_in_store+fats*b->sectors_per_fat,1,FAT,W_FAT); //update all fats
     dirty=0;
    }
    readSector(b->drive,sector,1,FAT); //fetch new data
    fat_in_store=sector;
   }      
   val=getFATval(b,FAT,last->cluster);
   if (fat12of==1) val=getFAT12valX(b,last->cluster,val);
   setFATval(b,FAT,last->cluster,(val & INV_FAT_MASK) | clusterNo); // link
   if (fat12of==1) setFAT12valX(b,last->cluster,(val & INV_FAT_MASK) | clusterNo);
   dirty=1;
  }
  last->cluster=clusterNo;
  last->sector=firstSectorOfCluster(b,clusterNo);
  //when appending a dir -> fill with zeros 
  if (type==W_DIR)
  {                                     
   for (j=0;j<b->sectors_per_cluster;j++)
    writeSector(b->drive,last->sector+j,1,EMPTY,type); 
  }
 }           
 if (dirty==1) //WB dirty data
 {
  for (fats=0;fats<b->no_fats;fats++)
   writeSector(b->drive,fat_in_store+fats*b->sectors_per_fat,1,FAT,W_FAT); //update all fats
  dirty=0;
 }
 free(FAT);
 free(EMPTY);
 recent = getNextAvailClusterNo(b);  //update to current
 updateFSInfo(b,i,recent);
 return i;
}

char* seekPath(struct DPB *b,char *path,char * longpath,longdirentry &pDir,int exact) 
//return the short path and direntry of object given by path
//user must free the space occupied by the returned string
//path: NULL: returns current directory; can handle drive letters
//exact bit 1==0: you can use wildcards, first occurence is passed back
//      bit 2==1: assume short names in path
{
 char *spath;
 char part[MAX_LFN_SIZE],current[MAX_LFN_SIZE],s[MAX_LFN_SIZE];
 word z,k,z1;          
 struct longdirentry f;
 struct dirpointer dp;
 
 if (path==NULL)
 {
  _getdcwd(b->drive+1,current,MAX_PATH_SIZE);
  if (current[strlen(current)-1]!='\\') strcat(current,"\\");
  if ((b->CD==1) && (b->cdfs==CD_JOLIET))
  {
   short2longCD(b,current,s);
   return seekPath(b,s,longpath,pDir,1);
  }
  else

  return seekPath(b,current,longpath,pDir,3); 
 }

 spath=(char*)malloc(MAX_PATH_SIZE);
 if (spath==NULL) return NULL;
 spath[0]='\0';
 longpath[0]='\0';
 z=0;
 if (path[1]==':')
 {
  strncat(spath,path,2);
  strncat(longpath,path,2);
  z=2;
 }
 strcat(spath,"\\");
 strcat(longpath,"\\");
 if (path[z]=='\\') // from root
 {
  z++;
  current[0]=0;
 }     
 else //from current directory
 {
  _getdcwd(b->drive+1,current,MAX_PATH_SIZE);
  if (current[strlen(current)-1]!='\\') strcat(current,"\\");
  if ((b->CD==1) && (b->cdfs==CD_JOLIET))
  {
   pDir=short2longCD(b,current,s);
  }
  else
   free(seekPath(b,current,longpath,pDir,3)); 
  dp.cluster=pDir.start_cluster;
  dp.relsector=0;
  dp.entry_no=0; 
  dp.countfile=COUNTFILE_START;
  dp.countdir=COUNTDIR_START; 
  dp.length=dp.length;
  z1=strlen(current);
  strcpy(spath,current);
  if ((b->CD==1) && (b->cdfs==CD_JOLIET)) strcpy(longpath,s);
  strcat(current,&path[z]);
  path=current;
  z=z1;
 }
 part[0]=0;
 k=strpos(&path[z],MAX_PATH_SIZE,'\\');
 strncat(part,&path[z],k); //without backslash
 z=z+k;     
 f.name[0]=0;
 //FAT drive
 if (current[0]==0) 
 {
  dp.cluster=b->first_root_cluster;
  dp.relsector=0;
  dp.entry_no=0;
  if (b->CD==1)
  {
   dp.countfile=COUNTFILE_START;
   dp.countdir=COUNTDIR_START;
   dp.length=b->no_root_entries;
  }
 }
 while (part[0]!=0)
 {
  f=FindMask(b,part,&dp,exact);
  if (f.name[0]==0) //found?
  {
   free(spath);
   return NULL; //not found
  }      
  pDir=f;
  strcat(spath,shortname(&f));
  strcat(longpath,f.name);
  if (path[z]=='\\')
  {
   strcat(spath,"\\");
   strcat(longpath,"\\");
   z++;
  }
  if ((path[z]!=0) && ((f.atrib & F_DIR) ==F_DIR)) //subdir?
  {
   dp.cluster=f.start_cluster;
   dp.relsector=0;
   dp.entry_no=0;
   dp.countfile=COUNTFILE_START;
   dp.countdir=COUNTDIR_START; 
   dp.length=f.length;
  }
  else
  {
   return strupr(spath); //end
  }
  part[0]=0;
  k=strpos(&path[z],MAX_PATH_SIZE,'\\');
  strncat(part,&path[z],k); //without backslash 
  z=z+k;
 } //while
 if ((f.name[0]==0) && (current[0]==0)) //it is the root dir
 {
  pDir.name[0]=0;   
  pDir.uniname[0]=0;
  pDir.dosname[0]=0;
  pDir.dosext[0]=0;
  pDir.atrib=F_DIR;
  pDir.start_cluster=dp.cluster;
  if (b->CD==1)
   pDir.length=dp.length;
  else
   pDir.length=0;
 }
 return strupr(spath); 
}

void ChangeDir(struct DPB *b, char *path)
{
 char *shortpath=NULL;
 char longpath[MAX_PATH_SIZE];
 struct longdirentry dir;
 int l;
    
 shortpath=seekPath(b,path,longpath,dir,0);
 if ((b->CD==1) && (b->cdfs==CD_JOLIET)) long2shortCD(b,longpath,shortpath);
 l=strlen(shortpath)-1;
 if ((l>2) && (shortpath[l]=='\\')) shortpath[l]='\0';
 _chdrive(b->drive+1);
 _chdir(shortpath);
 free(shortpath);
}

int drive_of_path(char *path)    //0=A, 1=B, ...
{
 unsigned drive;
 _dos_getdrive(&drive);
 if (path==NULL) return drive-1;
 if (path[1]==':')
 {
  
  return (toupper(path[0])-'A');
 }
 else return drive-1;
}

void deleteDirentry(struct DPB *b,struct longdirentry *dir,struct dirpointer *dp,struct longdirentry *f)
//dir: the directory to delete the entry from
//dp: position inside directory
//f: the direntry (name) to delete
{
 struct direntry *d;
 struct lfnentry *l;
 struct FATfile fatf;
 dword  prev;
 void *data;      
 word i;   
 byte count,no_lfn;
 
 data=malloc(b->bytes_per_sector);
 if (data==NULL)
 {
  printf(msg[30]);
  flushCache();
  exit(1);
 }
 d=(struct direntry*)data;
 l=(struct lfnentry*)data;
 if (f->uniname[0]==0) //no lfn
   no_lfn=0;
 else //has lfn
 {
  no_lfn=strlen(f->name)/CHR_PER_LFN; //how many lfns are used?
  if (strlen(f->name)%CHR_PER_LFN!=0) no_lfn++;
 }
 count=no_lfn;
 if (no_lfn>dp->entry_no) //some are in a different sector
 {        
  if (dp->relsector==0)  //lfn in previous cluster
  {                                
   if (dir->start_cluster==0)
   {
    printf(msg[31]);
    free(data);
    flushCache();
    exit(1);
   }
   fatf.cluster=dir->start_cluster;
   fatf.sector=firstSectorOfCluster(b,fatf.cluster);
   do //seek the previous cluster of the dir
   {
    prev=fatf.cluster; //this is the previous cluster
    if (getNextCluster(b,&fatf)==1)
    {
     printf(msg[32]);
     free(data);
     flushCache();
     exit(1);
    }
   } while (fatf.cluster!=dp->cluster);
   
   readSector(b->drive,firstSectorOfCluster(b,prev+1)-1,1,data); //read the last sector of the previous cluster
   for (i=b->dir_entries_per_sector-(no_lfn- dp->entry_no);i<b->dir_entries_per_sector;i++)
   {
    if ((i!=dp->entry_no) && ((l[i].count&0x3f)!=count)) //security check
    {
     printf(msg[33]);
     free(data);
     flushCache();
     exit(1);
    }
    l[i].count=0xe5; //mark as deleted
    count--;
   }
   writeSector(b->drive,firstSectorOfCluster(b,prev+1)-1,1,data,W_DIR);
   no_lfn=(byte)dp->entry_no;
  }   
  else //in the same cluster
  {
   readSector(b->drive,dp->relsector-1+firstSectorOfCluster(b,dp->cluster),1,data);
   for (i=b->dir_entries_per_sector-(no_lfn- dp->entry_no);i<b->dir_entries_per_sector;i++)
   {
    if ((i!=dp->entry_no) && ((l[i].count&0x3f)!=count)) //security check
    {
     printf(msg[34]);
     free(data);
     flushCache();
     exit(1);
    }
    l[i].count=0xe5; //mark as deleted
    count--;
   }
   writeSector(b->drive,dp->relsector-1+firstSectorOfCluster(b,dp->cluster),1,data,W_DIR);
   no_lfn=(byte)dp->entry_no;
  }
 }
 //rest is in the same sector
 
 readSector(b->drive,dp->relsector+firstSectorOfCluster(b,dp->cluster),1,data);
 for (i=dp->entry_no-no_lfn;i<=dp->entry_no;i++)
 {
  if ((i!=dp->entry_no) && ((l[i].count&0x3f)!=count)) //security check
  {
   printf(msg[35]);
   free(data);
   flushCache();
   exit(1);
  }
  l[i].count=0xe5; //mark as deleted
  count--;
 }  
 writeSector(b->drive,dp->relsector+firstSectorOfCluster(b,dp->cluster),1,data,W_DIR);
 free(data);
}

int freeClusterChain(struct DPB *b,struct longdirentry *f)
{
 void* FAT;
 dword sector,cluster,temp,count;
 word fats;
 
 FAT=malloc(b->bytes_per_sector);
 if (FAT==NULL)
 {
  printf(msg[36]);
  flushCache();
  exit(1);
 }
 count=0;
 cluster=f->start_cluster;
 if (cluster==0)
 {
  free(FAT);
  return 1;
 }
 if ((b->fat_type==FAT16) || (b->fat_type==FAT12) || (b->fat_type==FAT32))
 {           
  do
  {       
   sector=FATsector(b,cluster);
   readSector(b->drive,sector,1,FAT);
   temp=getFATval(b,FAT,cluster); //save the entry value
   if (fat12of==1) temp=getFAT12valX(b,cluster,temp);
   if ((temp & FAT_MASK)==0)
   {
    printf(msg[37]);
    free(FAT);
    flushCache();
    exit(1);
   }
   setFATval(b,FAT,cluster,0 | (temp & INV_FAT_MASK)); // free the cluster preserving reserved bits     
   if (fat12of==1) setFAT12valX(b,cluster,0|(temp & INV_FAT_MASK));
   count++;
   for (fats=0;fats<b->no_fats;fats++)
    writeSector(b->drive,sector+fats*b->sectors_per_fat,1,FAT,W_FAT);     //update all fats
   cluster=(temp & FAT_MASK); //next one   
  } while (((b->fat_type==FAT12) && (cluster<EOF12)) ||
           ((b->fat_type==FAT16) && (cluster<EOF16)) ||
           ((b->fat_type==FAT32) && (cluster<EOF32))); //until EOF
  if (b->fat_type==FAT32) updateFSInfo(b,-(signed long)count,0);         
 }
 else
 {
  printf(msg[38]);
  free(FAT);
  flushCache();
  exit(1);
 }
 free(FAT);
 return 0;
}

int recycleDirentries(struct DPB *b,struct dirpointer *start,byte count)
//look for count free Direntries and set dp according, we do not extend a dir
//return 1 if not found. return 2 if not found but empty entries at end of cluster.
{
 void *data;
 struct direntry *dir; 
 struct FATfile f;
 struct dirpointer dp;
 int eof;
 word i,entries_per_sector;
 byte n;
 
 dp=*start;
 data=(direntry*)malloc(b->bytes_per_sector);
 if (data==NULL)
 {
  printf(msg[39]);
  flushCache();
  exit(1);
 }
 entries_per_sector=b->bytes_per_sector/32;
 if ((b->fat_type==FAT32) && (dp.cluster==0)) dp.cluster=b->first_root_cluster;
 f.cluster=dp.cluster;
 
 if (f.cluster==0) //root?
 {      
  f.sector=b->first_root_sector;
 }
 else f.sector=firstSectorOfCluster(b,dp.cluster);
 
 n=count;
 do
 {
  do
  {
   if (readSector(b->drive,f.sector+dp.relsector,1,data))
   {         
    printf(msg[40]);
    free(data);
    flushCache();
    exit(1);
   };
   dir=&((struct direntry*)data)[dp.entry_no];
   for (i=dp.entry_no;i<entries_per_sector;i++)
   {            
    if ((dir->name[0]==0)              //subdir end or
        || ((byte)dir->name[0]==0xe5)) //deleted
    {
     if (n==count) *start=dp;
     n--;
    }
    else n=count; 
    if (n==0)
    {             
     free(data);
     return 0; //found!
    }
    dp.entry_no++;
    dir++;
   } //for each entry
   dp.entry_no=0;
   dp.relsector++;
  } while (((f.cluster!=0) && (dp.relsector<b->sectors_per_cluster)) ||
          ((f.cluster==0) && (dp.relsector<b->no_root_sectors)));
  if (f.cluster==0)
  {
   free(data);
   return 1; //root:not found  
  }
  eof=getNextCluster(b,&f);
  if (eof==1)
  {
   free(data);
   if (n!=count) return 2; //some free entries at the end but not enough
   else return 1; //not enough free space entries found
  }
  dp.relsector=0;
  dp.cluster=f.cluster;   
 } while (eof!=1);
 //--this code is never executed, unless you insert a break command above
  free(data);
  if (n!=count) return 2; //some free entries at the end but not enough
  else return 1; //not enough free space entries found
 //--
}
 
 
/**
 * Sets the date field of a directory entry
 * @param date must be in format mm/dd/yy (like _strdate())
 */
void setDate(char* date, struct longdirentry *f) {
 word year=((date[6]-'0')*10+(date[7]-'0'));        
 if (year<80) year=(2000+year)-1980;
 else year=(1900+year)-1980;
 f->date=(year << 9) | (((date[0]-'0')*10+(date[1]-'0')) << 5) | ((date[3]-'0')*10+(date[4]-'0'));
}
 
/**
 * Sets the time field of a directory entry
 * @param time must be in format hh:mm:ss (like _strtime())
 */
void setTime(char* time, struct longdirentry *f) {
 f->time=(((time[0]-'0')*10+(time[1]-'0')) << 11) | (((time[3]-'0')*10+(time[4]-'0')) << 5) | 
        (((time[6]-'0')*10+(time[7]-'0')) >> 1);
}
 
/**
 * Sets the date and time field of a directory entry to the current time/date
 */
void setCurrentDateTime(struct longdirentry *f) {
 char time[10];
 char date[10];
 _strtime(time);
 _strdate(date);
 setDate(date, f);
 setTime(time, f);
}
 
/**
 * Creates a new directory name in the directory parent on drive b
 * This function does not check if the directory already exists!
 * @param parent The parent directory of the new directory
 * @param name The long name of the new directory
 * @param f [out] The direntry that was created
 * @returns 0 ok
 *          1 Disk full
 *          2 Create failed. 1 cluster lost.
 *          3 Initialize failed. New directory corrupt.
 */
int makeDirectory(struct DPB *b, struct longdirentry *parent, char *name, longdirentry *f) {   
 strcpy(f->name,name);
 f->uniname[0]=0;
 ascii2uni((unsigned char*) name, (unsigned int*) f->uniname);
 f->dosname[0]=0;
 f->dosext[0]=0; 
 f->atrib=F_DIR;
 f->ftype=0;
 f->checksum=0;
 setCurrentDateTime(f);
 f->length=0;
 strnset(f->reserved,0,8);   
 return makeDirectory(b, parent, f);
}

/**
 * Creates a new directory name in the directory parent on drive b
 * This function does not check if the directory already exists!
 * @param parent The parent directory of the new directory
 * @param f The directory entry to create with all field correctly set
 *          The start_cluster field is set by the function
 * @returns 0 ok
 *          1 Disk full
 *          2 Create failed. 1 cluster lost.
 *          3 Initialize failed. New directory corrupt.
 */
int makeDirectory(struct DPB *b, struct longdirentry *parent, longdirentry *f) {   
 struct FATfile fat;

 fat.cluster=getNextAvailClusterNo(b);
 fat.sector=firstSectorOfCluster(b,fat.cluster);
 f->start_cluster=fat.cluster; 
 
 if (extendFile(b,&fat,1,W_DIR)!=1)
 {
  return 1;
 }
 
 if (insertDirentry(b, parent, f, PROTECT_ALL)!=0)
 {
  return 2;
 } 
 
 if (initDirectory(b, parent, f)!=0) {
  return 3;
 }         
 return 0;
}

/**
 * Creates the . and .. entries in a new directory
 * @param b The drive on which dir is located
 * @param parent The parent directory of dir
 * @param dir The directory to be initialized
 * @returns 0 if ok, 1 on error
 */
int initDirectory(struct DPB *b, struct longdirentry *parent, struct longdirentry *dir) {
 struct longdirentry g;
 
 g.name[0]='.';
 g.name[1]=0;
 g.uniname[0]=0;         
 for (int i=0;i<11;i++) g.dosname[i]=' ';
 g.dosname[0]='.';
 g.atrib=F_DIR;
 g.ftype=0;
 g.checksum=0;
 setCurrentDateTime(&g);
 g.start_cluster=dir->start_cluster;
 g.length=0;
 for (i=0;i<8;i++) g.reserved[i]=0;
 if (insertDirentryP(b,dir,&g,PROTECT_ALL)!=0)
 {
  return 1;
 }
 g.name[1]='.';
 g.name[2]=0;
 g.dosname[1]='.';
 if (parent->start_cluster!=b->first_root_cluster) {
  g.start_cluster=parent->start_cluster;
 }
 else {
  g.start_cluster=0; //points to root
 }
 if (insertDirentryP(b,dir,&g,PROTECT_ALL)!=0)
 {
  return 1;
 }         
 return 0;
}        


/**
 * Appends new direntry on a directory. We recycle deleted direntries.
 * Prevents filenames that end with a dot. 
 * @param b The drive on which dir is located
 * @param dir The directory to create the new entry in
 * @param f The new entry
 * @param protection PROTECT_ALL: no files are overwritten
 *                   PROTECT_RO: read-only files are not overwritten, normal files are
 *                   PROTECT_SYS: system and hidden files are not overwritten, r-o and normal files are
 *                   PROTECT_NO: all files are overwritten 
 * @returns 0 if successfull, !=0 otherwise
 *          1 if file is not writable (i.e. a directory, label)
 *          2 if file is system or hidden
 *          3 if file is read only
 *          4 if file is normal
 */
int insertDirentry(struct DPB *b,struct longdirentry *dir,struct longdirentry *f, int protection) {
 //strip trailing dot(s)
 while (f->name[strlen(f->name)-1] == '.') {
  f->name[strlen(f->name)-1] = 0;
 }

 return insertDirentryP(b, dir, f, protection);
}


/**
 * Private function. Do not use directly.
 * @see insertDirentry
 */           
int insertDirentryP(struct DPB *b,struct longdirentry *dir,struct longdirentry *f, int protection)
{
 struct dirpointer *dp;
 struct longdirentry F;
 struct direntry *pDir;
 struct lfnentry *pLfn;
 byte no_lfn,lfn_len,lfn_pos,lfn_cnt;
 int finished=0;
 int append;
 int excludeflags;
 dword sector,sector2;
 struct FATfile fatf;
                
 dp=new dirpointer; 
 if (dp==NULL)
 {
  printf(msg[41]);
  flushCache();
  exit(1);
 }
 dp->cluster=dir->start_cluster;
 dp->relsector=0;
 dp->entry_no=0;
 dp->countfile=COUNTFILE_START;
 dp->countdir=COUNTDIR_START;
 dp->length=dir->length;
 F=FindMask(b,f->name,dp,1);
 if (F.name[0]!=0) //overwrite Files
 {      
  if (protection==PROTECT_ALL) {
   delete dp;
   if ((F.atrib & F_READ_ONLY) !=0) return 3;
   if ((F.atrib & (F_HIDDEN|F_SYSTEM)) !=0) return 2;
   return 4;
  }
  excludeflags=(F_DIR|F_LABEL);
  if (protection==PROTECT_RO) excludeflags|=F_READ_ONLY;
  if (protection==PROTECT_SYS) excludeflags|=F_READ_ONLY|F_HIDDEN|F_SYSTEM;
  if ((F.atrib & excludeflags) !=0) // do not overwrite protected files
  { 
   delete dp;
   if ((F.atrib & F_READ_ONLY) !=0) return 3;
   return 2;
  }
  freeClusterChain(b,&F);
  dp->entry_no--;
  deleteDirentry(b,dir,dp,&F);
  F=FindMask(b,f->name,dp,1); //seek to end of dir
  if (F.name[0]!=0) //another equal entry? strange!
  {
   printf(msg[42]);
   delete dp;
   flushCache();
   exit(1);
  }
 } 
 if ((strncmp(f->dosname,".          ",11)!=0) && (strncmp(f->dosname,"..         ",11)!=0)) {
  makeAlias(b,dir,f);
 }
 if (f->uniname[0]==0) //no lfn
   no_lfn=0;
 else //has lfn
 {
  lfn_len=strlen(f->name);
  no_lfn=lfn_len/CHR_PER_LFN; //how many lfns are used?
  if (strlen(f->name)%CHR_PER_LFN!=0) no_lfn++;
 }          

 dp->cluster=dir->start_cluster;
 dp->relsector=0;
 dp->entry_no=0;
 dp->countdir=COUNTDIR_START;
 dp->countfile=COUNTFILE_START;
 dp->length=dir->length;
 append=recycleDirentries(b,dp,no_lfn+1);
 if ((append!=0) && (dp->cluster==0))
 {
  printf(msg[43],b->no_root_entries);
  delete dp;
  flushCache();
  exit(1);
 }      
 //dp points to beginning of free space
 pDir=(struct direntry*)malloc(b->bytes_per_sector);
 pLfn=(struct lfnentry*)pDir;
 if (pDir==NULL)
 {
  printf(msg[44]);
  delete dp;
  flushCache();
  exit(1);
 }
 lfn_cnt=no_lfn; //count them
 sector=0;  //invalid
 do
 {
  if ((dp->entry_no>=b->dir_entries_per_sector) || (append!=0)) //sector boundary?
  {
   if (append!=2) //remember start point?
   {
    dp->entry_no=0;
    dp->relsector++;
   }
   if (((dp->relsector>=b->sectors_per_cluster) && (dp->cluster!=0)) || (append!=0))//cluster boundary?
   {    
    if (append!=2) dp->relsector=0; 
    fatf.cluster=dp->cluster;
    fatf.sector=firstSectorOfCluster(b,dp->cluster);
    if ((append!=0) || (getNextCluster(b,&fatf)==1))        //if there is no next cluster
     if (extendFile(b,&fatf,1,W_DIR)==0)   //allocate one
     {
      printf(msg[45]);
      delete dp;
      free(pDir);
      flushCache();
      exit(1);
     }
    if (append!=2) dp->cluster=fatf.cluster;
    append=0;
   }
  }  
  sector2=firstSectorOfCluster(b,dp->cluster)+dp->relsector;
  if (sector!=sector2) //still in the same sector?               
  {
   if (sector!=0)
   {                    
    writeSector(b->drive,sector,1,pDir,W_DIR);
   }
   readSector(b->drive,sector2,1,pDir);
   sector=sector2;
  }
   if (lfn_cnt!=0) //write lfn
   {
    pLfn[dp->entry_no].count=lfn_cnt;
    if (lfn_cnt==no_lfn)    //first entry is special
    {
     pLfn[dp->entry_no].count|=0x40; //mark first entry
     int tail=lfn_len%(CHR_PER_LFN);
     if (tail==0) tail=CHR_PER_LFN;
     lfn_pos=lfn_len-tail;
     for (int s=1;s<CHR_PER_LFN-tail;s++) f->uniname[s+lfn_len]=0xffff; //pad with ff AFTER 0
     unincpy(pLfn[dp->entry_no].name3,&f->uniname[lfn_pos+11],2);
     unincpy(pLfn[dp->entry_no].name2,&f->uniname[lfn_pos+5],6);
     unincpy(pLfn[dp->entry_no].name1,&f->uniname[lfn_pos],5);
    }
    else
    {
     if (lfn_len>=CHR_PER_LFN) lfn_pos=lfn_len-CHR_PER_LFN; //first letter to store
     else lfn_pos=0;
     unincpy(pLfn[dp->entry_no].name3,&f->uniname[lfn_pos+11],2);
     unincpy(pLfn[dp->entry_no].name2,&f->uniname[lfn_pos+5],6);
     unincpy(pLfn[dp->entry_no].name1,&f->uniname[lfn_pos],5);
    }
    pLfn[dp->entry_no].atrib=F_LFN;
    pLfn[dp->entry_no].ftype=0;
    pLfn[dp->entry_no].checksum=makeChecksum(f->dosname);
    pLfn[dp->entry_no].reserved=0x0000;
    lfn_len=lfn_pos; 
    lfn_cnt--;
   }
   else //write shortentry
   {
    strncpy(pDir[dp->entry_no].name,f->dosname,8);
    strncpy(pDir[dp->entry_no].ext,f->dosext,3);
    pDir[dp->entry_no].atrib=f->atrib;
    strncpy(pDir[dp->entry_no].reserved,f->reserved,8);
    pDir[dp->entry_no].high_cluster=(word)((f->start_cluster & 0xffff0000) >> 16);
    pDir[dp->entry_no].time=f->time;
    pDir[dp->entry_no].date=f->date;
    pDir[dp->entry_no].start_cluster=(word)(f->start_cluster & 0x0000ffff);
    pDir[dp->entry_no].length=f->length;
    finished=1;     
    writeSector(b->drive,sector,1,pDir,W_DIR);
   } 
  dp->entry_no++;
 } while (finished!=1);
 delete dp;
 free(pDir);
 return 0;
}

int strlast(char *s,char c) //find last occurence of c in s, return position of c or 0xffff
{
 int l,j;          
 l=0xffff;
 for (j=0;s[j]!=0;j++)
  if (s[j]==c) l=j;
 return l;
}
        
int winChar(char c)
//1: if is a win allowed character , 0 else
{
 if ((c==43) || (c==44) || (c==59) || (c==61) ||
     (c==91) || (c==93)) return 1;
 else return 0;    
}        
        
int dosChar(unsigned char c)
{
  if ((c<128)                    //graphical
  && ((c>123) || (c<94))        //^-a-z-{
  && ((c>90) || (c<64))         //@-Z
  && ((c>57) || (c<48))         //0-9
  && (c!=33)                    //!
  && ((c<35) || (c>41))         //#-)
  && (c!=45)                    //-
  && ((c<125) || (c>126)))       //} and ~
   return 0;                    //invalid char
  else
   return 1;   
}

int dosStr(char *s)
{
 char *c;
 for (c=s;*c!=0;c++)
 {
  if ((dosChar(*c)==0) && (*c!='.')) return 0;
 }
 return 1; //all valid characters
}

//convert Windows ANSI to Codepage 850
/*
void convertInput(char *s)

{     
 char *c;
 for (c=s;*c!=0;c++) 
  *c=win850[(unsigned char)*c];
} 
*/
  
  
void makeAlias(struct DPB *b, struct longdirentry *dir, struct longdirentry *f)
{    
 unsigned int dot,len,i,j,tilda;
 char number[7],mask[13];
 struct dirpointer dp;
 struct longdirentry r;
                
 dp.cluster=dir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0;           
 dp.countfile=COUNTFILE_START;
 dp.countdir=COUNTDIR_START; 
 dp.length=dir->length;  
 if (f->dosname[0]!=0) {
  strcpy(mask,shortname(f));
  if (mask[0]!=0) {
   r=FindMask(b,mask,&dp,1+2);
   if (r.name[0]==0) //short name is already unique, so we keep it
   {         
    f->checksum=makeChecksum(f->dosname);
    return;
   }
  }
 }
                
                
 dot=strlast(f->name,'.'); //last dot 
 len=strlen(f->name);
 if  ( (((dot==0xffff) && (len<=8)) ^       //no dot Xor
        ((strpos(f->name,len,'.')==dot) &&       //only one dot and
         (dot<=8) &&   (len-dot<=4))  ) &&   //8.3 AND
       (dosStr(f->name)==1          )   )  //all chars valid
      
 {
  if (dot==0xffff)
  {
   for (i=0;i<len;i++) f->dosname[i]=asciiUpcase[(unsigned char)f->name[i]];
   for (i=len;i<8;i++) f->dosname[i]=' ';
   for (i=0;i<3;i++) f->dosext[i]=' ';
  }
  else
  {
   for (i=0;i!=dot;i++) f->dosname[i]=asciiUpcase[(unsigned char)f->name[i]];
   for (i=dot;i<8;i++) f->dosname[i]=' ';      //padding
   for (i=0;dot+i+1<len;i++) f->dosext[i]=asciiUpcase[(unsigned char)f->name[dot+1+i]];
   for (i=len-dot-1;i<3;i++) f->dosext[i]=' '; //padding
   len=dot;
  }
  tilda=1; //next number if not unique 
 }
 else //make a short alias
 {   
  //name
  for (i=j=0;(f->name[j]!='\0') && (i<6) && (j<=dot);j++)
  {
   if (dosChar(f->name[j])==1)
   {
    f->dosname[i]=asciiUpcase[(unsigned char)f->name[j]];
    i++;
   }
   else if (winChar(f->name[j])==1)
   {
    f->dosname[i]='_';
    i++;
   }
  }
  len=i;
  f->dosname[i]=126;i++; //~1
  f->dosname[i]='1';i++;
  //padding
  for (;i<8;i++) f->dosname[i]=' ';
  //extension
  if (dot!=0xffff)
  {        
   j=dot+1;
   for (i=0;(i<3) && (f->name[j]!=0);j++)
   {
    if (dosChar(f->name[j])==1)
    {
     f->dosext[i]=asciiUpcase[(unsigned char)f->name[j]];
     i++;
    }
    else if (winChar(f->name[j])==1)
    {
     f->dosext[i]='_';
     i++;
    }
   }
   //padding
   for (;i<3;i++) f->dosext[i]=' ';
  }
  else strncpy(f->dosext,"   ",3);
  tilda=2; //next number if not unique
 }
 //len=first char after name (<=6)
 //is it unique?   
 
 do
 {
  dp.cluster=dir->start_cluster;
  dp.relsector=0;
  dp.entry_no=0;           
  dp.countfile=COUNTFILE_START;
  dp.countdir=COUNTDIR_START; 
  dp.length=dir->length;
  strcpy(mask,shortname(f));
  r=FindMask(b,mask,&dp,1+2);
  if (r.name[0]!=0) //short name does already exist
  {         
   if (strcmp(r.name, f->name)!=0) //long names are the same? n: create id alias
   {
    _itoa(tilda,number,10);
    while (len+strlen(number)>=8) len--;
    f->dosname[len]=126; //~
    for (i=0;number[i]!=0;i++) f->dosname[len+1+i]=number[i];
    tilda++;
   }
   else //y: overwrite 
   {
    strncpy(f->dosname,r.dosname,8);
    strncpy(f->dosext,r.dosext,3);
    break;
   }
  }
 } while (r.name[0]!=0); //until unique
 f->checksum=makeChecksum(f->dosname);
}
       
       
void makeAliasCD(const struct DPB *b, struct longdirentry *f,dword countfile, dword countdir)
//countfile/countdir: number of entry * 2, files are even starting with 6, dirs are odd starting with 5
{    
 unsigned int dot,len,i,j;
 char number[7];
 dword tilda;

 if (strcmp(f->name,".")==0)
 {
  strncpy(f->dosname,".          ",11);
  return;
 }
 if (strcmp(f->name,"..")==0)
 {
  strncpy(f->dosname,"..         ",11);
  return;
 }

 dot=strlast(f->name,'.'); //last dot 
 len=strlen(f->name);
 if (b->cdfs==CD_ISO) //cut to 8.3
 {                
  for (i=0;i<8+3;i++) f->dosname[i]=' ';
  for (i=j=0;(i<len)&&(i<dot)&&(j<8);i++)
   if (dosChar(f->name[i])==1)
   {
    f->dosname[j]=asciiUpcase[(unsigned char)f->name[i]];
    j++;
   }
  if (dot!=0xffff)
   for (i=j=0;(dot+1+i<len)&&(j<3);i++)
    if (dosChar(f->name[dot+1+i])==1)
    {
     f->dosext[j]=asciiUpcase[(unsigned char)f->name[dot+1+i]];
     j++;
    }
  return;
 }
 if  ( (((dot==0xffff) && (len<=8)) ^       //no dot Xor
        ((strpos(f->name,len,'.')==dot) &&       //only one dot and
         (dot<=8) &&   (len-dot<=4))  ) &&   //8.3 AND
       (dosStr(f->name)==1          )   )  //all chars valid
      
 {
  if (dot==0xffff)
  {
   for (i=0;i<len;i++) f->dosname[i]=asciiUpcase[(unsigned char)f->name[i]];
   for (i=len;i<8;i++) f->dosname[i]=' ';
   for (i=0;i<3;i++) f->dosext[i]=' ';
  }
  else
  {
   for (i=0;i!=dot;i++) f->dosname[i]=asciiUpcase[(unsigned char)f->name[i]];
   for (i=dot;i<8;i++) f->dosname[i]=' ';      //padding
   for (i=0;dot+i+1<len;i++) f->dosext[i]=asciiUpcase[(unsigned char)f->name[dot+1+i]];
   for (i=len-dot-1;i<3;i++) f->dosext[i]=' '; //padding
   len=dot;
  }
  return;
 }
 else //make a short alias
 {   
  //name
  for (i=j=0;(f->name[j]!='\0') && (i<6) && (j<=dot);j++)
  {
   if (dosChar(f->name[j])==1)
   {
    f->dosname[i]=asciiUpcase[(unsigned char)f->name[j]];
    i++;
   }
   else if (winChar(f->name[j])==1)
   {
    f->dosname[i]='_';
    i++;
   }
  }
  len=i;
  f->dosname[i]=126;i++; //~1
  f->dosname[i]='1';i++;
  //padding
  for (;i<8;i++) f->dosname[i]=' ';
  //extension
  if (dot!=0xffff)
  {        
   j=dot+1;
   for (i=0;(i<3) && (f->name[j]!=0);j++)
   {
    if (dosChar(f->name[j])==1)
    {
     f->dosext[i]=asciiUpcase[(unsigned char)f->name[j]];
     i++;
    }
    else if (winChar(f->name[j])==1)
    {
     f->dosext[i]='_';
     i++;
    }
   }
   //padding
   for (;i<3;i++) f->dosext[i]=' ';
  }
  else strncpy(f->dosext,"   ",3);
 }
 //len=first char after name (<=6)
 
 //Add Tilda and number
 if ((f->atrib & F_DIR)==F_DIR) tilda=countdir;
 else tilda=countfile;
 _itoa((int)tilda,number,10);
 while (len+strlen(number)>=8) len--;
 f->dosname[len]=126; //~
 for (i=0;number[i]!=0;i++) f->dosname[len+1+i]=number[i];
 f->checksum=makeChecksum(f->dosname);
}
    
    
word *unichr(word *s, word c)
//Find first occurence of a charcter in an unicode string or return NULL
{       
 for (word *p=s;*p!=0;p++)
 {
  if (*p==c) return p;
 }
 return NULL;
}
       
       
void getCDDirEntry(const struct DPB *Disk,struct cddir *p, struct longdirentry *f,dword &countfile,dword &countdir)
//convert the cddir entry pointed to by p to a longdirentry pointed to by f
//countfile/dir: necessary for alias names
{
 char s[128];
 
 switch (Disk->cdfs)
 {
  case CD_JOLIET:
    if (p->name_len==1)
    {
     if ((p->uniname[0] & 0x00FF)==0) strcpy(s,"."); 
     if ((p->uniname[0] & 0x00FF)==1) strcpy(s,"..");
    }
    else
    {
     b2lnstr(p->uniname,p->name_len/2);
     f->uniname[0]=0;
     catUniStrgn(f->uniname,p->uniname,p->name_len/2);
     uni2ascii((unsigned int*)f->uniname,(unsigned char*)s);
     word *pU=unichr(f->uniname,0x003B); //cut off at ;
     if (pU!=NULL) *pU=0;
    }
  break;
  
  case CD_ISO:                
   strncpy(s,(char*)p->uniname,p->name_len);
   s[p->name_len]='\0';
   if (p->name_len==1)
   {
    if (s[0]==0) strcpy(s,".");
    else if (s[0]==1) strcpy(s,"..");
   }
  break;
  
  default:
   printf(msg[46]);
 }      
 strcpy(f->name,s);
 char *pS=strchr(f->name,';'); //cut off at ;
 if (pS!=NULL) *pS=0;
    
 f->start_cluster=p->data_lbn; 
 f->length=p->length;
 f->atrib=0;
 if ((p->atrib_iso & F_DIR_CD)==F_DIR_CD) f->atrib|=F_DIR;
 if ((p->atrib_iso & F_HIDDEN_CD)==F_HIDDEN_CD) f->atrib|=F_HIDDEN;
 
 f->date=((((p->date_y-80) << 4) | p->date_m) << 5) | p->date_d;
 f->time=((((p->time_h) << 6) | p->time_m) << 5) | (p->time_s/2);
      
 makeAliasCD(Disk,f,countfile,countdir);
    
 if ((f->atrib & F_DIR)==F_DIR) countdir+=2;
 else countfile+=2;
}

void findDirentryCDBySector(const struct DPB *b,struct longdirentry* f,dword qsector,struct dirpointer* start)
/*finds the direntry that points to a file at qsector and stores the result in f
 *for rootdir: set start->cluster=0
 */
{ 
 f->name[0]='\0';
 if (b->CD==0) return;
 if (start==NULL) return; //invalid pointer

 dword sector=start->cluster+start->relsector;
 byte *R;
 word pos=start->entry_no;
 dword no_sectors=start->length / b->bytes_per_sector; 
 if (((start->length % b->bytes_per_sector)==0) && (no_sectors!=0)) no_sectors--;
 dword last_sector=start->cluster+no_sectors;
 struct cddir *p;
            
 R=(byte*)malloc(b->bytes_per_sector);
 if (start->entry_no>=b->bytes_per_sector)
 {
  start->entry_no=0;
  pos=0;
  start->relsector++;
  sector++;
 }
 readCDSectors(b,sector,1,R);
 p=(cddir*)&R[pos];
 if ((p->entry_length==0) && (sector<last_sector)) //continue to next sector
 {
  sector++;
  start->relsector++;
  readCDSectors(b,sector,1,R);
  start->entry_no=0;
  pos=0;
  p=(cddir*)&R[pos];
 }

 while (p->entry_length!=0)
 {
  while (p->entry_length!=0)
  {                                                                     
   getCDDirEntry(b,p,f,start->countfile,start->countdir);
   if (f->start_cluster == qsector)
   {         
    start->entry_no+=p->entry_length;
    free(R);
    return; //found
   }
   pos+=p->entry_length;
   start->entry_no=pos;
   if (pos>=b->bytes_per_sector)
   {
    if (sector==last_sector) break; //end of dir ?
    start->entry_no=0;
    pos=0;
    start->relsector++;
    sector++;
   }
   p=(cddir*)&R[pos];
  } //while
  if (sector==last_sector) break;  //end of directory?
  sector++;
  start->relsector++;
  readCDSectors(b,sector,1,R);
  start->entry_no=0;
  pos=0;  
  p=(cddir*)&R[pos];
 }
                   
 f->name[0]='\0';
 free(R);
 return;
}
 
struct longdirentry short2longCD(struct DPB *b,char *current,char *path)
/*Find the long equivalent to current and return it in path
 *We assume directory sort ISO / Joliet beeing the same!
 *current must be a valid path! No error checking!
*/
{
 struct dirpointer iso,joliet;
 char *next;
 struct longdirentry idir,jdir;
 struct cddir *p,*q;
 int pos;
 byte *isoSec,*jolSec;
 char cname[MAX_LFN_SIZE];
 dword i_last_sec,j_last_sec;
 
 if (path!=NULL) path[0]='\0';
 jdir.name[0]='\0';
 jdir.start_cluster=b->first_root_sector;
 jdir.length=b->no_root_entries;
 if (b->CD!=1) return jdir;
 if (b->cdfs!=CD_JOLIET) return jdir;
 iso.cluster=b->CD_iso_root;
 iso.relsector=0;
 iso.entry_no=0;
 iso.countfile=COUNTFILE_START;
 iso.countdir=COUNTDIR_START;
 iso.length=b->CD_iso_root_length;
 joliet.cluster=b->first_root_sector;
 joliet.relsector=0;
 joliet.entry_no=0;
 joliet.countfile=COUNTFILE_START;
 joliet.countdir=COUNTDIR_START;
 joliet.length=b->no_root_entries;
 i_last_sec=iso.length/b->bytes_per_sector-1;
 j_last_sec=joliet.length/b->bytes_per_sector-1;
 isoSec=(byte*)malloc(b->bytes_per_sector);
 jolSec=(byte*)malloc(b->bytes_per_sector);
 if ((isoSec==NULL) || (jolSec==NULL))
 {
  printf(msg[47]);
  flushCache();
  exit(1);
 }
 next=strtok(current,"\\"); 
 if (path!=NULL) 
 {
  path[0]='A'+b->drive;
  path[1]=':';
  path[2]='\\';
  path[3]='\0';
 }
 do
 {
  next=strtok(NULL,"\\");
  if (next==NULL) break;  //no more

  readCDSectors(b,iso.cluster+iso.relsector,1,isoSec);
  readCDSectors(b,joliet.cluster+joliet.relsector,1,jolSec);
  do
  {
   p=(cddir*)&isoSec[iso.entry_no];
   q=(cddir*)&jolSec[joliet.entry_no];
   b->cdfs=CD_ISO;     //pretend we were ISO
   getCDDirEntry(b,p,&idir,iso.countfile,iso.countdir);
   b->cdfs=CD_JOLIET;  //b is always JOLIET if we get until here. so do not change places.
   //getCDDirEntry(b,q,&jdir,joliet.countfile,joliet.countdir);

   findDirentryCDBySector(b,&jdir,idir.start_cluster,&joliet);
   strcpy(cname,shortname(&idir)); //short
   if (match(cname,next)) 
   {
    if (idir.start_cluster != jdir.start_cluster) { 
     //security check
     printf(msg[49]);
     flushCache();
     exit(1);
    }
    break;   //found!
   }
   iso.entry_no+=p->entry_length;
   joliet.entry_no+=q->entry_length;
   if ((iso.entry_no>=b->bytes_per_sector) || (p->entry_length==0))
   {
    if (iso.relsector==i_last_sec) break; //dir end
    iso.entry_no=0;
    pos=0;
    iso.relsector++;
    readCDSectors(b,iso.cluster+iso.relsector,1,isoSec);
   }
   if ((joliet.entry_no>=b->bytes_per_sector) || (q->entry_length==0))
   {
    if (joliet.relsector==j_last_sec) break; //dir end
    joliet.entry_no=0;
    pos=0;
    joliet.relsector++;
    readCDSectors(b,joliet.cluster+joliet.relsector,1,jolSec);
   }
  } while (1);  //no checking for end of dir
  if ((p->entry_length==0) || (q->entry_length==0)) 
  {
   if (path!=NULL) path[0]='\0';
   jdir.name[0]='\0';
   break;
  }
  if (path!=NULL) 
  {
   strcat(path,jdir.name);
   strcat(path,"\\");
  } 
  iso.cluster=idir.start_cluster;
  iso.relsector=0;
  iso.entry_no=0;
  iso.countfile=COUNTFILE_START;
  iso.countdir=COUNTDIR_START;
  joliet.cluster=jdir.start_cluster;
  joliet.relsector=0;
  joliet.entry_no=0;
  joliet.countfile=COUNTFILE_START;
  joliet.countdir=COUNTDIR_START;
 } while (1);
 free(jolSec);
 free(isoSec);
 return jdir;
}

struct longdirentry long2shortCD(struct DPB *b,char *current,char *path)
/*Find the short equivalent to current and return it in path
 *We assume directory sort ISO / Joliet beeing the same!
 *current must be a valid path! No error checking!
*/
{
 struct dirpointer iso,joliet;
 char *next;
 struct longdirentry idir,jdir;
 struct cddir *p,*q;
 int pos;
 byte *isoSec,*jolSec;
 dword i_last_sec,j_last_sec;
     
 if (path!=NULL) path[0]='\0';
 idir.name[0]='\0';
 idir.start_cluster=b->first_root_sector;
 idir.length=b->CD_iso_root_length;
 if (b->CD!=1) return jdir;
 if (b->cdfs!=CD_JOLIET) return jdir;
 iso.cluster=b->CD_iso_root;
 iso.relsector=0;
 iso.entry_no=0;
 iso.countfile=COUNTFILE_START;
 iso.countdir=COUNTDIR_START;
 iso.length=b->CD_iso_root_length;
 joliet.cluster=b->first_root_sector;
 joliet.relsector=0;
 joliet.entry_no=0;
 joliet.countfile=COUNTFILE_START;
 joliet.countdir=COUNTDIR_START;
 joliet.length=b->no_root_entries;
 i_last_sec=iso.length/b->bytes_per_sector-1;
 j_last_sec=joliet.length/b->bytes_per_sector-1;
 isoSec=(byte*)malloc(b->bytes_per_sector);
 jolSec=(byte*)malloc(b->bytes_per_sector);
 if ((isoSec==NULL) || (jolSec==NULL))
 {
  printf(msg[48]);
  flushCache();
  exit(1);
 }
 next=strtok(current,"\\"); 
 if (path!=NULL) 
 {
  path[0]='A'+b->drive;
  path[1]=':';
  path[2]='\\';
  path[3]='\0';
 }
 do
 {
  next=strtok(NULL,"\\");
  if (next==NULL) break;  //no more

  readCDSectors(b,iso.cluster+iso.relsector,1,isoSec);
  readCDSectors(b,joliet.cluster+joliet.relsector,1,jolSec);
  do
  {
   p=(cddir*)&isoSec[iso.entry_no];
   q=(cddir*)&jolSec[joliet.entry_no];
   b->cdfs=CD_ISO;
   getCDDirEntry(b,p,&idir,iso.countfile,iso.countdir);
   b->cdfs=CD_JOLIET;
   getCDDirEntry(b,q,&jdir,joliet.countfile,joliet.countdir);
   if (match(jdir.name,next)) break;   //found!
   iso.entry_no+=p->entry_length;
   joliet.entry_no+=q->entry_length;
   if ((iso.entry_no>=b->bytes_per_sector) || (p->entry_length==0))
   {
    if (iso.relsector==i_last_sec) break; //dir end
    iso.entry_no=0;
    pos=0;
    iso.relsector++;
    readCDSectors(b,iso.cluster+iso.relsector,1,isoSec);
   }
   if ((joliet.entry_no>=b->bytes_per_sector) || (q->entry_length==0))
   {
    if (joliet.relsector==j_last_sec) break; //dir end
    joliet.entry_no=0;
    pos=0;
    joliet.relsector++;
    readCDSectors(b,joliet.cluster+joliet.relsector,1,jolSec);
   }
  } while (1);
  if ((p->entry_length==0) || (q->entry_length==0)) 
  {
   if (path!=NULL) path[0]='\0';
   idir.name[0]='\0';
   break;
  }
  if (path!=NULL) 
  {
   strcat(path,shortname(&idir));
   strcat(path,"\\");
  }
  iso.cluster=idir.start_cluster;
  iso.relsector=0;
  iso.entry_no=0;
  iso.countfile=COUNTFILE_START;
  iso.countdir=COUNTDIR_START;
  joliet.cluster=jdir.start_cluster;
  joliet.relsector=0;
  joliet.entry_no=0;
  joliet.countfile=COUNTFILE_START;
  joliet.countdir=COUNTDIR_START;
 } while (1); 
 free(jolSec);
 free(isoSec);
 return idir;
}
 
/**
 * Checks if there are entries in a directory.
 * The . and .. are not counted as entries.
 * Do not use this on the root directory!    
 * @param Disk The drive where dir is
 * @param dir The directory to check
 * @return !=0 if directory is empty, 0 otherwise.
 */      
int isDirEmpty(DPB *Disk, longdirentry *dir) {
  struct longdirentry g;
  struct dirpointer dp;
  dp.cluster=dir->start_cluster;
  dp.relsector=0;
  dp.entry_no=0;
  int i=0;
  do   
  {
   g=FindMask(Disk,NULL,&dp,0);
   i++;
  }while ((g.name[0]!=0) && (i<3));
  return (g.name[0]==0);
}        


/**
 * Removes a directory. Please use isDirEmpty() to check first!
 * @param Disk The drive where the directory is
 * @param dir The directory to delete
 * @param parentDir The parent directory of dir
 */
void rmDir(DPB *Disk, longdirentry *dir, longdirentry *parentDir) {                     
 struct dirpointer dp;
 struct longdirentry g;
 
 dp.cluster=parentDir->start_cluster;
 dp.relsector=0;
 dp.entry_no=0;
 g=FindMask(Disk,dir->name,&dp,1);  
 dp.entry_no--;    
 freeClusterChain(Disk,&g);
 deleteDirentry(Disk,parentDir,&dp,&g);
}