// filename ************** eFile.c *****************************
// Middle-level routines to implement a solid-state disk 
// Thomas Brezinski & Zack Lalanne 3/20/12

#include "edisk.h"
#include "ff.h"
#include "UART.h"
#include "diskio.h"
#include "uartstdio.h"

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary
// data from the SD card.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
static char g_cCmdBuf[CMD_BUF_SIZE];


//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;


//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){
   FRESULT fresult;
   fresult = f_mount(0, &g_sFatFs);
   if(fresult != FR_OK){
      UARTprintf("Init Error: %s\n", StringFromFresult(fresult));
	  return(1);
   }
   return 0;
}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void) {
   FRESULT fresult;
   fresult = f_mkfs(0,0,255);	// !!! all arguments must be bytes, so smaller sectors?
   if(fresult != FR_OK){
      UARTprintf("Format Error: %s\n", StringFromFresult(fresult));
      return(1);
   }
   return 0;
}

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[]){
   FRESULT fresult;
   fresult = f_open(&g_sFileObject, name, FA_CREATE_NEW);
   if(fresult != FR_OK){
      UARTprintf("Create Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){
   FRESULT fresult;
   fresult = f_open(&g_sFileObject, name, FA_WRITE);
   if (fresult != FR_OK){
      UARTprintf("WOpen Error: %s\n", StringFromFresult(fresult));;
	  return 1;
   }
   return 0;
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data){
   unsigned	short BytesWritten;
   FRESULT fresult;
   fresult = f_write(&g_sFileObject, (const void *)data, 1, &BytesWritten);
   if (fresult != FR_OK){
      UARTprintf("Write Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;  
}

//---------- eFile_Close-----------------
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){
   FRESULT fresult;
   fresult = f_mount(0, '\0');
   if (fresult != FR_OK){
      UARTprintf("Close Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;  
}



//---------- eFile_WClose-----------------
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){
   FRESULT fresult;
   fresult = f_close(&g_sFileObject);
   if (fresult != FR_OK){
      UARTprintf("WClose Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;  
}

//---------- eFile_ROpen-----------------
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){
   FRESULT fresult;
   fresult = f_open(&g_sFileObject, name, FA_READ);
   if (fresult != FR_OK) {
      UARTprintf("ROpen Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;
}
   
//---------- eFile_ReadNext-----------------
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){
   unsigned short BytesRead;
   FRESULT fresult;
   fresult = f_read(&g_sFileObject, pt, 1, &BytesRead);
   if (fresult != FR_OK){
      UARTprintf("ReadNext Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0;  
}

                              
//---------- eFile_RClose-----------------
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){
   FRESULT fresult;
   fresult = f_close(&g_sFileObject);
   if (fresult != FR_OK){
      UARTprintf("RClose Error: %s\n", StringFromFresult(fresult));
	  return 1;
   }
   return 0; 
}

//---------- eFile_Directory-----------------
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: characters returned by reference
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(unsigned char)){
    unsigned long ulTotalSize;
    unsigned long ulFileCount;
    unsigned long ulDirCount;
    FRESULT fresult;
    FATFS *pFatFs;

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    UARTprintf("\n");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            return(fresult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // If the attribue is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ulFileCount++;
            ulTotalSize += g_sFileInfo.fsize;
        }

        //
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        UARTprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                    (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                    (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                    (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                    (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                    (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                    (g_sFileInfo.fdate >> 9) + 1980,
                    (g_sFileInfo.fdate >> 5) & 15,
                     g_sFileInfo.fdate & 31,
                    (g_sFileInfo.ftime >> 11),
                    (g_sFileInfo.ftime >> 5) & 63,
                     g_sFileInfo.fsize,
                     g_sFileInfo.fname);
    }   // endfor

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    UARTprintf("\n%4u File(s),%10u bytes total\n%4u Dir(s)",
                ulFileCount, ulTotalSize, ulDirCount);

    //
    // Get the free space.
    //
    fresult = f_getfree("/", &ulTotalSize, &pFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    //
    // Display the amount of free space that was calculated.
    //
    UARTprintf(", %10uK bytes free\n", ulTotalSize * pFatFs->sects_clust / 2);

    //
    // Made it to here, return with no errors.
    //
    return(0);
}   

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){
   FRESULT fresult;
   fresult = f_unlink(name);
   if(fresult != FR_OK){
      UART0_SendString("Error deleting file: ");
	  UART0_SendString((char *)StringFromFresult(fresult));
	  UART0_SendString("\n");
	  return 1;
   }
   return 0;
}

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
