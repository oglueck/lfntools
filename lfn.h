#ifndef _LFN_H
#define _LFN_H

typedef unsigned short word;
typedef unsigned char byte;
typedef unsigned long int dword;

#define LFN_VER "1.79"  
#define ISO_STANDARD_ID "CD001"

//file attributes
#define F_READ_ONLY 1
#define F_HIDDEN    2
#define F_SYSTEM    4
#define F_LABEL     8
#define F_LFN       15
#define F_DIR       16
#define F_ARC       32   
//file attributes for CD
#define F_HIDDEN_CD 1
#define F_DIR_CD    2 
#define F_ASOC_CD   4
#define F_INFO_CD   8
#define F_PERM_CD   16
#define F_EXT_CD    128 
//file protection
#define PROTECT_NO  0
#define PROTECT_SYS 1
#define PROTECT_RO  2
#define PROTECT_ALL 3
//CD track type bits
#define AUDIO_TRACK 1
#define DATA_TRACK 64
//CD alias generation for Joliet
#define COUNTFILE_START 6
#define COUNTDIR_START 1             
//CD geometry
#define VTOC_START 16
//maximum empty space in cd dir sector
//#define CD_DIR_LIMIT 0x4C
//data type on writeSector
#define W_FAT  0x2001
#define W_DIR  0x4001
#define W_FILE 0x6001
#define W_OTHER 0x0001
//string lengths
#define MAX_PATH_SIZE 260
#define MAX_LFN_SIZE 255
#define CHR_PER_LFN 13
//FAT constants
#define FAT_MASK 0x0fffffff
#define INV_FAT_MASK 0xf0000000
#define EOF12    0xff8
#define EOF16    0xfff8
#define EOF32    0x0ffffff8
#define MAX_SECTOR_SIZE 512           //future harddisk and some MO may exceeed this size!
//Cache
#define MAX_DRIVES 26
#define MAX_CACHE_PAGES 256 
#define MAX_CACHE_AREAS 50           //make sure the number of pages does not exceed 32767!
#define MAX_CACHE_AREA_SIZE 48000    //and the memory used for vcachelookup does not exceed 64K!

//states of cache pages
enum Tdirty
{
 EMPTY, CLEAN, DIRTY
};


//Filesystem types
enum Tfat
{   
 NO,
 FAT12,
 FAT16,
 FAT32,
 CD
};

//CD types
enum CDFS_T {CD_NO,CD_ISO,CD_JOLIET}; 


#pragma pack(1)  //keep the pysical blocks byte aligned!
struct BPB //physical BIOS Parameter Block
{
  word bytes_per_sector;
  byte sectors_per_cluster;
  word reserved_sectors;
  byte no_fats;
  word no_root_entries;
  word no_sectors;
  byte media_descriptor;
  word sectors_per_fat;
  word sectors_per_track;
  word no_heads;
  dword no_hidden_sectors;
  dword no_large_sectors;
  byte phys_drive_number;
  byte current_head;
  byte signature;
  dword ID;
  char volume_label[11];
  char fat_type[8];
};

struct BPB32 //physical BIOS Parameter Block FAT32
{
  word bytes_per_sector;
  byte sectors_per_cluster;
  word reserved_sectors;
  byte no_fats;
  word no_root_entries;      //ignored
  word no_sectors;           //FAT32: 0
  byte media_descriptor;
  word sectors_per_fat;      //FAT32: 0
  word sectors_per_track;
  word no_heads;
  dword no_hidden_sectors;
  dword no_large_sectors;
  dword sectors_per_fat32;
  word flags32;
  word version32;
  dword cluster_of_root;
  word fs_info_sector;      //cf BIGFATBOOTFSINFO
  word backup_boot_sector;
  word reserved[6]; 
  byte phys_drive_number;
  byte unused;
  byte XBS;
  dword ID;
  char volume_label[11];
  char fat_type[8];
};

struct BIGFATBOOTFSINFO //physical FAT32 info block
{
 dword start_signature; //="RRaA"
 byte reserved[480];        
 dword signature; //="rrAa"
 dword no_free_clusters; //-1 if unknown
 dword next_free_cluster;
 byte reserved2[12];
 dword end_signature; //=AA550000
};

struct DPB //Disk Parameter Block
{
  char drive;
  char CD;
  char label[32];
  word bytes_per_sector;
  byte sectors_per_cluster;
  word reserved_sectors;
  byte no_fats;
  dword no_clusters;
  dword next_cluster;     //remembers where the last search after free clusters stopped
  dword no_root_entries;  //CD-ROM: length of root in bytes
  byte media_descriptor;
  dword sectors_per_fat;
  dword no_sectors;
  dword first_data_sector;
  dword first_root_sector;
  dword first_root_cluster; 
  word no_root_sectors;
  word fat_entries_per_sector;   //NOT constant on FAT12, do not rely on this!
  word dir_entries_per_sector;
  word fs_info_sector;      //cf BIGFATBOOTFSINFO  
  enum Tfat fat_type;      
  enum CDFS_T cdfs;
  dword CD_vol_start; //start of the current data track 
  dword CD_iso_root;  //contains sector if ISO root dir 
  dword CD_iso_root_length; 
};

struct direntry //physical
{
 char name[8];
 char ext[3];
 byte atrib;
 char reserved[8];
 word high_cluster; //high word of the start cluster used in fat32: is this correct?
 word time;
 word date;
 word start_cluster;
 dword length;
};

struct lfnentry //physical
{
 byte count;
 word name1[5];
 byte atrib; //0x0f
 byte ftype; //00
 byte checksum;
 word name2[6];
 word reserved; //0x0000
 word name3[2];
};

struct longdirentry
{
 char name[MAX_LFN_SIZE];
 word uniname[MAX_LFN_SIZE];
 char dosname[8];           //do not split dosname & dosext !! Code relies on them glued together!
 char dosext[3];
 byte atrib;
 byte ftype; //always 0
 byte checksum;
 word time;
 word date;
 dword start_cluster; //cluster on disk, sector on CD
 dword length;
 char reserved[8]; //store reserved bytes in direntry
};

struct FATfile
{
 dword cluster;
 dword sector;
};

struct secblk
{
 dword sector;
 word  no_sectors;
 void __far *pBuffer;
};

struct dosver
{
 byte main;
 byte sub;
 byte oem;
};

/*  On CD the members of dirpointer have slightly different meanings:
 *  cluster=first abs. sector of dir, relsector=rel. sector of actual position, entry_no=position (in bytes)
 */
 
struct dirpointer
{
  dword cluster;
  dword relsector; //0..(sectors_per_cluster-1)
  word  entry_no; 
  dword countfile; //initialize with COUNTFILE_START and COUNTDIR_START at directory start, needed by Joliet FS
  dword countdir;
  dword length;   //length of directory in bytes (CD-ROM only)
};

struct iso_primary_descriptor
{
 byte type_of_descriptor;
 char ID[5];
 byte version;
 byte vol_flags;
 char system_id[32];
 char cd_title[32];
 char unused1[8];
 dword volume_size;
 dword volume_size_m;
 char esc_seq[3];
 char unused2[29];
 word vol_set_size;
 word vol_set_size_m;
 word vol_seq_no;
 word vol_seq_no_m;
 word block_size;
 word block_size_m;
 dword path_table_size;
 dword path_table_size_m;
 dword path_table;
 dword opt_path_table;
 dword path_table_m;
 dword opt_path_table_m;
 char root_1[2];
 dword root_start;
 dword root_2;
 dword root_length;
 char root_3[20];
 char vol_set_id[128];
 char publisher_id[128];
 char preparer_id[128];
 char application_id[128];
 char copyright[37]; 
 char abstract[37];
 char bibliography[37];
 char creation_date[16];
 char modification_date[16];
 char expiration_date[16];
 char effective_date[16];
 byte fs_version;
 byte unused3;
 char app_data[512];
 char cdx_id[8];
 char unused4[653];
};

struct iso_directory_record {
 char length   [1];
 char ext_attr_length  [1]; 
 char extent   [8];
 char size   [8];
 char date   [7];
 char flags   [1];
 char file_unit_size  [1]; 
 char interleave   [1]; 
 char volume_sequence_number [4];
 unsigned char name_len  [1];
 char name[255];
};

struct cddir //ISO/HighSierra Directory entry
{
 byte  entry_length;
 byte  xar_length;
 dword data_lbn;       //lbn=logical block number=sector
 dword data_lbn_mot;
 dword length;
 dword length_mot;
 byte  date_y;
 byte  date_m;
 byte  date_d;
 byte  time_h;
 byte  time_m;
 byte  time_s;
 byte  atrib_sierra;
 byte  atrib_iso;
 word  interleave;
 word  seq;
 word  seq_mot;
 byte  name_len;
 word  uniname[110]; //unicode
};

struct BasicReqHeader
{
 byte length,sub_unit,command;
 word status;
 byte reserved[8];
};

struct InputReqHeader
{
 BasicReqHeader basic;
 byte media_descriptor;
 dword transfer_address;
 word size,start;
 dword error;
};

struct RedBookAddress
{
 unsigned char frame,second,minute,unused;
};

struct AudioDiskInfo
{
    byte                    sub_function;
    unsigned char           firstTrack;
    unsigned char           lastTrack;
    struct RedBookAddress   leadOutTrack;
};

struct  AudioTrackInfo
{
 byte                    sub_function;
 unsigned char           track;
 struct RedBookAddress   start;
 unsigned char           trackInfo;
};

struct vcacheinfo {
 byte empty;
 byte drive;
 dword sector; 
 dword timestamp;
};

#pragma pack()

struct cachepage
{
 byte   drive;
 dword  sector;   
 word   typ;
 dword  timestamp;
 Tdirty dirty;
 char   data[MAX_SECTOR_SIZE];
};   


//Globals
int win98=1; 
int noLocking=0;
int cacheWasSetup=0;
int bypassCache=0; //Set to 1 if you want to bypass all caching.
int useVMCache=1; //virtual memory for caching
int forceIso=0; //allow Joliet  
int forceTrack=0;         
                 
//internal globals
int fat12of=0; //FAT12 overflow   
                                                                                       
//vcache
struct cachepage *cache[MAX_CACHE_PAGES];     
int no_free_pages=0; //to be filled later 
int total_cache_pages=0; //to be filled later
#ifdef VMEMORY 
 int no_cache_areas=0; //to be filled later
 int no_pages_per_area=MAX_CACHE_AREA_SIZE/sizeof(cachepage);
 int current_area_no=-1;     
 cachepage __far* current_area=NULL;
 int current_area_dirty=0;
 _vmhnd_t cacheArea[MAX_CACHE_AREAS];
 struct vcacheinfo *vcachelookup;
#endif
word bpsd[MAX_DRIVES]; //effective bytes per sector of a drive
dword timestamp=1;  //init cache timestamp

//Functions
void getDOSver(struct dosver *v); 
int directAccessDenied(char drive);
int readSectorP(char drive,dword sector,word count, void *buffer);
int writeSectorP(char drive,dword sector,word count, void *buffer,word typ);
cachepage __far* getCacheArea(int i);
void initCache(void);
int isInCache(char drive, dword sector,word *page);
word getEmptyPage(void);
void discardPage(word page);
word discardOldestPage(void);
void flushCache(void);
int readSector(char drive,dword sector,word count, void *buffer);
int writeSector(char drive,dword sector,word count, void *buffer,word typ);
int lockDrive(byte drv);
int unlockDrive(byte drv);
int isCDRomSupported(word drv);
int isCDRom(word drv);
dword rba2lba(RedBookAddress rba);
void InitReqHeader(BasicReqHeader *brh,byte len,byte cmd,byte unit);
void InitInputHeader(InputReqHeader *irh,void _far *cb,word length,byte unit);
void InitAudioDiskInfo(AudioDiskInfo *adi);
void InitAudioTrackInfo(AudioTrackInfo *ati, byte track);
void SendDDRequest(char drive,void _far *rh);
int GetAudioDiskInfo(char drive, unsigned char subunit,AudioDiskInfo *adi);
int GetAudioTrackInfo(char drive, byte subunit, byte trackNo,AudioTrackInfo *ati);
dword DetermineVolStart(byte drive);
int readCDSectorsP(const struct DPB *b,dword sector,word count,void _far *buffer);
int readCDSectors(const struct DPB *b,dword psector,word count,void _far *buffer);
int getVTOC(struct iso_primary_descriptor* desc, struct DPB *d);
int makeDPB(char drive,struct DPB &d);
byte makeChecksum(char *aliasname);
void unincpy(word *dest,word *source,word count);
void lfnString(struct lfnentry &lfn,char *bs,word *ws);
unsigned int strpos(const char *string,int len,char c);
void b2lstr(word* s);
void b2lnstr(word* s,word n);
dword FATsector(const struct DPB *b,dword cluster);
dword getFATval(const struct DPB *b,void *FAT,dword cluster);
void setFATval(const struct DPB *b,void *FAT,dword cluster,dword val);
dword getFAT12valX(const struct DPB *b,dword cluster,dword val);
void setFAT12valX(const struct DPB *b,dword cluster,dword val);
dword getNextAvailClusterNo(struct DPB *b);
dword firstSectorOfCluster(const struct DPB *b,dword cluster);
int getNextCluster(const struct DPB *b,struct FATfile *f);
char* shortname(const struct longdirentry* d);
int match(const char *name,const char *mask);
int WildComp(const char *I,const char *W);
void catUniStrg(word *dest,const word *source); 
void catUniStrgn(word *dest, const word *source, const int count);
struct longdirentry FindMaskCD(const struct DPB *b,char *mask,struct dirpointer* start,int exact);
struct longdirentry FindMask(const struct DPB *b,char *mask,struct dirpointer* start,int exact);
int updateFSInfo(const struct DPB *b,signed long allocated_clusters,dword last_cluster);
dword extendFile(struct DPB *b,struct FATfile* chain,dword count,int type);
char* seekPath(struct DPB *b,char *path,char * longpath,longdirentry &pDir,int exact);
void ChangeDir(struct DPB *b, char *longpath);
int drive_of_path(char *path);
void deleteDirentry(struct DPB *b,struct longdirentry *dir,struct dirpointer *dp,struct longdirentry *f);
int freeClusterChain(struct DPB *b,struct longdirentry *f);
int recycleDirentries(struct DPB *b,struct dirpointer *start,byte count);   
int makeDirectory(struct DPB *b, struct longdirentry *parent, char *name, longdirentry *f);
int makeDirectory(struct DPB *b, struct longdirentry *parent, longdirentry *f);
int initDirectory(struct DPB *b, struct longdirentry *parent, struct longdirentry *dir);
int insertDirentry(struct DPB *b,struct longdirentry *dir,struct longdirentry *f, int protection);
int insertDirentryP(struct DPB *b,struct longdirentry *dir,struct longdirentry *f, int protection);
int strlast(char *s,char c);
int winChar(char c);
int dosChar(unsigned char c);
int dosStr(char *s);
void convertInput(char *s);
void makeAlias(struct DPB *b, struct longdirentry *dir, struct longdirentry *f);
void makeAliasCD(const struct DPB *b, struct longdirentry *f,dword countfile, dword countdir);
word *unichr(word *s, word c);
void getCDDirEntry(const struct DPB *Disk,struct cddir *p, struct longdirentry *f,dword &countfile,dword &countdir);
struct longdirentry short2longCD(struct DPB *b,char *current,char *path);
struct longdirentry long2shortCD(struct DPB *b,char *current,char *path);
int isDirEmpty(DPB *Disk, longdirentry *dir);          
void rmDir(DPB *Disk, longdirentry *dir, longdirentry *parentDir);

#endif // _LFN_H