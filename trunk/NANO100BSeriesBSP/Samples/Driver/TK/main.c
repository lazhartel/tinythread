/******************************************************************************
 * @file     main.c
 * @brief    Nano1xx Touch Key Driver Sample code
 * @version  1.0.1
 * @date     04, September, 2012
 *
 * @note
 * Copyright (C) 2012-2014 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nano1xx.h"
#include "nano1xx_tk.h"
#include "nano1xx_assert.h"

#define CLS				  do {printf("%c[2J", 0x1B); printf("%c[%d;1H", 0x1B, 0);}while(0)
#define JMP(x)			  do {printf("%c[0J", 0x1B); printf("%c[%d;1H", 0x1B, x);}while(0)

extern void UART_Setting(void);


uint16_t th[16][2] = {{0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0},
        {0xFFFF, 0}
};
uint16_t data[16];
uint16_t f, t;
uint8_t volatile complete = 0;
uint8_t set = 0;  // set 0 = ch0~7, set 1 = ch8~15

void env_test(void)
{
        uint8_t c, div = 2, i;

        uint16_t ch;
        uint8_t level = 8;
        S_TK_CH_CFG ch_cfg = {8, 2, 0xFFFF, 0x0000};


        TK_Init(TK_CTL1_SEN_SEL_16BIT);

        TK_EnableInterrupt(TK_INT_ALL);

again:
        printf("Plese select test channel mask 0001~ffff\n");
        ch = 0;
        for(i = 0; i < 4; i++) {
                while(1) {
                        c = getchar();
                        if(c >= 0x30 && c <= 0x39) {
                                c -= 0x30;
                                break;
                        }
                        if(c >= 0x61 && c <= 0x66) {
                                c -= 0x57;
                                break;
                        }
                        if(c >= 0x41 && c <= 0x46) {
                                c -= 0x37;
                                break;
                        }

                }
                ch = (ch << 4) + c;
        }

        if(ch == 0)
                goto again;

        CLS;
        printf("test channel mask %04x\n", ch);
        printf("Increase current: s        Decrease current: x\n");
        printf("Increase engine clock: d   Decrease engine clock: c\n");

        printf("Quit: q\n");

        while(1) {
                JMP(6);
                printf("Current setting: current %02d/15, clk 1/%02d\n", level, (1 + div));

                complete = 0;
                set = 0;
                for(i = 0; i < 8; i++) {
                        ch_cfg.u16LowThld = th[i][0];
                        ch_cfg.u16HighThld = th[i][1];
                        TK_ConfigChannel(i, &ch_cfg);
                }
                TK_Start(ch & 0xFF);
                while(complete == 0);

                complete = 0;
                set = 1;
                for(i = 8; i < 16; i++) {
                        ch_cfg.u16LowThld = th[i][0];
                        ch_cfg.u16HighThld = th[i][1];
                        TK_ConfigChannel(i, &ch_cfg);
                }
                TK_Start(ch & 0xFF00);
                while(complete == 0);


                for(i = 0; i < TK_CH_NUM; i++) {
                        if(ch & (1 << i)) {
                                printf("ch%02d: l_th-%04x h_th-%04x data-%04x %c %c\n", i,
                                       th[i][0],
                                       th[i][1],
                                       data[i],
                                       (t & (1 << i)) ? 'T' : ' ',
                                       (f & (1 << i)) ? 'F' : ' ');
                                if(th[i][0] >= data[i])
                                        th[i][0] = data[i] - 0x500;
                                else if(data[i] - th[i][0] > 0x500)
                                        th[i][0] += 0x500;
                                if(th[i][1] <= data[i])
                                        th[i][1] = data[i] + 0x500;
                                else if(th[i][1] - data[i] > 0x500)
                                        th[i][1] -= 0x500;
                        }
                }
                t = f = 0;
                if((UART0->FSR & UART_FSR_RX_EMPTY_F) == 0) {
                        c = getchar();
                } else {
                        continue;

                }
                switch(c) {
                case 's':
                case 'S':
                        if(level != TK_MAX_CURRENT_LEVEL)
                                level++;
                        ch_cfg.u8Level = level;
                        break;
                case 'x':
                case 'X':
                        if(level != TK_MIN_CURRENT_LEVEL)
                                level--;
                        ch_cfg.u8Level = level;
                        break;
                case 'd':
                case 'D':
                        if(div != TK_MIN_CLOCK_DIVIDER)
                                div--;
                        ch_cfg.u8Div = div;
                        break;
                case 'c':
                case 'C':
                        if(div != TK_MAX_CLOCK_DIVIDER)
                                div++;
                        ch_cfg.u8Div = div;
                        break;
                case 'q':
                case 'Q':
                        goto quit;

                }
        }
quit:
        TK_DeInit();
        return;
}



int32_t main(void)
{
		GCR->PA_L_MFP = (GCR->PA_L_MFP & ~(PA0_MFP_MASK | PA1_MFP_MASK)) | PA0_MFP_TK8 | PA1_MFP_TK9;		// TK8, 9
		GCR->PA_H_MFP = (GCR->PA_H_MFP & ~(PA12_MFP_MASK | PA13_MFP_MASK)) | PA12_MFP_TK10 | PA13_MFP_TK11;		// TK10, 11
		GCR->PC_H_MFP = (GCR->PC_H_MFP & ~(PC8_MFP_MASK | PC9_MFP_MASK | PC10_MFP_MASK | PC11_MFP_MASK)) |
						PC8_MFP_TK12 | PC9_MFP_TK13 | PC10_MFP_TK14 | PC11_MFP_TK15;		// 12~15
		GCR->PD_L_MFP = (GCR->PD_L_MFP & ~(PD0_MFP_MASK | PD1_MFP_MASK | PD2_MFP_MASK | PD3_MFP_MASK | PD4_MFP_MASK | PD5_MFP_MASK)) |  
						PD0_MFP_TK0 | PD1_MFP_TK1 | PD2_MFP_TK2 | PD3_MFP_TK3 | PD4_MFP_TK4 | PD5_MFP_TK5;    // 0~5
		GCR->PF_L_MFP = (GCR->PF_L_MFP & ~(PF4_MFP_MASK | PF5_MFP_MASK)) | PF4_MFP_TK6 | PF5_MFP_TK7;    // 6, 7

        env_test();

        printf("Adios!!\n");
        return(0);
}

#ifdef USE_ASSERT
/**
  * @brief  The function prints the source file name and line number where the assert_param() error
  *         occurs, and then stops in an infinite loop. User can add his own codes here if necessary.
  * @param[in] file Source file name
  * @param[in] line Line number
  * @return None
  */
void assert_error(uint8_t * file, uint32_t line)
{
 	GCR->PB_L_MFP = (GCR->PB_L_MFP & ~0x77) | (PB1_MFP_UART0_TX | PB0_MFP_UART0_RX);  /* Select multi-function pin for UART0 */
 	CLK->APBCLK |= CLK_APBCLK_UART0_EN;    /* Enable UART0 clock */
	CLK->CLKDIV0 &= ~CLK_CLKDIV0_UART_MASK;
	CLK->CLKSEL1 = (CLK->CLKSEL1 & ~CLK_CLKSEL1_UART_MASK) | CLK_CLKSEL1_UART_HXT;  /* Select 12 Mhz XTAL */

 
    /* Set UART to 115200,n,8,1,none */
    UART0->BAUD = 0x67;             /* Baud Rate:115200 for 12MHz */
    UART0->TLCTL = 0x03;            /* Word len is 8 bits */          
	
	printf("[%s] line %d : wrong parameters.\r\n", file, line);

	/* Infinite loop */
	while(1) ;

}
#endif

/*** (C) COPYRIGHT 2012 Nuvoton Technology Corp. ***/
