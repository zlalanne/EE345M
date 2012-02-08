   THUMB
    REQUIRE8
    PRESERVE8
    AREA    |.text|, CODE, READONLY, ALIGN=2

	EXTERN	RunPt
	EXPORT  SysTick_Handler

SysTick_Handler		  ; 1) Saves R0-R3,R12,LR,PC,PSR
	CPSID I           ; 2) Make atomic
	PUSH  {R4-R11}    ; 3) Save remaining regs r4-11
	LDR   R0, =RunPt  ; 4) R0=pointer to RunPt, old
	LDR   R1, [R0]    ;    R1 = RunPt
	STR   SP, [R1]    ; 5) Save SP into TCB
	LDR   R1, [R1,#4] ; 6) R1 = RunPt->next
	STR   R1, [R0]    ;    RunPt = R1
	LDR   SP, [R1]    ; 7) new thread SP; SP=RunPt->sp;
	POP   {R4-R11}    ; 8) restore regs r4-11
	CPSIE I           ; 9) tasks run enabled
	BX    LR          ; 10) restore R0-R3,R12,LR,PC,PSR

	ALIGN
	END