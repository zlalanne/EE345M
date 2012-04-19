; Modified By:
; Thomas Brezinski	TCB567
; Zachary Lalanne ZLL67
; TA: Zahidul Haq
; Date of last change: 2/24/2012


;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;// Real Time Operating System 

; This example accompanies the book
;  "Embedded Systems: Real Time Interfacing to the Arm Cortex M3",
;  ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2011
;
;  Programs 6.4 through 6.12, section 6.2
;
;Copyright 2011 by Jonathan W. Valvano, valvano@mail.utexas.edu
;    You may use, edit, run or distribute this file
;    as long as the above copyright notice remains
; THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
; OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; For more information about my classes, my research, and my books, see
; http://users.ece.utexas.edu/~valvano/
; */

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  RunPt            ; currently running thread
		EXTERN  NextPt           ; next thread to run

        EXPORT  OS_DisableInterrupts
        EXPORT  OS_EnableInterrupts
        EXPORT  StartCritical
        EXPORT  EndCritical
        EXPORT  StartOS
        EXPORT  PendSV_Handler


;*********** OS_DisableInterrupts ***************
; disable interrupts
; inputs:  none
; outputs: none
OS_DisableInterrupts
        CPSID   I
        BX      LR

;*********** OS_EnableInterrupts ***************
; disable interrupts
; inputs:  none
; outputs: none
OS_EnableInterrupts
        CPSIE   I
        BX      LR

;*********** StartCritical************************
; make a copy of previous I bit, disable interrupts
; inputs:  none
; outputs: previous I bit
StartCritical
        MRS     R0, PRIMASK        ; Set prio int mask to mask all (except faults)
        CPSID   I
        BX      LR


;*********** EndCritical************************
; using the copy of previous I bit, restore I bit to previous value
; inputs:  previous I bit
; outputs: none
EndCritical
        MSR     PRIMASK, R0
        BX      LR

StartOS
    LDR     R0, =RunPt         ; currently running thread
    LDR     R2, [R0]           ; R2 = value of RunPt
    LDR     SP, [R2]           ; new thread SP; SP = RunPt->stackPointer;
    POP     {R4-R11}           ; restore regs r4-11
    POP     {R0-R3}            ; restore regs r0-3
    POP     {R12}
    POP     {LR}               ; discard LR from initial stack
    POP     {LR}               ; start location
    POP     {R1}               ; discard PSR
    CPSIE   I                  ; Enable interrupts at processor level
    BX      LR                 ; start first thread


PendSV_Handler                 ; 1) Saves R0-R3,R12,LR,PC,PSR
    CPSID   I                  ; 2) Prevent interrupt during switch
    PUSH    {R4-R11}           ; 3) Save remaining regs r4-11
    LDR     R0, =RunPt         ; 4) R0=pointer to RunPt, old thread
    LDR     R1, [R0]           ;    R1 = RunPt
    STR     SP, [R1]           ; 5) Save SP into TCB
	
	LDR 	R1, =NextPt        ; 6) R1=pointer to NextPt, next thread
	LDR		R1, [R1]		   ;    R1 = NextPt
	
	STR		R1, [R0] 		   ; 7) Update Runpt to NextPt
    LDR     SP, [R1]           ; 8) new thread SP; SP = RunPt->sp;

    POP     {R4-R11}           ; 9) restore regs r4-11
    CPSIE   I                  ; 10) tasks run with interrupts enabled
    BX      LR                 ; 11) restore R0-R3,R12,LR,PC,PSR

    
	ALIGN
	END
  