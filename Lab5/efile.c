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
unsigned long gFSTimerCount = 0;
 

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
     
	 // Open the file
	 fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_WRITE);
	 if (fresult != FR_OK){
      UARTprintf("WOpen Error: %s\n\r", StringFromFresult(fresult));;
	  return 1;
     }

	 // By default, write to the end of the file
	 fresult = f_lseek(&g_sFileObject, g_sFileObject.fsize);
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

//---------- eFile_ChangeDirectory-----------------
// Changes the current working directory
// Input: name of directory to change to
// Output: 0 if successful and 1 on failure
int eFile_ChangeDirectory(char directory[]){
    
	unsigned int uIdx;
    FRESULT fresult;

    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    strcpy(g_cTmpBuf, g_cCwdBuf);

    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    if(directory[0] == '/')
    {
        // Make sure the new path is not bigger than the cwd buffer.
        if(strlen(directory) + 1 > sizeof(g_cCwdBuf))
        {
            UARTprintf("Resulting path name is too long\n");
            return(0);
        } else
        {
            strncpy(g_cTmpBuf, directory, sizeof(g_cTmpBuf));
        }
    }

    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    else if(!strcmp(directory, ".."))
    {
        // Get the index to the last character in the current path.
        uIdx = strlen(g_cTmpBuf) - 1;

        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        while((g_cTmpBuf[uIdx] != '/') && (uIdx > 1))
        {
            // Back up one character.
            uIdx--;
        }

        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        g_cTmpBuf[uIdx] = 0;
    } else {
        // Test to make sure that when the new additional path is
        // added on to the current path, there is room in the buffer
        // for the full new path.  It needs to include a new separator,
        // and a trailing null character.
        if(strlen(g_cTmpBuf) + strlen(directory) + 1 + 1 > sizeof(g_cCwdBuf))
        {
            UARTprintf("Resulting path name is too long\n");
            return(0);
        } else {
            
			// If not already at the root level, then append a /
            if(strcmp(g_cTmpBuf, "/"))
            {
                strcat(g_cTmpBuf, "/");
            }

            // Append the new directory to the path.
            strcat(g_cTmpBuf, directory);
        }
    }

    // At this point, a candidate new directory path is in chTmpBuf.
    // Try to open it to make sure it is valid.
    fresult = f_opendir(&g_sDirObject, g_cTmpBuf);

    // If it cant be opened, then it is a bad path.  Inform
    // user and return.
    if(fresult != FR_OK)
    {
        UARTprintf("cd: %s\n", g_cTmpBuf);
        return(fresult);
    } else
    {
        strncpy(g_cCwdBuf, g_cTmpBuf, sizeof(g_cCwdBuf));
    }

    // Return success.
    return(0); 
}

//---------- eFile_PrintWorkingDirectory-----------------
// prints the working directory
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_PrintWorkingDirectory(void)
{
    // Print the CWD to the console.
    UARTprintf("%s\n\r", g_cCwdBuf);

    // Return success.
    return(0);
}

//---------- eFile_ReadEntireFile-----------------
// prints a file
// Input: none
// Output: 0 if successful and 1 on failure
int eFile_ReadEntireFile(char file[])
{
    FRESULT fresult;
    unsigned short usBytesRead;

    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    if(strlen(g_cCwdBuf) + strlen(file) + 1 + 1 > sizeof(g_cTmpBuf))
    {
        UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    // Copy the current path to the temporary buffer so it can be manipulated.
    strcpy(g_cTmpBuf, g_cCwdBuf);

    // If not already at the root level, then append a separator.
    if(strcmp("/", g_cCwdBuf))
    {
        strcat(g_cTmpBuf, "/");
    }

    // Now finally, append the file name to result in a fully specified file.
    strcat(g_cTmpBuf, file);

    // Open the file for reading.
    fresult = f_open(&g_sFileObject, g_cTmpBuf, FA_READ);

    // If there was some problem opening the file, then return
    // an error.
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    do
    {
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        fresult = f_read(&g_sFileObject, g_cTmpBuf, sizeof(g_cTmpBuf) - 1,
                         &usBytesRead);

        // If there was an error reading, then print a newline and
        // return the error to the user.
        if(fresult != FR_OK)
        {
            UARTprintf("\n");
            return(fresult);
        }

        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printf.
        g_cTmpBuf[usBytesRead] = 0;

        // Print the last chunk of the file that was received.
        UARTprintf("%s", g_cTmpBuf);

    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    } while(usBytesRead == sizeof(g_cTmpBuf) - 1);

    // Return success.
    return(0);
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

//---------- eFile_GetFSTime-----------------
// returns the number of interrupts of FS counter
// Input: none
// Output: value of gFSTimerCount
unsigned long eFile_GetFSTime(void) {
  return gFSTimerCount;
}

//---------- eFile_ClearFSTime-----------------
// clears the number of interrupts of FS counter
// Input: none
// Output: none
void eFile_ClearFSTime(void) {
  gFSTimerCount = 0;
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
	gFSTimerCount++;
	TimerIntClear(TIMER1_BASE, TIMER_TIMB_TIMEOUT);
}

