void init_cpu(void)
{
#define rele1 PORTC_Bit2
#define rele2 PORTC_Bit3
#define rele3 PORTA_Bit4
#define rele4 PORTA_Bit7  
#define rele5 PORTG_Bit2
#define rele6 PORTD_Bit1 
#define rele7 PORTA_Bit2

// DDRB_Bit6 = 1;
/* 
DDRC_Bit2 = 1;
DDRC_Bit3 = 1;
DDRC_Bit4 = 1;
DDRC_Bit5 = 1;  
*/  
#define memory_select PORTB_Bit4
#define modem_status PINA_Bit5
#define modem_power PORTG_Bit0

// константы для датафлэш
#define Buff1_write        0x84
#define Buff2_write        0x87
#define Buff1_mem_write    0x83
#define Buff2_mem_write    0x86
#define Current_page_read  0xD2
#define Current_page_write 0x82
// SPI initialization
// SPI Type: Master
// SPI Clock Rate: 1843,200 kHz
// SPI Clock Phase: Cycle Half
// SPI Clock Polarity: High
// SPI Data Order: MSB First
DDRB = (1<<0)|(1<<1)|(1<<2);
SPCR = (1<<SPE)|(1<<MSTR); //58
//SPSR = 0x00;
DDRC_Bit2=1; // реле 1 порт на выход
DDRC_Bit3=1; // реле 2 порт на выход
DDRA_Bit4=1; // реле 3 порт на выход
DDRA_Bit7=1; // реле 4 порт на выход
DDRG_Bit2=1; // реле 5 порт на выход
DDRD_Bit1=1; // реле 6 порт на выход
DDRA_Bit2=1; // реле 7 порт на выход


DDRB_Bit4=1; // Select memory
DDRB_Bit5=1; // Voice out

//DDRB_Bit2=1; // MOSI
//DDRB_Bit1=1; // SCK

DDRG_Bit0=1; // modem power

DDRC_Bit4=1; // Сброс диспа ножку на выход

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 7,200 kHz
// Mode: Normal top=FFh
// OC0 output: Disconnected
// 10.00 ms
ASSR=0x00;
TCCR0=0x07; 
TCNT0=0xB8; 
//TCNT0=0x10;
OCR0=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x01;
ETIMSK=0x00;


// Настройка USART1 
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART1 Receiver: On
// USART1 Transmitter: On
// USART1 Mode: Asynchronous
// USART1 Baud Rate: 115200
UCSR1A=0x00;
UCSR1B=0x98;
UCSR1C=0x06;
UBRR1H=0x00;
UBRR1L=0x03;


// Шим для звука
TCCR1A = 0xC1; //деRгаем по сравнению
TCCR1B = 0x09; //no prescale
memory_select=1; // выключим пока датафлэш

// Watchdog Timer initialization
// Watchdog Timer Prescaler: OSC/2048k
#pragma optsize-
WDTCR=0x1F; // примерно 2 сек
WDTCR=0x0F;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

__enable_interrupt();
}