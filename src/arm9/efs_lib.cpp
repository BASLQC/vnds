//anoNL: Updated for libfat 1.0.2

/*

  Embedded File System (EFS)
  --------------------------

  file        : efs_lib.c
  author      : Noda (initially based on Alekmaul's libNitro)
  description : File system functions

  special thanks : Alekmaul, Michoko, Eris, Kusma, M3d10n

  history :

    12/05/2007 - v1.0
      = original release

    13/05/2007 - v1.1
      = corrected problems with nds files with loader
      = corrected problems with nds files made with standard libnds makefile
      + optimised variables, now use 505 bytes less in RAM
      + added EFS_Flush() function, to ensure data is written
      + included ASM code for reserved space

    14/05/2007 - v1.1a
      = corrected bug with EFS_fopen() when filename does not begin with "/"
      + added defines for c++ compatibility

    28/09/2007 - v1.2
      = corrected a major bug with EFS_fread and EFS_fwrite
      = moved some functions tweaks to fix real fat mode
      + added autoflush on file close by default
      + added extension checking when searching the NDS file (improve speed)

    12/01/2008 - v1.3
      = corrected EFS_GetFileSize() function when using real fat mode

    25/06/2008 - v2.0
      + complete rewrite of the lib (breaks compatibility with old versions!)
      + added full devoptab integration, so it now use standard iolib functions
      + added automatic GBA rom detection (so it works in GBA flash kits & emu
        without any modifications), based on Eris' NitroFS driver idea
      + full chdir and unix standard paths (relative/absolute) support
      + great speed improve

*/

#include <nds.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/iosupport.h>
#include <fat.h>

#include "efs_lib.h"

// defines
#define EFS_READBUFFERSIZE      4096
#define EFS_SEARCHFILE          0
#define EFS_SEARCHDIR           1
#define EFS_LISTDIR             2
#define EFS_GBALOADERSTR        "PASSDF"
#define EFS_STDLOADERSTR        "PASS01"
#define EFS_LOADERSTROFFSET     172
#define EFS_LOADEROFFSET        512
#define EFS_FNTOFFSET           64
#define EFS_FATOFFSET           72
#define EFS_NITROROOTID         0xF000
#define EFS_ISFILE              0
#define EFS_ISDIR               1
#define EFS_PREFIX              "efs:"
#define EFS_PREFIX_LEN          4
#define EFS_DEVICE              "efs:/"


// internal file/dir struct
typedef struct _EFS_FileStruct {
    u32 start;
    u32 pos;
    u32 end;
} EFS_FileStruct;

typedef struct _EFS_DirStruct {
    u16 dir_id;
    u16 curr_file_id;
    u32 pos;
} EFS_DirStruct;

// mix up the magic string to avoid doubling it in the NDS
const char efsMagicStringP2[] = " EFSstr";
const char efsMagicStringP1[] = "\xCE\x05\xA9\xBF";

// export variables
extern int efs_id;
extern u32 efs_filesize;

// globals
int nds_file;
u32 fnt_offset;
u32 fat_offset;
bool hasLoader;
bool useDLDI;
bool hasWritten;

char fileInNDS[EFS_MAXPATHLEN];
char currPath[EFS_MAXPATHLEN];
bool filematch;
u8 searchmode;
u32 file_idpos;
u32 file_idsize;

devoptab_t EFSdevoptab = {
    "efs",                      // const char *name;
    sizeof(EFS_FileStruct),     // int structSize;
    &EFS_Open,                  // int (*open_r)(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
    &EFS_Close,                 // int (*close_r)(struct _reent *r, int fd);
    &EFS_Write,                 // int (*write_r)(struct _reent *r, int fd, const char *ptr, int len);
    &EFS_Read,                  // int (*read_r)(struct _reent *r, int fd,char *ptr, int len);
    &EFS_Seek,                  // int (*seek_r)(struct _reent *r, int fd, int pos, int dir);
    &EFS_Fstat,                 // int (*fstat_r)(struct _reent *r, int fd, struct stat *st);
    &EFS_Stat,                  // int (*stat_r)(struct _reent *r, const char *file, struct stat *st);
    NULL,                       // int (*link_r)(struct _reent *r, const char *existing, const char *newLink);
    NULL,                       // int (*unlink_r)(struct _reent *r, const char *name);
    &EFS_ChDir,                 // int (*chdir_r)(struct _reent *r, const char *name);
    NULL,                       // int (*rename_r)(struct _reent *r, const char *oldName, const char *newName);
    NULL,                       // int (*mkdir_r)(struct _reent *r, const char *path, int mode);

    sizeof(EFS_DirStruct),      // int dirStateSize;
    &EFS_DirOpen,               // DIR_ITER* (*diropen_r)(struct _reent *r, DIR_ITER *dirState, const char *path);
    &EFS_DirReset,              // int (*dirreset_r)(struct _reent *r, DIR_ITER *dirState);
    &EFS_DirNext,               // int (*dirnext_r)(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
    &EFS_DirClose               // int (*dirclose_r)(struct _reent *r, DIR_ITER *dirState);
};


// search into NitroFS
void ExtractDirectory(const char *prefix, u32 dir_id) {
    char strbuf[EFS_MAXPATHLEN];
    u32 entry_start;    // reference location of entry name
    u16 top_file_id;    // file ID of top entry
    u32 file_id;
    u32 seek_pos = (useDLDI ? lseek(nds_file, 0, SEEK_CUR) : 0);    // save file position if using DLDI

    if(useDLDI) {
        lseek(nds_file, fnt_offset + 8*(dir_id & 0xFFF), SEEK_SET);
        read(nds_file, &entry_start, sizeof(entry_start));
        read(nds_file, &top_file_id, sizeof(top_file_id));

        if((searchmode == EFS_LISTDIR) && file_idpos) {
            lseek(nds_file, file_idpos, SEEK_SET);
            top_file_id = file_idsize;
            file_idsize = ~2;
        } else {
            lseek(nds_file, fnt_offset + entry_start, SEEK_SET);
        }
    } else {
        seek_pos = fnt_offset + 8*(dir_id & 0xFFF);
        memcpy(&entry_start, seek_pos + (u8*)GBAROM, sizeof(entry_start));
        memcpy(&top_file_id, seek_pos + sizeof(entry_start) + (u8*)GBAROM, sizeof(top_file_id));

        if((searchmode == EFS_LISTDIR) && file_idpos) {
            seek_pos = file_idpos;
            top_file_id = file_idsize;
            file_idsize = ~2;
        } else {
            seek_pos = fnt_offset + entry_start;
        }
    }

    for(file_id=top_file_id; !filematch; file_id++) {
        u8 entry_type_name_length;
        u32 name_length;
        bool entry_type_directory;
        char entry_name[EFS_MAXNAMELEN];

        if(useDLDI) {
            read(nds_file, &entry_type_name_length, sizeof(entry_type_name_length));
        } else {
            memcpy(&entry_type_name_length, seek_pos + (u8*)GBAROM, sizeof(entry_type_name_length));
            seek_pos += sizeof(entry_type_name_length);
        }

        name_length = entry_type_name_length & 127;
        entry_type_directory = (entry_type_name_length & 128) ? true : false;
        if(name_length == 0)
            break;

        memset(entry_name, 0, EFS_MAXNAMELEN);

        if(useDLDI) {
            read(nds_file, entry_name, entry_type_name_length & 127);
        } else {
            memcpy(entry_name, seek_pos + (u8*)GBAROM, entry_type_name_length & 127);
            seek_pos += entry_type_name_length & 127;
        }

        if(searchmode == EFS_LISTDIR) {
            strcpy(fileInNDS, entry_name);

            if(useDLDI) {
                file_idpos = lseek(nds_file, 0, SEEK_CUR);
            } else {
                file_idpos = seek_pos;
            }

            if(entry_type_directory) {
                file_idpos += 2;
                file_idsize = ~1;
            } else {
                u32 top;
                u32 bottom;
                u32 fpos = fat_offset + 8*file_id;

                if(useDLDI) {
                    lseek(nds_file, fpos, SEEK_SET);
                    read(nds_file, &top, sizeof(top));
                    read(nds_file, &bottom, sizeof(bottom));
                } else {
                    memcpy(&top, fpos + (u8*)GBAROM, sizeof(top));
                    memcpy(&bottom, fpos + sizeof(top) + (u8*)GBAROM, sizeof(bottom));
                }

                file_idsize = bottom - top;
            }
            filematch = true;
            break;
        }

        if(entry_type_directory) {
            u16 dir_id;

            if(useDLDI) {
                read(nds_file, &dir_id, sizeof(dir_id));
            } else {
                memcpy(&dir_id, seek_pos + (u8*)GBAROM, sizeof(dir_id));
                seek_pos += sizeof(dir_id);
            }

            strcpy(strbuf, prefix);
            strcat(strbuf, entry_name);
            strcat(strbuf, "/");

            if((searchmode == EFS_SEARCHDIR) && !stricmp(strbuf, fileInNDS)) {
                file_idpos = dir_id;
                file_idsize = file_id;
                filematch = true;
                break;
            }

            ExtractDirectory(strbuf, dir_id);

        } else if(searchmode == EFS_SEARCHFILE) {
            strcpy(strbuf, prefix);
            strcat(strbuf, entry_name);

            if (!stricmp(strbuf, fileInNDS)) {
                u32 top;
                u32 bottom;
                file_idpos = fat_offset + 8*file_id;

                if(useDLDI) {
                    lseek(nds_file, file_idpos, SEEK_SET);
                    read(nds_file, &top, sizeof(top));
                    read(nds_file, &bottom, sizeof(bottom));
                } else {
                    memcpy(&top, file_idpos + (u8*)GBAROM, sizeof(top));
                    memcpy(&bottom, file_idpos + sizeof(top) + (u8*)GBAROM, sizeof(bottom));
                }

                file_idsize = bottom - top;
                if(hasLoader)
                    top += 512;
                file_idpos = top;
                filematch = true;
                break;
            }
        }
    }

    // restore initial file position
    if(!filematch && useDLDI)
        lseek(nds_file, seek_pos, SEEK_SET);
}

// check if the file is the good one, and save the path if desired
bool CheckFile(char *path, bool save) {
    bool ok = false;
    char ext[7], *ext2 = ext + 2;
    int i, f;
    u32 size;

    // check file extension
    strcpy(ext, path + strlen(path) - 6);
    for(i=0; i<7; ++i)
        ext[i] = tolower(ext[i]);

    if(!strcmp(ext2, ".nds") || !strcmp(ext2, ".bin") || !strcmp(ext, "ds.gba")) {
        if((f = open(path, O_RDWR))) {
            // check file size
            size = lseek(f, 0, SEEK_END);

            if(size == efs_filesize) {
                bool found = false;
                char dataChunk[EFS_READBUFFERSIZE];
                char magicString[12] = "";
                int dataChunk_size;
                u32 efs_offset = 0;

                // rebuild magic string
                strcat(magicString, efsMagicStringP1);
                strcat(magicString, efsMagicStringP2);

                // search for magic string
                lseek(f, 0, SEEK_SET);
                while(efs_offset < size && !found) {
                    dataChunk_size = read(f, dataChunk, EFS_READBUFFERSIZE);

                    for(i=0; i < dataChunk_size; i++) {
                        if(dataChunk[i] == magicString[0]) {
                            if(dataChunk_size-i < 12) {
                                break;
                            }
                            if(memcmp(&dataChunk[i], magicString, 12) == 0) {
                                found = true;
                                efs_offset += i;
                                break;
                            }
                        }
                    }
                    if(!found) {
                        efs_offset += dataChunk_size;
                    }
                }

                // check file id
                if(found == true && (*(int*)(dataChunk+i+12) == efs_id)) {
                    strcpy(efs_path, path);
                    // store file path in NDS
                    if(save) {
                        // reopening of the file is needed to prevent an offset bug in some DLDI drivers
                        close(f);
                        f = open(path, O_RDWR);

                        // correct offset as neoflash DLDI driver has some issues (we know it's word-aligned)
                        if((efs_offset & 3) < 2)
                            efs_offset = efs_offset - (efs_offset & 3);
                        else
                            efs_offset = efs_offset + 4 - (efs_offset & 3);

                        lseek(f, efs_offset+20, SEEK_SET);
                        write(f, path, 256);
                    }
                    ok = true;
                }
            }
        }
        close(f);
    }

    return ok;
}

// search in directory for the NDS file
bool SearchDirectory() {
    DIR_ITER *dir;
    bool found = false;
    char path[EFS_MAXPATHLEN];
    char filename[EFS_MAXPATHLEN];
    struct stat st;

    dir = diropen(".");
    while((dirnext(dir, filename, &st) == 0) && (!found)) {
        if(st.st_mode & S_IFDIR) {
            if(((strlen(filename) == 1) && (filename[0]!= '.')) ||
                ((strlen(filename) == 2) && (strcasecmp(filename, "..")))  ||
                (strlen(filename) > 2))
            {
                chdir(filename);
                found = SearchDirectory();
                chdir("..");
            }
        } else {
            getcwd(path, EFS_MAXPATHLEN-1);
            strcat(path, filename);

            if(CheckFile(path, true)) {
                found = true;
                break;
            }
        }
    }
    dirclose(dir);

    return found;
}

// parse a unix-style path
void parsePath(const char *inputPath, char *outputPath, bool isDirectory) {

    char *curr = outputPath;

    // skip efs prefix
    if(strncmp(inputPath, EFS_PREFIX, EFS_PREFIX_LEN) == 0)
        inputPath += EFS_PREFIX_LEN;

    if(outputPath != currPath) {
        // all paths except the current path start with a '/'
        *curr++ = '/';
        *curr = '\0';

        // prepend current dir if needed
        if(inputPath[0] != '/' && *currPath) {
            strcat(curr, currPath);
            curr += strlen(currPath);
        }
    }

    while(*inputPath) {

        if(*inputPath == '.') {

            // handle '..'
            if(inputPath[1] == '.' && (!inputPath[2] || inputPath[2] == '/')) {
                // go back one directory
                *--curr = 0;
                while(curr >= outputPath && *curr != '/')
                    *curr-- = 0;

                curr++;
                inputPath += 2;

            } else {
                // if it's only '.'
                if(!inputPath[1] || inputPath[1] == '/') {
                    inputPath++; // skip it
                } else {
                    // it's just a name, copy string
                    while(*inputPath) {
                        *curr++ = *inputPath;

                        if(*inputPath++ == '/')
                            break;
                    }
                }
            }

        } else if(*inputPath != '/') {

            // copy string until next '/' or end
            while(*inputPath) {
                *curr++ = *inputPath;

                if(*inputPath++ == '/')
                    break;
            }

        } else {

            // skip next '/'
            inputPath++;
        }
    }

    // append final '/' if parsed path is a directory
    if(isDirectory && curr > outputPath && curr[-1] != '/')
        *curr++ = '/';

    // append null-terminating character
    *curr = '\0';
}

// initialize the file system
int EFS_Init(int options, char *path) {

    bool found = false;

    // reset current path
    memset(currPath, 0, EFS_MAXPATHLEN);

    // first try to init NitroFS from GBA mem
    sysSetBusOwners(BUS_OWNER_ARM9, BUS_OWNER_ARM9);    // take gba slot ownership

    if(strncmp(((const char*)GBAROM)+EFS_LOADERSTROFFSET, EFS_GBALOADERSTR, strlen(EFS_GBALOADERSTR)) == 0) {

        // there's a GBA loader here
        memcpy(&fnt_offset, EFS_FNTOFFSET+EFS_LOADEROFFSET + (u8*)GBAROM, sizeof(fnt_offset));
        memcpy(&fat_offset, EFS_FATOFFSET+EFS_LOADEROFFSET + (u8*)GBAROM, sizeof(fat_offset));
        fnt_offset += EFS_LOADEROFFSET;
        fat_offset += EFS_LOADEROFFSET;
        hasLoader = true;
        useDLDI = false;
        AddDevice(&EFSdevoptab);
        found = true;
        strcpy(efs_path, "GBA ROM");

    } else if(strncmp(((const char*)GBAROM)+EFS_LOADERSTROFFSET, EFS_STDLOADERSTR, strlen(EFS_STDLOADERSTR)) == 0) {

        // there's a standard nds loader here
        memcpy(&fnt_offset, EFS_FNTOFFSET + (u8*)GBAROM, sizeof(fnt_offset));
        memcpy(&fat_offset, EFS_FATOFFSET + (u8*)GBAROM, sizeof(fat_offset));
        hasLoader = false;
        useDLDI = false;
        AddDevice(&EFSdevoptab);
        found = true;
        strcpy(efs_path, "GBA ROM");

    } else {

        // if init from GBA mem failed go for DLDI I/O
        useDLDI = true;

        // init libfat if requested
        if(options & EFS_AND_FAT) {
            if(!fatInitDefault())
                return false;
        }

        // check if the provided path is valid
        if(path && CheckFile(path, true)) {
            found = true;
        } else {
            // check if there's already a path stored
            if(efs_path[0]) {
                if(CheckFile(efs_path, false)) {
                    found = true;
                } else {
                    efs_path[0] = '\0';
                }
            }

            // if no path is defined, search the whole FAT space
            if(!efs_path[0]) {
                chdir("/");
                if(SearchDirectory())
                    found = true;
            }
        }

        // if nds file is found, open it and read the header
        if(found) {
            char buffer[8];

            nds_file = open(efs_path, O_RDWR);

            // check for if a loader is present
            lseek(nds_file, EFS_LOADERSTROFFSET, SEEK_SET);
            read(nds_file, buffer, 6);
            buffer[7] = '\0';

            if(strcmp(buffer, EFS_GBALOADERSTR) == 0) {
                // loader present
                lseek(nds_file, EFS_LOADEROFFSET+EFS_FNTOFFSET, SEEK_SET);
                read(nds_file, &fnt_offset, sizeof(u32));
                lseek(nds_file, 4, SEEK_CUR);
                read(nds_file, &fat_offset, sizeof(u32));
                fnt_offset += EFS_LOADEROFFSET;
                fat_offset += EFS_LOADEROFFSET;
                hasLoader = true;
            } else {
                lseek(nds_file, EFS_FNTOFFSET, SEEK_SET);
                read(nds_file, &fnt_offset, sizeof(u32));
                lseek(nds_file, 4, SEEK_CUR);
                read(nds_file, &fat_offset, sizeof(u32));
                hasLoader = false;
            }

            AddDevice(&EFSdevoptab);
        }
    }

    // set as default device if requested
    if(found && (options & EFS_DEFAULT_DEVICE))
        chdir(EFS_DEVICE);      // works better than setDefaultDevice();

    return (found && (!useDLDI || (nds_file != -1)));
}

// open a file
int EFS_Open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    EFS_FileStruct *file = (EFS_FileStruct*)fileStruct;

    if(useDLDI && !nds_file)
        return -1;

    // search for the file in NitroFS
    filematch = false;
    searchmode = EFS_SEARCHFILE;
    file_idpos = 0;
    file_idsize = 0;

    // parse given path
    parsePath(path, fileInNDS, false);

    // search into NitroFS
    ExtractDirectory("/", EFS_NITROROOTID);

    if(file_idpos) {
        file->start = file_idpos;
        file->pos = file_idpos;
        file->end = file_idpos + file_idsize;
        return 0;
    }

    return -1;
}

// set the current position in the file
off_t EFS_Seek(struct _reent *r, int fd, off_t pos, int dir) {
    EFS_FileStruct *file = (EFS_FileStruct*)fd;
    switch(dir) {
        case SEEK_SET:
            file->pos = file->start + pos;
            break;
        case SEEK_CUR:
            file->pos += pos;
            break;
        case SEEK_END:
            file->pos = file->end + pos;
            break;
    }
    return (file->pos - file->start);
}

// read data from file
ssize_t EFS_Read(struct _reent *r, int fd, char *ptr, size_t len) {
    EFS_FileStruct *file = (EFS_FileStruct*)fd;

    if(file->pos+len > file->end)
        len = file->end - file->pos;    // don't read past the end of the file
    if(file->pos > file->end)
        return 0;   // EOF

    if (useDLDI) {
        // seek to right position and read data
        lseek(nds_file, file->pos, SEEK_SET);
        len = read(nds_file, ptr, len);
    } else {
        memcpy(ptr, file->pos + (u8*)GBAROM, len);
    }
    if (len > 0)
        file->pos += len;

    return len;
}

// write data to file (only works using DLDI)
ssize_t EFS_Write(struct _reent *r, int fd, const char *ptr, size_t len) {
    EFS_FileStruct *file = (EFS_FileStruct*)fd;

    if(file->pos+len > file->end)
        len = file->end - file->pos;    // don't write past the end of the file
    if(file->pos > file->end)
        return 0;   // EOF

    if (useDLDI) {
        // seek to right position and write data
        lseek(nds_file, file->pos, SEEK_SET);
        len = write(nds_file, ptr, len);
        if (len > 0)
            file->pos += len;
        hasWritten = true;
    } else {
        return 0;
    }

    return len;
}

// close current file
int EFS_Close(struct _reent *r, int fd) {
    // flush writes in the file system
    if(useDLDI && hasWritten) {
        if(nds_file)
            close(nds_file);
        nds_file = open(efs_path, O_RDWR);
    }
    return 0;
}

// open a directory
DIR_ITER* EFS_DirOpen(struct _reent *r, DIR_ITER *dirState, const char *path) {
    EFS_DirStruct *dir = (EFS_DirStruct*)dirState->dirStruct;

    if(useDLDI && !nds_file)
        return NULL;

    // search for the directory in NitroFS
    filematch = false;
    searchmode = EFS_SEARCHDIR;
    file_idpos = ~1;
    file_idsize = 0;

    // parse given path
    parsePath(path, fileInNDS, true);

    // are we trying to list the root path?
    if(!strcmp(fileInNDS, "/"))
        file_idpos = EFS_NITROROOTID;
    else
        ExtractDirectory("/", EFS_NITROROOTID);

    if (file_idpos != ~1u) {
        dir->dir_id = file_idpos;
        dir->curr_file_id = file_idsize;
        dir->pos = ~1;
        return dirState;
    }

    return NULL;
}

// go back to the beginning of the directory
int EFS_DirReset(struct _reent *r, DIR_ITER *dirState) {
    EFS_DirStruct *dir = (EFS_DirStruct*)dirState->dirStruct;

    if(useDLDI) {
        lseek(nds_file, fnt_offset + 8*(dir->dir_id & 0xFFF) + 4, SEEK_SET);
        read(nds_file, &dir->curr_file_id, sizeof(dir->curr_file_id));
    } else {
        memcpy(&dir->curr_file_id, fnt_offset + 8*(dir->dir_id & 0xFFF) + 4 + (u8*)GBAROM, sizeof(dir->curr_file_id));
    }
    dir->pos = ~1;
    return 0;
}

// read next entry of the directory
int EFS_DirNext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *st) {
    EFS_DirStruct *dir = (EFS_DirStruct*)dirState->dirStruct;

    if(useDLDI && !nds_file)
        return -1;

    // special case for ".." and "."
    if(dir->pos == ~1u || dir->pos == ~0u) {
        strcpy(filename, ".");

        if(dir->pos == ~0u)
            strcat(filename, ".");

        st->st_mode = S_IFDIR;
        dir->pos++;

        return 0;

    } else {

        // search for the file in NitroFS
        filematch = false;
        searchmode = EFS_LISTDIR;
        file_idpos = dir->pos;
        file_idsize = dir->curr_file_id;

        ExtractDirectory("", dir->dir_id);

        if(file_idsize != ~2u) {
            strcpy(filename, fileInNDS);
            dir->pos = file_idpos;
            dir->curr_file_id++;

            if(file_idsize != ~1u) {
                st->st_mode = 0;
                st->st_size = file_idsize;
            } else {
                st->st_mode = S_IFDIR;
            }

            return 0;
        }
    }

    return -1;
}

// get some info on a file
int EFS_Fstat(struct _reent *r, int fd, struct stat *st) {
    EFS_FileStruct *file = (EFS_FileStruct*)fd;
    st->st_size = file->end - file->start;
    // maybe add some other info?
    return 0;
}

// get some info on a file from a path
int EFS_Stat(struct _reent *r, const char *file, struct stat *st) {
    EFS_FileStruct fs;

    if(EFS_Open(NULL, &fs, file, 0, 0))
        return -1;

    st->st_size = fs.end - fs.start;
    // maybe add some other info?
    return 0;
}

// close a directory
int EFS_DirClose(struct _reent *r, DIR_ITER *dirState) {
    return 0;
}

// change current directory
int EFS_ChDir(struct _reent *r, const char *name) {

    if(!name)
        return -1;

    // parse given path
    parsePath(name, currPath, true);

    return 0;
}



// reserved space for post-compilation adding of EFS tags
asm (
"@--------------------------------------------------------------------------------------\n"
"   .balign    32                                                                       \n"
"   .arm                                                                                \n"
"@--------------------------------------------------------------------------------------\n"
"   .word   0xBFA905CE      @ Magic number to identify this region                      \n"
"   .asciz  \" EFSstr\"     @ Identifying Magic string (8 bytes with null terminator)   \n"
"   .global efs_id                                                                      \n"
"efs_id:                                                                                \n"
"   .word   0x00            @ ID of the rom                                             \n"
"   .global efs_filesize                                                                \n"
"efs_filesize:                                                                          \n"
"   .word   0x00            @ Size of the rom                                           \n"
"   .global efs_path                                                                    \n"
"efs_path:                                                                              \n"
"   .skip   256             @ Path of the rom                                           \n"
"@--------------------------------------------------------------------------------------\n"
);
