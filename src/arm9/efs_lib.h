//anoNL: Updated for libfat 1.0.2

/*

  Embedded File System (EFS)
  --------------------------

  file        : efs_lib.h
  author      : Noda (initially based on Alekmaul's libNitro)
  description : File system functions

  special thanks : Alekmaul, Michoko, Eris, Kusma, M3d10n

  history :

    12/05/2007 - v1.0
      = original release

    13/05/2007 - v1.1
      = cleaned up code a bit
      - removed header struct
      + added EFS_Flush() function, to ensure data is written

    18/05/2007 - v1.1a
      + added defines for c++ compatibility

    28/09/2007 - v1.2
      = fixed real fat mode (hopefully)
      + added some options

    25/06/2008 - v2.0
      + complete rewrite of the lib (breaks compatibility with old versions!)
      + added full devoptab integration, so it now use standard iolib functions
      + added automatic GBA rom detection (so it works in GBA flash kits & emu
        without any modifications), based on Eris' NitroFS driver idea
      + full chdir and unix standard paths (relative/absolute) support
      + great speed improve

*/

#ifndef __EFS_LIB_H__
#define __EFS_LIB_H__

#include <stdio.h>
#include <sys/dir.h>


// defines
#define EFS_MAXPATHLEN  256
#define EFS_MAXNAMELEN  128

// the NDS file path, dynamically set by the lib
extern char efs_path[256];

// init options
typedef enum {
    EFS_ONLY = 0,           // init only the efslib, may require prior fatlib init
    EFS_AND_FAT = 1,        // init libfat with default options before efslib if needed
    EFS_DEFAULT_DEVICE = 2  // set as default device in devoptab
} EFS_Init_Options;


// initialize the file system, first trying to search the NDS at the given path if !NULL
// return 1 if the FS has been initialized properly
// return 0 if there was an error initializing the FS (libfat included)
int EFS_Init(int options, char *path);


// devoptab functions implementation (don't use those directly)
DIR_ITER* EFS_DirOpen(struct _reent *r, DIR_ITER *dirState, const char *path);
int EFS_DirReset(struct _reent *r, DIR_ITER *dirState);
int EFS_DirNext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *st);
int EFS_DirClose(struct _reent *r, DIR_ITER *dirState);
int EFS_Open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
int EFS_Close(struct _reent *r, int fd);
ssize_t EFS_Read(struct _reent *r, int fd, char *ptr, size_t len);
ssize_t EFS_Write(struct _reent *r, int fd, const char *ptr, size_t len);
off_t EFS_Seek(struct _reent *r, int fd, off_t pos, int dir);
int EFS_Fstat(struct _reent *r, int fd, struct stat *st);
int EFS_Stat(struct _reent *r, const char *file, struct stat *st);
int EFS_ChDir(struct _reent *r, const char *name);

#endif  // define __EFS_LIB_H__
