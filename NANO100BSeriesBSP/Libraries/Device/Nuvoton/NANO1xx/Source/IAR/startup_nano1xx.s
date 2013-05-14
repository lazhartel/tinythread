;/******************************************************************************
; * @file:   startup_nano1xx.s
; * @brief   CMSIS ARM Cortex-M0 Core Device Startup File
; * @version 1.0.1
; * @date    04, September, 2012
; *
; * Copyright (C) 2012 Nuvoton Technology Corp. All rights reserved.
; *****************************************************************************/

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



	MODULE  ?cstartup

	;; Forward declaration of sections.
	SECTION CSTACK:DATA:NOROOT(3) ;; 8 bytes alignment

	SECTION .intvec:CODE:NOROOT(2);; 4 bytes alignment

	EXTERN  SystemInit
	EXTERN  __iar_program_start
	PUBLIC  __vector_table

	DATA
__vector_table
	DCD     sfe(CSTACK)
	DCD     Reset_Handler

	DCD     NMI_Handler
	DCD     HardFault_Handler
	DCD     0
	DCD     0
	DCD     0
	DCD     0
	DCD     0
	DCD     0
	DCD     0
	DCD     SVC_Handler
	DCD     0
	DCD     0
	DCD     PendSV_Handler
	DCD     SysTick_Handler

	; External Interrupts
	DCD     BOD_IRQHandler            ; Brownout low voltage detected interrupt
	DCD     WDT_IRQHandler            ; Watch Dog Timer interrupt
	DCD     EINT0_IRQHandler          ; External signal interrupt from PB.14 pin
	DCD     EINT1_IRQHandler          ; External signal interrupt from PB.15 pin
	DCD     GPABC_IRQHandler          ; External interrupt from PA[15:0]/PB[15:0]/PC[15:0]
	DCD     GPDEF_IRQHandler          ; External interrupt from PD[15:0]/PE[15:0]/PF[7:0]
	DCD     PWM0_IRQHandler           ; PWM 0 interrupt
	DCD     PWM1_IRQHandler           ; PWM 1 interrupt
	DCD     TMR0_IRQHandler           ; Timer 0 interrupt
	DCD     TMR1_IRQHandler           ; Timer 1 interrupt
	DCD     TMR2_IRQHandler           ; Timer 2 interrupt
	DCD     TMR3_IRQHandler           ; Timer 3 interrupt
	DCD     UART0_IRQHandler          ; UART0 interrupt
	DCD     UART1_IRQHandler          ; UART1 interrupt
	DCD     SPI0_IRQHandler           ; SPI0 interrupt
	DCD     SPI1_IRQHandler           ; SPI1 interrupt
	DCD     SPI2_IRQHandler           ; SPI2 interrupt
	DCD     HIRC_IRQHandler           ; HIRC interrupt
	DCD     I2C0_IRQHandler           ; I2C0 interrupt
	DCD     I2C1_IRQHandler           ; I2C1 interrupt
	DCD     Default_Handler           ; Reserved
	DCD     SC0_IRQHandler            ; SC0 interrupt
	DCD     SC1_IRQHandler            ; SC1 interrupt
	DCD     USBD_IRQHandler           ; USB FS Device interrupt
	DCD     TK_IRQHandler             ; Touch key interrupt
	DCD     LCD_IRQHandler            ; LCD interrupt
	DCD     PDMA_IRQHandler           ; PDMA interrupt
	DCD     I2S_IRQHandler            ; I2S interrupt
	DCD     PDWU_IRQHandler           ; Power Down Wake up interrupt
	DCD     ADC_IRQHandler            ; ADC interrupt
	DCD     DAC_IRQHandler            ; DAC interrupt
	DCD     RTC_IRQHandler            ; Real time clock interrupt

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
	THUMB
	PUBWEAK Reset_Handler
	SECTION .text:CODE:REORDER(2)       ; 4 bytes alignment
Reset_Handler
		LDR     R0, =0x50000100
		; Unlock Register
		LDR     R1, =0x59
		STR     R1, [R0]
		LDR     R1, =0x16
		STR     R1, [R0]
		LDR     R1, =0x88
		STR     R1, [R0]

		; Init POR
		LDR     R2, =0x50000024
		LDR     R1, =0x00005AA5
		STR     R1, [R2]

		; Lock register
		MOVS    R1, #0
		STR     R1, [R0]

		LDR      R0, =SystemInit
		BLX      R0
		LDR      R0, =__iar_program_start
		BX       R0

	PUBWEAK NMI_Handler
	PUBWEAK SVC_Handler
	PUBWEAK PendSV_Handler
	PUBWEAK SysTick_Handler
	PUBWEAK BOD_IRQHandler
	PUBWEAK WDT_IRQHandler
	PUBWEAK EINT0_IRQHandler
	PUBWEAK EINT1_IRQHandler
	PUBWEAK GPABC_IRQHandler
	PUBWEAK GPDEF_IRQHandler
	PUBWEAK PWM0_IRQHandler
	PUBWEAK PWM1_IRQHandler
	PUBWEAK TMR0_IRQHandler
	PUBWEAK TMR1_IRQHandler
	PUBWEAK TMR2_IRQHandler
	PUBWEAK TMR3_IRQHandler
	PUBWEAK UART0_IRQHandler
	PUBWEAK UART1_IRQHandler
	PUBWEAK SPI0_IRQHandler
	PUBWEAK SPI1_IRQHandler
	PUBWEAK SPI2_IRQHandler
	PUBWEAK HIRC_IRQHandler
	PUBWEAK I2C0_IRQHandler
	PUBWEAK I2C1_IRQHandler
	PUBWEAK SC0_IRQHandler
	PUBWEAK SC1_IRQHandler
	PUBWEAK USBD_IRQHandler
	PUBWEAK TK_IRQHandler
	PUBWEAK LCD_IRQHandler
	PUBWEAK PDMA_IRQHandler
	PUBWEAK I2S_IRQHandler
	PUBWEAK PDWU_IRQHandler
	PUBWEAK ADC_IRQHandler
	PUBWEAK DAC_IRQHandler
	PUBWEAK RTC_IRQHandler
	SECTION .text:CODE:REORDER(2)
HardFault_Handler
#ifdef DEBUG_ENABLE_SEMIHOST
        ; Check if the interrupt comes from thread or not
        ; When enterring interrupt hander
        ;    if CurrentMode==Mode_Handler then
        ;        LR = 0xFFFFFFF1;	(previous stack: MSP)
        ;    else
        ;    if CONTROL<1> == '0' then
        ;        LR = 0xFFFFFFF9;	(previous stack: MSP)
        ;    else
        ;        LR = 0xFFFFFFFD;	(previous stack: PSP)
        MOV     R0, LR
        LSLS    R0, R0, #29           ; Check bit 2
        BMI     SP_is_PSP             ; previous stack is PSP
        MRS     R0, MSP               ; previous stack is MSP, read MSP
        B       SP_Read_Ready
SP_is_PSP
        MRS     R0, PSP               ; Read PSP
SP_Read_Ready
        LDR     R1, [R0, #24]         ; Get previous PC
        LDRH    R3, [R1]              ; Get instruction
        LDR     R2, =0xBEAB           ; The sepcial BKPT instruction
        CMP     R3, R2                ; Test if the instruction at previous PC is BKPT
        BNE     HardFault_Handler_Ret ; Not BKPT

        ADDS    R1, #4                ; Skip BKPT and next line
        STR     R1, [R0, #24]         ; Save previous PC

        BX      LR
HardFault_Handler_Ret

#endif
NMI_Handler
SVC_Handler
PendSV_Handler
SysTick_Handler
BOD_IRQHandler
WDT_IRQHandler
EINT0_IRQHandler
EINT1_IRQHandler
GPABC_IRQHandler
GPDEF_IRQHandler
PWM0_IRQHandler
PWM1_IRQHandler
TMR0_IRQHandler
TMR1_IRQHandler
TMR2_IRQHandler
TMR3_IRQHandler
UART0_IRQHandler
UART1_IRQHandler
SPI0_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
HIRC_IRQHandler
I2C0_IRQHandler
I2C1_IRQHandler
SC0_IRQHandler
SC1_IRQHandler
USBD_IRQHandler
TK_IRQHandler
LCD_IRQHandler
PDMA_IRQHandler
I2S_IRQHandler
PDWU_IRQHandler
ADC_IRQHandler
DAC_IRQHandler
RTC_IRQHandler
Default_Handler
	B Default_Handler


#ifdef DEBUG_ENABLE_SEMIHOST


; int SH_DoCommand(int n32In_R0, int n32In_R1, int *pn32Out_R0);
; Input
;	R0,n32In_R0: semihost register 0
;	R1,n32In_R1: semihost register 1
; Output
;	R2,*pn32Out_R0: semihost register 0
; Return
;	0: No ICE debug
;	1: ICE debug
SH_DoCommand	
        EXPORT SH_DoCommand
        BKPT   0xAB                  ; Wait ICE or HardFault
                                     ; ICE will step over BKPT directly
                                     ; HardFault will step BKPT and the next line
        B      SH_ICE
SH_HardFault                         ; Captured by HardFault
        MOVS   R0, #0                ; Set return value to 0
        BX     lr                    ; Return
SH_ICE                               ; Captured by ICE
        ; Save return value
        CMP    R2, #0
        BEQ    SH_End
        STR    R0, [R2]              ; Save the return value to *pn32Out_R0
SH_End
        MOVS   R0, #1                ; Set return value to 1
        BX     lr                    ; Return

#endif




	END

