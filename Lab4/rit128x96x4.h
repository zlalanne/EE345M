// Modified By:
// Thomas Brezinski	TCB567
// Zachary Lalanne ZLL67
// TA: Zahidul Haq
// Date of last change: 2/24/2012

// Modified By:
// Megan Ruthven MAR3939
// Zachary Lalanne ZLL67
// TA: NACHI
// Date of last change: 9/19/2011

//*****************************************************************************
//
// rit128x96x4.h - Prototypes for the driver for the RITEK 128x96x4 graphical
//                   OLED display.
//
// Copyright (c) 2007-2010 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 6075 of the EK-LM3S1968 Firmware Package.
//
//*****************************************************************************

#ifndef __RIT128X96X4_H__
#define __RIT128X96X4_H__

//*****************************************************************************
//
// Prototypes for the driver APIs.
//
//*****************************************************************************
extern void RIT128x96x4Clear(void);
extern void RIT128x96x4StringDraw(const char *pcStr,
                                    unsigned long ulX,
                                    unsigned long ulY,
                                    unsigned char ucLevel);
extern void RIT128x96x4ImageDraw(const unsigned char *pucImage,
                                   unsigned long ulX,
                                   unsigned long ulY,
                                   unsigned long ulWidth,
                                   unsigned long ulHeight);
extern void RIT128x96x4Init(unsigned long ulFrequency);
extern void RIT128x96x4Enable(unsigned long ulFrequency);
extern void RIT128x96x4Disable(void);
extern void RIT128x96x4DisplayOn(void);
extern void RIT128x96x4DisplayOff(void);

extern void RIT128x96x4DrawLine(int startx, int starty, int endx, int endy);
extern void RIT128x96x4DrawDot(unsigned long xpos, unsigned long ypos);

// *************** RIT128x96x4PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// It does not output to display until RIT128x96x4ShowPlot called
// Inputs: ymin and ymax are range of the plot
// four numbers are displayed along left edge of plot
// y0,y1,y2,y3, can be -9 to 99, any number outside this range is skipped
// y3 --          hash marks at number           Ymax
//     |
//    --          hash marks between numbers     Ymin+(5*Yrange)/6
//     |
// y2 --                                         Ymin+(4*Yrange)/6
//     |
//    --                                         Ymin+(3*Yrange)/6
//     |
// y1 --                                         Ymin+(2*Yrange)/6
//     |
//    --                                         Ymin+(1*Yrange)/6
//     |
// y0 --                                         Ymin
// Outputs: none
extern void RIT128x96x4PlotClear(long ymin, long ymax, long y0, long y1, long y2, long y3);
extern void RIT128x96x4PlotReClear(void);

/****************RIT128x96x4DecOut2***************
 output 2 digit signed integer number to ASCII string
 format signed 32-bit 
 range -9 to 99
 Input: signed 32-bit integer, position, level 
 Output: none
 Examples
  82  to "82"
  1   to " 1" 
 -3   to "-3" 
 */ 
extern void RIT128x96x4DecOut2(unsigned long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);

extern void RIT128x96x4UDecOut3(unsigned long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);
/****************Int2Str***************
 converts signed integer number to ASCII string
 format signed 32-bit 
 range -99999 to +99999
 Input: signed 32-bit integer 
 Output: null-terminated string exactly 7 characters plus null
 Examples
  12345 to " 12345"  
 -82100 to "-82100"
   -102 to "  -102" 
     31 to "    31" 
 */ 

/****************Int2Str2***************
 converts signed integer number to ASCII string
 format signed 32-bit 
 range -9 to 99
 Input: signed 32-bit integer 
 Output: null-terminated string exactly 2 characters plus null
 Examples
  82  to "82"
  1   to " 1" 
 -3   to "-3" 
 */ 
extern void Int2Str2(long const n, char *string);

extern void RIT128x96x4PlotPoint(long y);
extern void RIT128x96x4PlotBar(long y);
extern void RIT128x96x4PlotdBfs(long y);
extern void RIT128x96x4PlotNext(void);
extern void RIT128x96x4ShowPlot(void);
extern void RIT128x96x4PlotClearFreq(void);
extern void RIT128x96x4ShowPlotFreq(void);
extern void RIT128x96x4PlotNextFreq(void);


#endif // __RIT128X96X4_H__
