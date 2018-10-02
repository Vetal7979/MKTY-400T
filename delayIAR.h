/*
Хидер задержек для IAR. Использует __delay_cycles для формирования задержек
от наносекунд до секунд. В параметре CPU_CLK_Hz указать тактовую частоту в герцах.


*/

#ifndef DELAY_H
#define DELAY_H

#include <inavr.h>

#define CPU_CLK_kHz (unsigned long)(CPU_CLK_Hz/1000)

#define delay_ns(x) __delay_cycles(x*CPU_CLK_kHz*0.000001)
#define delay_us(x) __delay_cycles(x*(CPU_CLK_Hz/1000000))
#define delay_ms(x) __delay_cycles(x*(CPU_CLK_Hz/1000))
#define delay_s(x)  __delay_cycles(x*CPU_CLK_Hz)

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


#endif
