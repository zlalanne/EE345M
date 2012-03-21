// filename ************** eFile.c *****************************
// Middle-level routines to implement a solid-state disk 
// Thomas Brezinski & Zack Lalanne 3/20/12

#include "edisk.h"

#define DIRENTRYSIZE 16
#define DIRNAMESIZE 12

struct DirEntry {
   char Name[12];
   unsigned long Sector;
};	// each entry is 16 bytes so there can be 32 entries

typedef struct DirEntry DirEntryType;

// the last 32 bits of each sector will be a pointer to the next sector linked, or 0 if it is the last sector in that list


//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){
   DSTATUS result = eDisk_Init(0); // init drive 0, only drive 0 supported
   if (result) {
      return 1;
   }
   return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void) {
   int i;
   DSTATUS result;
   BYTE block[512] = {0};
   DirEntryType FreeMem = {"Free", 1};
   // write the directory block of the sd card to all 0's because there are currently no entries
   for (i = 0; i < DIRNAMESIZE; i++) {
      block[495 + i] = FreeMem.Name[i];
   }
   block[495 + DIRNAMESIZE] = (BYTE)((FreeMem.Sector && 0xFF000000) >> 24);
   block[495 + DIRNAMESIZE + 1] = (BYTE)((FreeMem.Sector && 0x00FF0000) >> 16);
   block[495 + DIRNAMESIZE + 2] = (BYTE)((FreeMem.Sector && 0x0000FF00) >> 8);
   block[495 + DIRNAMESIZE + 3] = (BYTE)(FreeMem.Sector && 0x000000FF);
   result = eDisk_WriteBlock(block, 0);
   if (result) {
      return 1;
   }
   return 0;
}

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]);  // create new file, make it empty 


//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]);      // open a file for writing 

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data);  

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void); 


//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void); // close the file for writing

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]);      // open a file for reading 
   
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt);       // get next byte 
                              
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void); // close the file for writing

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(unsigned char));   

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]);  // remove this file 

//---------- eFile_RedirectToFile-----------------
// open a file for writing 
// Input: file name is a single ASCII letter
// stream printf data into file
// Output: 0 if successful and 1 on failure (e.g., trouble read/write to flash)
int eFile_RedirectToFile(char *name);

//---------- eFile_EndRedirectToFile-----------------
// close the previously open file
// redirect printf data back to UART
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_EndRedirectToFile(void);
