/*

  Embedded File System (EFS)
  --------------------------

  file        : efs.c 
  author      : Noda, based on the source code of dlditool by Chishm
  description : EFS unique ID generator & patcher

  history : 

    02/05/2007 - v1.0
      * Original release


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    
*/

#define VERSION "v1.0"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/param.h>
#else
#define MAXPATHLEN      1024
#endif

#include <sys/stat.h>

#ifndef bool
typedef enum {false = 0, true = !0} bool;
#endif

typedef signed int addr_t;
typedef unsigned char data_t;

const data_t efsMagicString[] = "\xCE\x05\xA9\xBF EFSstr";

void printUsage (char* programName) {
    printf ("Usage:\n");
    printf ("%s <nds>\n", programName);
    printf ("    <nds>   the NDS application binary to patch in a unique EFS ID\n");
    return;
}

addr_t quickFind (const data_t* data, const data_t* search, size_t dataLen, size_t searchLen) {
    const int* dataChunk = (const int*) data;
    int searchChunk = ((const int*)search)[0];
    addr_t i;
    addr_t dataChunkEnd = (addr_t)(dataLen / sizeof(int));

    for ( i = 0; i < dataChunkEnd; i++) {
        if (dataChunk[i] == searchChunk) {
            if ((i*sizeof(int) + searchLen) > dataLen) {
                return -1;
            }
            if (memcmp (&data[i*sizeof(int)], search, searchLen) == 0) {
                return i*sizeof(int);
            }
        }
    }

    return -1;
}


int main(int argc, char* argv[])
{
    char *appFileName = NULL;
    addr_t patchOffset;            // Position of patch destination in the file
    FILE* appFile;
    data_t *pAH;
    data_t *appFileData = NULL;
    size_t appFileSize = 0;
    int i, new_id;

    printf ("*** Embedded File System ID patch tool " VERSION " by Noda\n");
    printf ("based on dlditool source by Michael Chisholm (Chishm)\n\n");

    for (i = 1; i < argc; i++) {
        if (appFileName == NULL) {
            appFileName = (char*) malloc (strlen (argv[i]) + 1);
            if (!appFileName) {
                return -1;
            }
            strcpy (appFileName, argv[i]);
        } else {
            printUsage (argv[0]);
            return -1;
        }
    }

    if ((appFileName == NULL)) {
        printUsage (argv[0]);
        return -1;
    }

    if (!(appFile = fopen (appFileName, "rb+"))) {
        printf ("Cannot open \"%s\" - %s\n", appFileName, strerror(errno));
        return -1;
    }

    // Load the app file
    fseek (appFile, 0, SEEK_END);
    appFileSize = ftell(appFile);
    appFileData = (data_t*) malloc (appFileSize);
    fseek (appFile, 0, SEEK_SET);

    if (!appFileData) {
        fclose (appFile);
        if (appFileData) free (appFileData);
        printf ("Out of memory\n");
        return -1;
    }

    fread (appFileData, 1, appFileSize, appFile);

    // Find the EFS reserved space in the file
    patchOffset = quickFind (appFileData, efsMagicString, appFileSize, sizeof(efsMagicString)/sizeof(char));

    if (patchOffset < 0) {
        printf ("%s does not have an EFS section\n", appFileName);
        return -1;
    }

    pAH = &appFileData[patchOffset];
    
    // Generate a random ID
    srand (time(NULL));
    new_id = rand();
    
    // Write EFS ID & filesize
    fseek (appFile, patchOffset+12, SEEK_SET);
    fwrite (&new_id, sizeof(int), 1, appFile);
    fwrite (&appFileSize, sizeof(int), 1, appFile);
    fclose (appFile);

    printf ("EFS section address:  0x%08X\n", patchOffset);
    printf ("\n");
    printf ("Old EFS ID:         %d\n", *(int*)(pAH+12));
    printf ("New EFS ID:         %d\n", new_id);
    printf ("\n");
    printf ("Old EFS filesize:   %d\n", *(unsigned int*)(pAH+16));
    printf ("New EFS filesize:   %d\n", appFileSize);
    printf ("\n");

    free (appFileData);

    printf ("Patched successfully\n");

    return 0;
}

