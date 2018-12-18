#include <xc.h>
#include <p18f4520.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "conbits.h"
#include "uart_layer.h"

#define TIME_PER_OVERFLOW   (65535u) // us
#define TIME_PER_TICK       (1u) // us

void interrupt high_isr(void);
void interrupt low_priority low_isr(void);

typedef struct{
    uint8_t ccpL;
    uint8_t ccpH;
}cap_uint16;

cap_uint16 captrue_value;
uint16_t ccp2value = 0;
uint32_t overflow_tmr3 = 0;
uint32_t cap_overflow_tmr3 = 0;
bool new_time_bool = false;

uint8_t print_buffer[256] = {0}; // buffer to print stuff to serial

uint32_t time_after_captrue(uint32_t overflow,cap_uint16 *captrue_now){
    uint16_t cap_value = 0;
    memcpy(&cap_value,captrue_now,sizeof(cap_uint16));
    return ((overflow * TIME_PER_OVERFLOW) + (cap_value * TIME_PER_TICK));
}

void main(void){
    
    uint32_t time_ccp = 0;
    
    OSCCONbits.IRCF = 0x07;
    OSCCONbits.SCS = 0x03;
    while(OSCCONbits.IOFS!=1); // 8Mhz
    
    TRISB=0;    // b port as output
    LATB=0x00; // b port low
    
    TRISCbits.RC1 = 1;
  
    uart_init(51,0,1,0);//baud 9600
    
    CCP2CONbits.CCP2M = 0x05;
    
    T3CONbits.RD16 = 0;
    T3CONbits.T3CCP2 = 0;
    T3CONbits.T3CCP1 = 1;
    T3CONbits.T3CKPS = 1;
    T3CONbits.TMR3CS = 0;
    
    IPR2bits.TMR3IP = 0;
    IPR2bits.CCP2IP = 1;
    
    PIE2bits.TMR3IE = 1;
    PIE2bits.CCP2IE = 1;
    
    
    RCONbits.IPEN = 1; 
    INTCONbits.GIEH = 1; 
    INTCONbits.GIEL = 1;// base interrupt setup
    
    T3CONbits.TMR3ON = 1;
    
    sprintf(print_buffer,"\n\rprogram start\n\r");
    uart_send_string(print_buffer); // everything works in setup
    
    for(;;){
        
        if(new_time_bool){
            time_ccp = time_after_captrue(cap_overflow_tmr3,&captrue_value);
            sprintf(print_buffer,"\rtime press %10ums",time_ccp/1000);
            uart_send_string(print_buffer);
            new_time_bool = false;
        }

    } 
}



void interrupt high_isr(void){
    INTCONbits.GIEH = 0;
    if(PIR2bits.CCP2IF == 1){
        cap_overflow_tmr3 = overflow_tmr3;
        captrue_value.ccpL = CCPR2L;
        captrue_value.ccpH = CCPR2H;
        overflow_tmr3 = 0;
        new_time_bool = true;
        PIR2bits.CCP2IF = 0;
    }
    INTCONbits.GIEH = 1;
}

void interrupt low_priority low_isr(void){
    INTCONbits.GIEH = 0;
    if(PIR2bits.TMR3IF == 1){
        overflow_tmr3++;
        PIR2bits.TMR3IF = 0;
    }  
    INTCONbits.GIEH = 1;
}



