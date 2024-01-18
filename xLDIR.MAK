# Microsoft Visual C++ generated build script - Do not modify

PROJ = LDIR
DEBUG = 0
PROGTYPE = 6
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = D:\DATEN\PROG\LFN\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = LDIR.CPP    
RC = rc
CFLAGS_D_DEXE = /I D:\Progra~1\Sprachen\MSVC\include\ /nologo /G2 /W3 /Zi /AL /Od /D "_DEBUG" /D "_DOS" /FR /Fd"LDIR.PDB"
CFLAGS_R_DEXE = /I D:\Progra~1\Sprachen\MSVC\include\ /nologo /Gs /G3 /W3 /AL /Ox /D "NDEBUG" /D "_DOS" 
LFLAGS_D_DEXE = /NOLOGO /NOI /STACK:24064 /ONERROR:NOEXE /CO 
LFLAGS_R_DEXE = /NOLOGO /NOI /STACK:24064 /ONERROR:NOEXE 
LIBS_D_DEXE = oldnames llibce 
LIBS_R_DEXE = oldnames llibce 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_DEXE)
LFLAGS = $(LFLAGS_D_DEXE)
LIBS = $(LIBS_D_DEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_DEXE)
LFLAGS = $(LFLAGS_R_DEXE)
LIBS = $(LIBS_R_DEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = LDIR.SBR


LDIR_DEP = d:\daten\prog\lfn\lfn.cpp \
	d:\daten\prog\lfn\unicode2.inc \
	d:\daten\prog\lfn\lfn.h \
	d:\daten\prog\lfn\lfnmsgen.inc


all:	$(PROJ).EXE

LDIR.OBJ:	LDIR.CPP $(LDIR_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c LDIR.CPP

$(PROJ).EXE::	LDIR.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
LDIR.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
D:\PROGRA~1\SPRACHEN\MSVC\LIB\+
D:\PROGRA~1\SPRACHEN\MSVC\MFC\LIB\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
