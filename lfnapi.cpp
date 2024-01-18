#include "lfn.cpp"

int switch_uninstall;

void(__cdecl __interrupt __far *int21h)();

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's API for long file names %s\n",LFN_VER);
  printf("Copyright (C) 2000 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: lcopy [drive:][path]sourcefile [drive:][path][destinationfile][/?]\n\n");
  printf("Copy selected files.\n");
  printf("Switches:\n");
  printf("/c\tDisable cache\n");  
 #endif
 #ifdef LANG_DE
  printf("Odi's API fr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 2000 Ortwin Glck\nDies ist freie Software unter der GPL. Siehe readme Datei fr Details.\n\n");
  printf("LCOPY [Laufwerk:][Pfad]Quelldatei [Laufwerk:][Pfad][Zieldatei][/?]\n\n");
  printf("Kopiert Dateien.\n");
  printf("Optionen:\n");
  printf("/c\tCache abschalten\n");  
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
    case 'U': switch_uninstall=1;
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

int GetVolumeInformation(char __far *pszFS, unsigned int size, char __far *pszRoot, word *flags)
{
 int drv,err;
 struct DPB Disk;
  
 drv=drive_of_path(pszRoot); 
 printf("DEBUG:%u (%u)\n",drv,size);
 makeDPB(drv,Disk); 
 
 err = 0;
 switch (Disk.fat_type)
 {       
  case NO:
   return 1;
  break;
  
  case FAT12:
  case FAT16:        
   if (size>3)
    strcpy(pszFS,"FAT");
   *flags=0x4000;
  break;
   
  case FAT32:
   if (size>5)
    strncpy(pszFS,"FAT32",size);
   *flags=0x4006;
  break;
  
  case CD:
   if (size>4)
    strncpy(pszFS,"CDFS",size);
   if (Disk.cdfs==CD_JOLIET) {
    *flags=0x4006;
   }
   else {
    *flags=0x4000;
   }
  break; 
 }        
 
 return err;
}

void (__interrupt __far __cdecl Dispatcher)( unsigned _es, unsigned _ds, 
                                     unsigned _di, unsigned _si,
                                     unsigned _bp, unsigned _sp,
                                     unsigned _bx, unsigned _dx,
                                     unsigned _cx, unsigned _ax,
                                     unsigned _ip, unsigned _cs,
                                     unsigned flags)
{
 #define PTR(seg,ofs) (void*)((((long)seg) << 16) | ((long)ofs))
 
 char __far *pszFS, *pszRoot;
 int err=0;
 word bits;
 
 _enable();
 switch (_ax)
 {
  case 0x71A0:          
   
   pszFS = (char*) PTR(_es,_di);
   pszRoot = (char*) PTR(_ds,_dx);
   err=GetVolumeInformation(pszFS,_cx,pszRoot, &bits); 
   _bx=bits;
   _cx=0x00ff;
   _dx=0x0104;
  break;
  
  default:
   _chain_intr(int21h);
  break;
 }
 if (err) {
  flags|=1; //set carry flag
  _ax=err;
 }
 else {
  flags&=0xfffe; //clear carry flag
 }
}

void Map21hFunctions()
{
 int21h = _dos_getvect(0x21);
 
 _dos_setvect(0x21, (void (__interrupt __far __cdecl *)(void))&Dispatcher);
}

void main(int argc, char *argv[], char *envp)
{                         
 int i;
 
 switch_uninstall=0;

 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
 }
 
 /* check if already installed! */
 
 Map21hFunctions(); 
 
 initCache(); 
 
 //_heapmin();
  
 int rSS, rSP,paragraphs;                
 _asm {
  mov [rSS],ss
  mov [rSP],sp
 }                      
 paragraphs = rSS+rSP/16;
 printf("reserved Paragraphs: %u", paragraphs);
 _dos_keep(0,paragraphs);
}