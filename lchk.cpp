// lren

#include "lfn.cpp"

void GetCDFSInfo(struct DPB *d) {
 struct iso_primary_descriptor desc;
 int cy,usc2,joliet,sector;            
 joliet=0;
 sector=VTOC_START;
 #ifdef LANG_EN
  printf("Physical File Systems:\n");
  printf("#\tID\tEnc\tRoot\n");    
 #endif                                      
 #ifdef LANG_DE
  printf("Physikalische Datei Systeme:\n");
  printf("#\tID\tEnc\tRoot\n");    
 #endif 
 cy=readCDSectorsP(d,sector+d->CD_vol_start,1,&desc);
 if (cy!=0) return;  
 while ((desc.type_of_descriptor!=0xff)) {
  usc2= ((desc.esc_seq[0]=='%') && (desc.esc_seq[1]=='/'))
   && ((desc.esc_seq[2]=='@') || (desc.esc_seq[2]=='C') || (desc.esc_seq[2]=='E'));
  printf("%.2X\t%.5s\t%.3s\t%.u", desc.type_of_descriptor, desc.ID, ((desc.vol_flags & 1)==0) ? usc2 ? "Uni" : desc.esc_seq : "ASC", desc.root_start); 
  if (desc.type_of_descriptor == 0) printf("\t(boot)");
  printf("\n");
  sector++; 
  cy=readCDSectorsP(d,sector+d->CD_vol_start,1,&desc);
  if (cy!=0) return;      
 }  
}

void GetTrackInfo(char drive)
{
 AudioDiskInfo adi;
 AudioTrackInfo ati;
 byte track, dataTrack;
 
 #ifdef LANG_EN
  printf("Information on CD-ROM sessions:\n"); 
 #endif                                      
 #ifdef LANG_DE
  printf("CD-ROM Session Informationen:\n"); 
 #endif
 if (GetAudioDiskInfo(drive,0,&adi)!=0)
 { 
  printf("Tracks: %u - %u\n",adi.firstTrack,adi.lastTrack);  
  if (adi.lastTrack>=adi.firstTrack)
  { 
   #ifdef LANG_EN
    printf("List of tracks:\n");
    printf("Track #\tStart    Type\n");
   #endif                                      
   #ifdef LANG_DE
   printf("Liste der Tracks:\n");
   printf("Track #\tStart    Typ\n");
   #endif
   dataTrack=0;
   for (track=adi.firstTrack;track<=adi.lastTrack;track++) //Mixed-Mode
   {
    GetAudioTrackInfo(drive,0,track,&ati);
    if ((ati.trackInfo & DATA_TRACK)!=0) dataTrack=track;
    printf("%.2u\t%.2u:%.2u.%.2u ",track,ati.start.minute,ati.start.second,ati.start.frame);
    if ((ati.trackInfo & DATA_TRACK) !=0) printf("DATA\n");
    else printf("AUDIO\n");
   }
   if (dataTrack==0)
   {
    #ifdef LANG_EN
     printf("No data track found!\n");    
    #endif                                      
    #ifdef LANG_DE
     printf("Kein Datentrack gefunden!\n");    
    #endif
    return;
   }
  }
  else //CD-Extra
  {  
   #ifdef LANG_EN
    printf("This is probably a CD-Extra (1 Audio Session, 1 Data Session)\n");   
   #endif                                      
   #ifdef LANG_DE
    printf("Dies ist vermutlich eine CD-Extra (1 Audio Session, 1 Daten Session)\n");   
   #endif
   return;
  }
 }
}

void PrintDPB(struct DPB &b)
{
 struct dosver ver;                   
 getDOSver(&ver);
 #ifdef LANG_EN
  printf("Version ");printf(LFN_VER);printf("\n");
  printf("Copyright (C) 1999 Ortwin Glueck\nThis is free software under GPL. See the readme file for details.\n\n");
  printf("DOS Version: %X-%u.%u\tCode page: %u\n",ver.oem,ver.main,ver.sub, getDOSActiveCP());
  printf("Drive: %u\nBytes per Sector: %u\n",b.drive,b.bytes_per_sector);
 #endif                                      
 #ifdef LANG_DE
  printf("Version ");printf(LFN_VER);printf("\n");
  printf("Copyright (C) 1999 Ortwin GlÅck\nDies ist freie Software unter der GPL. Siehe readme Datei fÅr Details.\n\n");
  printf("DOS Version: %X-%u.%u\tCode page: %u\n",ver.oem,ver.main,ver.sub, getDOSActiveCP());
  printf("Laufwerk: %u\nBytes pro Sektor: %u\n",b.drive,b.bytes_per_sector);
 #endif
 
 if (b.fat_type==CD) 
 {                   
  GetCDFSInfo(&b);
  GetTrackInfo(b.drive);
  #ifdef LANG_EN
   printf("First Root Sector: %u\n",b.first_root_sector);
   printf("Session Size: %lu\n",b.no_sectors);
   printf("Session Start: %lu\n",b.CD_vol_start); 
  #endif                                      
  #ifdef LANG_DE
   printf("Erster Root Sektor: %u\n",b.first_root_sector);
   printf("Session Grîsse: %lu\n",b.no_sectors);
   printf("Session Start: %lu\n",b.CD_vol_start); 
  #endif
 }
 else
 {
  #ifdef LANG_EN
   printf("Sectors per Cluster: %u\n",b.sectors_per_cluster);
   printf("Number of Clusters: %lu\n",b.no_clusters);
   printf("Number of FATs: %u   Number of Root entries: %u\n",b.no_fats,b.no_root_entries);
   printf("Media Descriptor: %x\nSectors per FAT: %lu\n",b.media_descriptor,b.sectors_per_fat);
   printf("Sectors / reserved: %lu / %u\n",b.no_sectors,b.reserved_sectors);
   printf("First Data Sector: %lu\n",b.first_data_sector);
   printf("First Root Sector/Cluster: %lu/%lu\n",b.first_root_sector,b.first_root_cluster);
   printf("Root Sectors: %u\nFat entries per Sector: %u\n",b.no_root_sectors,b.fat_entries_per_sector);
  #endif                                      
  #ifdef LANG_DE
   printf("Sektoren pro Cluster: %u\n",b.sectors_per_cluster,b.reserved_sectors);
   printf("Anzahl Clusters: %lu\n",b.no_clusters);
   printf("Anzahl FATs: %u   Anzahl Root EintrÑge: %u\n",b.no_fats,b.no_root_entries);
   printf("Media Descriptor: %x\nSektoren pro FAT: %lu\n",b.media_descriptor,b.sectors_per_fat);
   printf("Sektoren / reserviert: %lu / %u\n",b.no_sectors,b.reserved_sectors);
   printf("Erster Datensektor: %lu\n",b.first_data_sector);
   printf("Erster Root Sektor/Cluster: %lu/%lu\n",b.first_root_sector,b.first_root_cluster);
   printf("Root Sektoren: %u\nFAT-EintrÑge pro Sektor: %u\n",b.no_root_sectors,b.fat_entries_per_sector);
  #endif
 }
 #ifdef LANG_EN
  printf("Label: %.32s\n",b.label);
  printf("File System: ");
 #endif                                      
 #ifdef LANG_DE
  printf("Bezeichnung: %.32s\n",b.label);
  printf("Dateisystem: ");
 #endif

 switch (b.fat_type)
 {
  case FAT12: printf("FAT12\n");break;
  case FAT16: printf("FAT16\n");break;
  case FAT32: printf("FAT32\n");break; 
  case CD:    printf("CD-ROM: ");
              switch (b.cdfs)
              {                                                   
               case CD_NO:
                #ifdef LANG_EN
                 printf("Unknown\n");
                #endif                                      
                #ifdef LANG_DE
                 printf("Unbekannt\n");
                #endif
               break;
               case CD_ISO: printf("ISO-9660\n");break;
               case CD_JOLIET: printf("Microsoft Joliet\n");break;
              }
              break;
  default:
   #ifdef LANG_EN
    printf("Unknown\n");
   #endif                                      
   #ifdef LANG_DE
    printf("Unbekannt\n");
   #endif
  break;
 }
 #ifdef LANG_EN
  printf("FAT32 compatible disk access ");
 #endif                                      
 #ifdef LANG_DE
  printf("FAT32 kompatibler Zugriffsmodus "); 
 #endif
 
 #ifdef LANG_EN
  if (win98==1) printf("enabled\n");
  else printf("disabled\n");
 #endif                                      
 #ifdef LANG_DE
  if (win98==1) printf("wird benÅtzt\n");
  else printf("wird nicht benÅtzt\n");
 #endif

 #ifdef LANG_EN
  printf("Volume locking ");
  if (noLocking==1) printf("disabled\n");
  else printf("enabled\n");
 #endif                                      
 #ifdef LANG_DE
  printf("Laufwerkssperrung ");
  if (noLocking==1) printf("wird nicht benÅtzt\n");
  else printf("wird benÅtzt\n");
 #endif     
 
 #ifdef LANG_EN
  printf("Cache: ");
  if (bypassCache) printf("disabled\n");
  else {
   if (useVMCache!=0) {
    #ifdef VMEMORY   
     printf("virtual, areas: %u, pages per area: %u, mem used:%lu bytes\n",no_cache_areas,no_pages_per_area,((long)no_cache_areas*no_pages_per_area*sizeof(cachepage)));
    #endif    
   }
   else printf("standard\n");
  }
 #endif
 #ifdef LAND_DE
  printf("Cache: ");
  if (bypassCache) printf("abgeschaltet\n");
  else {
   if (useVMCache!=0) {
    #ifdef VMEMORY   
     printf("virtuell, Bereiche: %u, pages per area: %u, RAM belegt:%lu Bytes\n",no_cache_areas,no_pages_per_area,((long)no_cache_areas*no_pages_per_area*sizeof(cachepage)));
    #endif    
   }
   else printf("normal\n");
  }
 #endif
}

int FSInfo(const struct DPB *b)
//FAT32 only
//allocated_clusters: relative number of newly allocated clusters, negative values are freed clusters
{
 struct BIGFATBOOTFSINFO *fsi;           
 if (b->fat_type!=FAT32) return 1;
 fsi=(struct BIGFATBOOTFSINFO*)malloc(b->bytes_per_sector);
 if (fsi==NULL)
 {
  printf(msg[27]);
  return 1;
 }
 readSector(b->drive,b->fs_info_sector,1,fsi);
 if ((fsi->start_signature==0x41615252) && (fsi->signature==0x61417272) && (fsi->end_signature==0xAA550000)) {
  if (fsi->no_free_clusters!=-1)
  {            
   #ifdef LANG_EN
    printf("Free clusters: %lu",fsi->no_free_clusters);
   #endif                                      
   #ifdef LANG_DE
    printf("Freie Cluster: %lu",fsi->no_free_clusters);
   #endif
  }
  else {         
   #ifdef LANG_EN
    printf("Free clusters: unknown");
   #endif                                      
   #ifdef LANG_DE
    printf("Freie Cluster: unbekannt");
   #endif
  }              
  #ifdef LANG_EN
   printf("   Next free cluster: %lu\n",fsi->next_free_cluster);
  #endif                                      
  #ifdef LANG_DE
   printf("   NÑchster freier Cluster: %lu\n",fsi->next_free_cluster);
  #endif  
 }
 else {         
  #ifdef LANG_EN
   printf("Extended filesystem information record signatures invalid!\n");
  #endif                                      
  #ifdef LANG_DE
   printf("Erweiterte Dateisystem Info-Signaturen ungÅltig!\n");
  #endif
 }
 free(fsi);
 return 0;
}


void main(int argc,char *argv[],char* envp[])
{
 struct DPB Disk;
 byte drv;                  
 initLFNLib();
 if (argc==0) drv=drive_of_path(NULL);
 else drv=drive_of_path(argv[1]); 
 makeDPB(drv,Disk);  //we dont care for errors. lchk is diagnostic software!
 PrintDPB(Disk); 
 FSInfo(&Disk);
 killCache();
}