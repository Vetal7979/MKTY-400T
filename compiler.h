#include "ioavr.h"

#include <intrinsics.h>
#include <pgmspace.h>
#include <string.h>

#define u8 unsigned char
#define u16 unsigned int
#define nop() __no_operation();
#define sbi(reg,bit)  (reg |= (1<<bit))   //<! set bit in port
#define cbi(reg,bit)  (reg &= ~(1<<bit))  //<! clear bit in port
#define sbr(reg,bit)  (reg |= (1<<bit))   //<! set bit in port
#define cbr(reg,bit)  (reg &= ~(1<<bit))  //<! clear bit in port

#define XTALL			8.0

#define	_delay_us(us)	__delay_cycles (XTALL * us);
#define _delay_ms(ms)	_delay_us (1000 * ms) 

#define RXB8 1
#define TXB8 0
#define UPE 2
#define OVR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)

#define delay_600ns __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();
#define delay_500ns __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();

#define delay_400ns __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();

#define delay_250ns __no_operation();\
                    __no_operation();\
                    __no_operation();\
                    __no_operation();


