// lcd: Longfilename CD

#include "lfn.cpp"

void PrintHelp(void)
{
 #ifdef LANG_EN
  printf("This is Odi's Cd for long file names %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("Usage: lcd [drive:]path\n"); 
  printf("Change to any drive and/or directory.\n");
  printf("Switches.\n");
  printf("/i\tForce ISO-9660 file system\n");
  printf("/tn\tUse CD data track n\n");
 #endif
 #ifdef LANG_DE
  printf("Odi's Cd fÅr lange Dateinamen %s\n",LFN_VER);
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("LCD [Laufwerk:]Pfad\n"); 
  printf("Wechsel zu beliebigem Laufwerk/Verzeichnis.\n");
  printf("Optionen.\n");
  printf("/i\tauf CD-ROMs immer ISO-9660 Dateisystem benÅtzen\n");
  printf("/tn\tBenÅtze CD-Daten Track (Session) Nummer n\n");
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
    case '?':
     PrintHelp();
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

void main(int argc, char *argv[], char **envp)
{
 struct DPB Disk;
 char drv;
 int i,no;
         
 if (argc==1) exit(1);         
 no=argc;
 for (i=1;i<argc;i++)
 {
  if (argv[i][0]=='/')
   ParseParams(argv[i]);
  else no=i;
 }
 useVMCache=0;
 initLFNLib();
 drv=drive_of_path(argv[no]);
 makeDPB(drv,Disk);    //we dont care about errors. its not dangerous
 
 ChangeDir(&Disk,argv[no]);
}
