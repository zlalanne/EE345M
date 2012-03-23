// filename ************** eFile.c *****************************
// Middle-level routines to implement a solid-state disk 
// Thomas Brezinski & Zack Lalanne 3/20/12

#include <stdio.h>
#include <string.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "edisk.h"
#include "ff.h"
#include "UART.h"
#include "diskio.h"
#include "uartstdio.h"

// Defines the size of the buffers that hold the path, or temporary
// data from the SD card.  There are two buffers allocated of this size.
// The buffer size must be large enough to hold the longest expected
// full path name, including the file name, and a trailing null character.
#define PATH_BUF_SIZE   80

// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";

// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
static char g_cTmpBuf[PATH_BUF_SIZE];

// The following are data structures used by FatFs.
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

int StreamToFile = 0;  // 0=UART, 1=stream to file 

// ******** generatePath ************
// Generates the current path for opening files
int generatePath(char file[]) {
  
    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    
	if(strlen(g_cCwdBuf) + strlen(file) + 1 + 1 > sizeof(g_cTmpBuf)) {
        UARTprintf("Resulting path name is too long\n");
        return(1);
    }

    // Copy the current path to the temporary buffer so it can be manipulated.
    strcpy(g_cTmpBuf, g_cCwdBuf);

    // If not already at the root level, then append a separator.
    if(strcmp("/", g_cCwdBuf)) {
        strcat(g_cTmpBuf, "/");
    }

    // Now finally, append the file name to result in a fully specified file.
    strcat(g_cTmpBuf, file);

    // Return success.
    return(0);
}

//---------- eFile_Init-----------------
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
// since this program initializes the disk, it must run with 
//    the disk periodic task operating
int eFile_Init(void){

   FRESULT fresult;
   
  // Enabling SSIO
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);


   fresult = f_mount(0, &g_sFatFs);
   if(fresult != FR_OK){
      UARTprintf("Init Error: %s\n\r", StringFromFresult(fresult));
	  return(1);
   }

  // Initialize Timer1B: Used for sleep decrementing
  // Need to call OS_Init before running eFile_Init
  TimerDisable(TIMER1_BASE, TIMER_B);
  TimerIntDisable(TIMER1_BASE, TIMER_TIMB_TIMEOUT);
  TimerPrescaleSet(TIMER1_BASE, TIMER_B, 16); 
  TimerLoadSet(TIMER1_BASE, TIMER_B, 31250); // Every interrupt is 10ms
  TimerIntClear(TIMER1_BASE, TIMER_TIMB_TIMEOUT);
  TimerIntEnable(TIMER1_BASE, TIMER_TIMB_TIMEOUT);
  TimerEnable(TIMER1_BASE, TIMER_B);
  IntEnable(INT_TIMER1B);

  // Priority 2	interrupt
  IntPrioritySet(INT_TIMER1B, (0x02 <<5));

  return 0;

}

//---------- eFile_Format-----------------
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void) {
   FRESULT fresult;

   // Third argument is allocation unit size
   fresult = f_mkfs(0,0,32);
   if(fresult != FR_OK){
      UARTprintf("Format Error: %s\n\r", StringFromFresult(fresult));
      return(1);
   }
   return 0;
}

//---------- eFile_Create-----------------
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create(char name[]){
   
   FRESULT fresult;
   
   if(!generatePath(name)){
     fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_CREATE_NEW);
   
     if(fresult != FR_OK){
       if(fresult != FR_EXIST){
         UARTprintf("Create Error: %s\n\r", StringFromFresult(fresult));
	     return 1;
	   }
     }
	 return 0; 
  } else {
    // Could not generate path
    return 1;
    }
}

//---------- eFile_WOpen-----------------
// Open the file, read into RAM last block
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){
   
   FRESULT fresult;
   if(!generatePath(name)){
     
	 fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_WRITE);
     
	 if (fresult != FR_OK){
      UARTprintf("WOpen Error: %s\n\r", StringFromFresult(fresult));;
	  return 1;
     }

     return 0;
  } else {
    // Could not generate path
    return 1;
  }
}

//---------- eFile_Write-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write( char data){

   unsigned	short BytesWritten;
   FRESULT fresult;
   fresult = f_write(&g_sFileObject, &data, 1, &BytesWritten);
   if (fresult != FR_OK){
      UARTprintf("Write Error: %s\n\r", StringFromFresult(fresult));
	  return 1;
   }
   return 0;  
}

//---------- eFile_WriteString-----------------
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WriteString(char data[]){
   unsigned	short BytesWritten;
   FRESULT fresult;
   fresult = f_write(&g_sFileObject, data, sizeof(data), &BytesWritten);
   if (fresult != FR_OK){
      UARTprintf("Write Error: %s\n\r", StringFromFresult(fresult));
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
      UARTprintf("Close Error: %s\n\r", StringFromFresult(fresult));
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
      UARTprintf("WClose Error: %s\n\r", StringFromFresult(fresult));
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
  if(!generatePath(name)){
     
	 fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_READ);
     
	 if (fresult != FR_OK){
      UARTprintf("WOpen Error: %s\n\r", StringFromFresult(fresult));;
	  return 1;
     }

     return 0;
  } else {
    // Could not generate path
    return 1;
  }
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
      UARTprintf("ReadNext Error: %s\n\r", StringFromFresult(fresult));
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
      UARTprintf("RClose Error: %s\n\r", StringFromFresult(fresult));
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

    // Open the current directory for access.
    fresult = f_opendir(&g_sDirObject, g_cCwdBuf);

    // Check for error and return if there is a problem.
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;

    // Give an extra blank line before the listing.
    UARTprintf("\n\r");

    // Enter loop to enumerate through all directory entries.
    for(;;)
    {
        // Read an entry from the directory.
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        // Check for error and return if there is a problem.
        if(fresult != FR_OK)
        {
            return(fresult);
        }

        // If the file name is blank, then this is the end of the
        // listing.
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        // If the attribue is directory, then increment the directory count.
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        else
        {
            ulFileCount++;
            ulTotalSize += g_sFileInfo.fsize;
        }

        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
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
    }

    // Print summary lines showing the file, dir, and size totals.
    UARTprintf("\n\r%4u File(s),%10u bytes total\n\r%4u Dir(s)",
                ulFileCount, ulTotalSize, ulDirCount);

    // Get the free space.
    fresult = f_getfree("/", &ulTotalSize, &pFatFs);

    // Check for error and return if there is a problem.
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    // Display the amount of free space that was calculated.
    UARTprintf(", %10uK bytes free\n\r", ulTotalSize * pFatFs->sects_clust / 2);

    // Made it to here, return with no errors.
    return(0);
}   

//---------- eFile_Delete-----------------
// delete this file
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){
   
   FRESULT fresult;
   if(!generatePath(name)){
     
	 fresult = f_unlink(g_cTmpBuf);
     if(fresult != FR_OK){
      UART0_SendString("Error deleting file: ");
	  UART0_SendString((char *)StringFromFresult(fresult));
	  UART0_SendString("\n\r");
	  return 1;
     }

     return 0;

  } else {
    // Could not generate path
    return 1;
  }   
}

//---------- eFile_RedirectToFile-----------------
// open a file for writing 
// Input: file name is a single ASCII letter
// stream printf data into file
// Output: 0 if successful and 1 on failure (e.g., trouble read/write to flash)
int eFile_RedirectToFile(char *name){ 
  eFile_Create(name);              // ignore error if file already exists 
  if(eFile_WOpen(name)) return 1;  // cannot open file 
  StreamToFile = 1; 
  return 0; 
}

//---------- eFile_EndRedirectToFile-----------------
// close the previously open file
// redirect printf data back to UART
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_EndRedirectToFile(void){ 
  StreamToFile = 0; 
  if(eFile_WClose()) return 1;    // cannot close file 
  return 0; 
}
 
int fputc (int ch, FILE *f) {  
  if(StreamToFile){ 
    if(eFile_Write(ch)){          // close file on error 
       eFile_EndRedirectToFile(); // cannot write to file 
       return 1;                  // failure 
    } 
    return 0; // success writing 
  }
   					
  // regular UART output 
  UART0_OutChar(ch); 
  return 0;  
}

// ******** Timer1B_Handler ************
// Keeps time for the Fat File System 
void Timer1B_Handler(void) {
    // Call the FatFs tick timer.
    disk_timerproc();
	TimerIntClear(TIMER1_BASE, TIMER_TIMB_TIMEOUT);
}

