//   МКТУ-400Т
// 
// 
// 
// 
#define CPU_CLK_Hz 7372800
#include <iom64.h>
#include "main.h"
#include "compiler.h"
#include "delayIAR.h"
#include "delay.c"
#include <ioavr.h>
#include <string.h>  
#include <stdlib.h>
#include <stdio.h>
#include "uart.h"
#include "init.c"
#include "voice.c"

//#include "lcd.c" 
//#include "lcd.h"
//#include "pff.h" //?
//#include "diskio.h" //?

char cycle_buff[BUFF_SIZE];  // кольцевой буфер
u16 r_index=0, w_index=0, rx_count=0;  // индекс чтения, записи, кол-во байт в буфере

char rx_buff[Rx_buff_size]; // входной буфер
char sms_buff[SMS_buff_size]; // буфер смс
char answer[70]; // ответное смс

u8 rx_cnt=0;
unsigned char mess; 
char dtmf_buff[6], dtmf_cnt=0; // буфер dtmf команд
u16 timeout; // таймаут ответа от модема
char task=0xFF;  // номер задачи, FF - не определено
                // 0 - основной цикл

/* Флаги (можно потом сделать чтоб все хранилось в 1 переменной битами) */   
unsigned char ok_flag, err_flag=0, sms_flag=0, cpin_rdy=0, ring_flag=0, cmgs_flag=0;
unsigned char call_rdy=0, sms_rdy=0, n_pwr_dn=0, cfun=0;


char mass_number[max_numb_len];
char mass_inc_number[max_numb_len]; // номер при входящем звонке
char mass_master_number[max_numb_len]; // мастер номер (считанный)
char ring_master=0; // 1 если определили что звонит мастер номер


unsigned char pa[11]; // Настройки ячейки CONFIG *000011**11 (11 символов) так и будет по порядку, звездочки тоже.
unsigned char vv[4]; // если в 1 то авария датчика
char p[4]; // хранимый пароль из симки (массив 4 байта в ASCii)
unsigned char power_mon_bit;    // мониторим напругу или нет
unsigned char sms_mode;         // тип смс
unsigned char sms_razresh;       // разрешение посылки смс

unsigned long overflow; // счетчик для вешания трубки
unsigned char ring_cnt=0;  // чтоб трубку брало со 2-го RING, а не сразу
unsigned char wait_on,wait_off;    // байт на задержку включения и задержку выключения
unsigned char retry_count;         // байт на счетчик попыток
unsigned char relay_status[8]={0,0,0,0,0,0,0,0};  // 0 элемент не используем - чтобы не было путаницы с реле 



unsigned char sms_stat[4]={0,0,0,0};  // для однократного посыла смс
//unsigned char cm[5]={0,0,0,0,0}; // надо ли звонить на номера 1..5


unsigned char temperature_status;
unsigned char baterry_bit = 1;
unsigned char zb=0;
unsigned char ust[4], hyst;
int temp[4];
char range;


/*
char flag_rele3=0; // флаг того, что реле включим по срабатыванию сигналки
unsigned long rele3_count=0; // счетчик времени работы сирены 
*/
unsigned int reztemp;//результат замера АЦП
int temperature;



//------------------------------------------------------------------------------
// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
  ADMUX=adc_input | (ADC_VREF_TYPE & 0xff);
  // Delay needed for the stabilization of the ADC input voltage
  delay_us(10);
  // Start the AD conversion
  ADCSRA|=0x40;
  // Wait for the AD conversion to complete
  while ((ADCSRA & 0x10)==0);
  ADCSRA|=0x10;
  return ADC;
}
//------------------------------------------------------------------------------
signed int Get_Temperature(char dat_numb)
{
  char channel;
  switch (dat_numb)
  {
  case 1:{channel=5;}break;
  case 2:{channel=3;}break;
  case 3:{channel=4;}break;
  case 4:{channel=1;}break;
  }
    reztemp=0;  
    reztemp = read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp += read_adc(channel);
    reztemp >>= 3;
     
     return ADC2Temp(reztemp);

}


//------------------------------------------------------------------------------
/*
void put_message_UCS2(char *b)
{
// не работает, бяда с указателями
 char aaa[5]={0,0,0,0,0};
 u16 tmp=0;
// lcd_at(0,0); lcd_put_char(*b,0);
 while(*b);
 {
//  Delay_ms(500);
  __watchdog_reset();
  if (*s>=192) tmp=*s;
  else tmp=*s + 0x350;
  sprintf(aaa,"%4.4X",tmp);
 put_message_modem_RAM(aaa);
  *s++;
 
 }
}
*/
//------------------------------------------------------------------------------
void put_message_modem_RAM (char *s)
{
  while (*s)
    putchar_modem(*s++);
  Delay_ms(del);
}
//------------------------------------------------------------------------------
u8 putchar_modem(unsigned char send_char)
{
  while (!(UCSR1A & 0x20))
    ; /* wait xmit ready */
  UDR1 = (unsigned char) send_char;
  return(send_char);
}
//------------------------------------------------------------------------------
void put_message_modem (char const __flash  *s)
{
  while (*s)
    putchar_modem(*s++);
  
  Delay_ms(del);  // не торопимся, а то может заглючить :)
}

//------------------------------------------------------------------------------
void Get_Number(char *buff, char *mass)  // выдирает номер их *buff и кладет его в mass
{
//  char i=0;
  while(*buff++!='"');
  while(*buff!='"') // начинаем читать номер до след "
   {
    *mass++=*buff++; // пишем номер в mass_number
   }
  *mass=0x00; // завершаем строку \0
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int ADC2Temp(unsigned int Temp_adc1)     // ----
{  
  register unsigned int cnt;    
  
  if ( Temp_adc1 < 222 || Temp_adc1 > 514 ) return 1000;
  //setup counter
  cnt=0xffff;
  
  while ( InTab[++cnt] < Temp_adc1 );
  
  if(cnt) --cnt;
  Temp_adc1= ( (double)OutTab[cnt]+((long)OutTab[cnt+1]-OutTab[cnt])*(Temp_adc1-InTab[cnt])/
              (InTab[cnt+1]-InTab[cnt]) );
  return Temp_adc1; }  
//------------------------------------------------------------------------------
// Говорит любое число от -199 до 199
void say_numeric(int num)
{
  if (num==0) say_zero();
  if (num<0){ minus(); num=num*(-1); } //если отрицательная температура скажем минус и сделаем ее положительной
  if (num>=100) { say_100(); num=num-100; Delay_ms(100); } // больше  100 - скажем сто, и отнимем....   
  if (num>=20)
  {
    switch (num/10)
    {
    case 2:{say_20();}break;
    case 3:{say_30();}break;
    case 4:{say_40();}break;
    case 5:{say_50();}break;
    case 6:{say_60();}break;
    case 7:{say_70();}break;
    case 8:{say_80();}break;
    case 9:{say_90();}break;
    }
    Delay_ms(100);
    num%=10;
  }
  else if(num>=10 && num<=19)
  {
   switch (num)
   {
   case 10:{ say_10(); }break; 
   case 11:{ say_11(); }break; 
   case 12:{ say_12(); }break; 
   case 13:{ say_13(); }break; 
   case 14:{ say_14(); }break; 
   case 15:{ say_15(); }break; 
   case 16:{ say_16(); }break; 
   case 17:{ say_17(); }break; 
   case 18:{ say_18(); }break; 
   case 19:{ say_19(); }break; 
   }
   num/=100; // чтоб получился 0 и вышли
  }
  
  if (num<10)
  {
    switch (num)
    {
    case 1:{say_one();}break;  
    case 2:{say_two();}break;
    case 3:{say_three();}break;
    case 4:{say_four();}break;
    case 5:{say_five();}break;
    case 6:{say_six();}break;
    case 7:{say_seven();}break;
    case 8:{say_eight();}break;
    case 9:{say_nine();}break;
    }    
  }
}
//------------------------------------------------------------------------------
// говорит градус, градусов, градуса
void say_gradus(int num)
{
 if (num<0) num*=-1;
 num%=10;
 if (num==1) gradus();
 else if(num==2 || num==3 || num==4) gradusa();
 else gradusov();

}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//---------------------------Прерывание по приходу данных из модема-------------
#pragma vector = USART1_RXC_vect   
__interrupt void _USART1_RXC (void)
{
/* Read one byte from the receive data register */
unsigned  char status, data;
  status=UCSR1A;
  data=UDR1;
  if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
  {
    cycle_buff[w_index]=data;
    w_index++; rx_count++;
    if (w_index==BUFF_SIZE) w_index=0;
  }  
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Timer 0 overflow interrupt service routine
// 10.0 ms
#pragma vector = TIMER0_OVF_vect  
__interrupt void _TIMER0_OVF_vect (void)
{
// Reinitialize Timer 0 value
 TCNT0=0xB8; // 10.00 ms

 if(overflow!=0) overflow--;

 if (timeout!=0) timeout--;
 
// if(rele3_count!=0) rele3_count--;
 
//PORTC_Bit6=~PORTC_Bit6;// test
}// Timer 0 
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
unsigned char Get_Char(void)
{
   unsigned char sym = 0;
   if (rx_count > 0)  //если буфер не пустой
   {                            
      sym = cycle_buff[r_index];              //считываем символ из буфера
      rx_count--;                                   //уменьшаем счетчик символов
      r_index++;                                  //инкрементируем индекс головы буфера
      if (r_index == BUFF_SIZE) r_index = 0;
   }
   return sym;
}
//------------------------------------------------------------------------------
void Clean_buff(void)
{
  rx_count=0;
  r_index=0;
  w_index=0;
}
//------------------------------------------------------------------------------
void Parser(void)
{ 
 unsigned char sym = 0;
  // для SIM900 символ на 6 позиции, для SIM800 - на 7 (эти суки добавили пробел)
  // define в main.h

#ifdef SIM800
 #ifndef SIM900
 char dtmf_sym_pos=7;
 #endif 
#else
 #ifdef SIM900
  #ifndef SIM800 
 char dtmf_sym_pos=6;
  #endif 
 #endif 
#endif
 
 while(!mess && rx_count!=0)
 {  
   sym=Get_Char();
//   if(sym==0x0A && (rx_cnt!=0 && rx_buff[rx_cnt-1]==0x0D))                      // пришло 0x0A, а перед этим было 0x0D
   if (sym==0x0D)                                                               // ловим сообщения только по 0x0D, а 0x0A пропускаем
   { 
     mess=1;                                                                    // поднимаем флаг - пока флаг - след сообщение не прочитается, поэтому mess=0; только после обработки строки, а то может заглючить
     rx_buff[rx_cnt]=0; /*rx_buff[rx_cnt-1]=0; */                               // заменяем \r\n на \0\0 (завершаем строку) - чтоб нормально работать со строками и не мешался \r в конце) 
     rx_cnt=0;
     if (rx_buff[0]==0 || !strncmp_P(rx_buff,at_p,3)) mess=0;                   // пришло только \r\n  или пришло эхо от модема - его пропускаем                                    
//     if (rx_buff[rx_cnt-2]==0x0D) rx_buff[rx_cnt-2]=0;                          // при эхе от модема может придти \r\r\n, но начинаться будет не с AT - вроде не должно такого быть!!
    
     // Далее парсим на URC                                                     // 
     if (rx_buff[0]=='+')                                                       // пришло что-то, начинающееся с + -> разбираем его 
     {
       if (!strncmp_P(rx_buff,cpin,6))                                          // +CPIN:
       { 
         if (!strncmp_P(rx_buff,cpin_ready,12)) cpin_rdy=1;                     // +CPIN: READY                      
         if (!strncmp_P(rx_buff,cpin_not_ins,19) || 
             !strncmp_P(rx_buff,cpin_not_ready,16)) while(1);                   // +CPIN: NOT READY придет если отвалится симка
         
         mess=0; 
       } 
       
       else if (!strncmp(rx_buff,"+CFUN:",6))                                   // +CFUN:
       { 
         if (!strncmp(rx_buff,"+CFUN: 1",12)) cfun=1;  
         mess=0; 
       } 
       
       else if (!strncmp_P(rx_buff,dtmf,6))                                     // +DTMF:
       { 
         if (pa[10]==0x31)// проговариваем нажимаемые цифры
           if (rx_buff[dtmf_sym_pos]>=0x30 && rx_buff[dtmf_sym_pos]<=0x39) say_numeric(rx_buff[dtmf_sym_pos]-0x30); else gudok();  
           overflow=15*100; dtmf_buff[dtmf_cnt++]=rx_buff[dtmf_sym_pos];  // каждое нажатие кнопки дает еще +15 сек до того как повесим трубку
           if(dtmf_cnt==6) dtmf_cnt=0; 
           mess=0;                                                              // обязательно в конце, иначе начинает путать цифры ?????
       }
       
       else if (!strncmp_P(rx_buff,cmti,6))                                     // +CMTI:
       { if (!sms_flag) { sms_flag=1; strcpy(sms_buff,rx_buff); }  mess=0; }    //строку копируем для дальнейшего разбора (если уже обрабатываем какую-то смс, то эту пропустим)
       
       else if (!strncmp_P(rx_buff,clip_p,6))                                   // +CLIP:
       { 
         Get_Number(rx_buff,mass_inc_number);                                   // выдираем номер
         if (!strncmp(mass_master_number,mass_inc_number,strlen(mass_master_number))) ring_master=1; else ring_master=0;
         mess=0;
       }
     }// '+'
     else
     {
       if (!strncmp_P(rx_buff,ring,4) && task!=0) { ring_flag=1; mess=0; }      // ринг надо как-то по другому обрабатывать       // mess=0 - выкидываем RING чтоб не висело в буфере 
       else if (!strncmp_P(rx_buff,call_ready,10)) { call_rdy=1; mess=0; }      // Call Ready
       else if (!strncmp_P(rx_buff,sms_ready,9)) { sms_rdy=1; mess=0; }         // SMS Ready
       else if (!strncmp_P(rx_buff,power_dn,17)) { n_pwr_dn=1; mess=0; }        // NORMAL POWER DOWN
       //       else if (!strncmp_P(rx_buff,init_str,4)) { mess=0; rx_cnt=0; }           // пропускаем IIIIюююю (смотрим тока 1-е 4 символа, т.к. больше - глючит)
       
       
     }
   }                 
   else if (sym!=0 && sym!=0x0A) rx_buff[rx_cnt++]=sym;                                      // еще не конец - пишем в буфер
   
   if (rx_cnt==255) rx_cnt=0;  // защита от переполнения (хотя т.к. rx_cnt = u8 - оно сработает автоматом по переполнению )
   
 }//while
}
//------------------------------------------------------------------------------
// мигаем код ошибки
void System_Error(unsigned char err)
{
 while (1)
 {
  __watchdog_reset();  
  switch (err)
  {
  case 0:{}break;
  case 1:             // Нет SIM или SIM отвалилась
    { 

    }break;
  case 2:{}break;
  }
 }
}
//------------------------------------------------------------------------------
void Hardware_PWR_ON(void)
{
 Delay_ms(100);                                                                 // без этой задержки почему-то криво обрабатывает флаг n_pwr_dn
 n_pwr_dn=0; call_rdy=0; cpin_rdy=0;  sms_rdy=0; // сбросим флаги
 modem_power = 1;
 Delay_s(3);
 modem_power = 0; 

}
//------------------------------------------------------------------------------

StatusTypeDef SIM800_PWR_ON(void)
{ 
  if (setup==1) // не первое включение
    Hardware_PWR_ON();
  else
    setup=1;    // включили 1-й раз

 mess=0;

 char op_present=0;
 char st=0;
 char end=0;
 char err_cnt=0;

timeout=2000; // ждем появления Call Ready 20 сек

 while(!end)
 {
  __watchdog_reset();  

  switch (st)
  { // появление Call Ready задается AT+CIURC=1 (default)
  case 0: {  if (mess) mess=0; if(call_rdy && cpin_rdy/* && sms_rdy */) st++; }break; // ждем прихода +CPIN: Ready, Call Ready и SMS Ready 

  case 1: { put_message_modem (ipr_115200); st++; timeout=300; }break;
  case 2: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,strlen_P(error))) {st--; err_cnt++;} }  mess=0;} } break;
  
  case 3: // ATE 0
    { 
#ifdef ATE1
st=10;
#else
put_message_modem (echo_off); st++; timeout=300; 
#endif
    }break;
  case 4:  // OK
    { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st=10; else { if (!strncmp_P(rx_buff,error,strlen_P(error))) {st--; err_cnt++;} }  mess=0;} } break;   
    
  case 10:
    { put_message_modem(cops); st++; timeout=300;}break;
  case 11:
    { 
      if (mess) 
      {  
        if (!strncmp_P(rx_buff,cops_p,6)) // пришел +COPS:
        {
          if (!strncmp_P(rx_buff,cops_ans,12)) op_present=1; // пришел +COPS: 0,0,"
          else if (!strncmp_P(rx_buff,cops_no_ans,8)) op_present=0; // пришел +COPS: 0
          st++;
        }
        mess=0;
      } 
    }break;
  case 12:
    {
      if (mess) 
      {
        if (!strncmp_P(rx_buff,ok,2) && op_present==1) st++;  
        else {st=10; Delay_s(1); timeout=300;} 
        mess=0;
      }      
    }break;
  case 13:{ end=1; }break;
  }// switch
  if (!mess) Parser();  
  if (n_pwr_dn) { n_pwr_dn=0;   st=0;  Hardware_PWR_ON(); }
  if (timeout==0) { end=1; return STATUS_TIMEOUT; }
  if (err_cnt>10) return STATUS_ERROR;

}
return STATUS_OK;
}
//------------------------------------------------------------------------------
/*
// В конце надо AT&W
// это для SIM800, для SIM900 почти нихера в память не сохраняет
StatusTypeDef SIM800_First_Setup(void)
{

  Hardware_PWR_ON();
  Delay_s(5);
  put_message_modem(at);
  Delay_s(2);
  put_message_modem(at);
  Delay_s(2);
  put_message_modem(ipr_115200);
  Delay_s(2);
  put_message_modem(ciurc);
  Delay_s(2);
//  put_message_modem(sms_mess_ind); // сработает только если было SMS Ready
//  Delay_s(2);
  put_message_modem(atw);
  Delay_s(2);
  setup=1;  
  Hardware_PWR_ON(); // выключим модем и зациклимся - прибор пересбросится
  while(1); 
}
*/
StatusTypeDef SIM800_First_Setup(void)
{
  char end=0, st=0, err_cnt=0;
  Hardware_PWR_ON();
  while(!end)
  {
    __watchdog_reset();  
   switch (st)
   {
   case 0:{ put_message_modem (at); st++; timeout=500; }break; // вначале большой таймаут (5 сек), т.к. долго включается и т.п.
   case 1:{ if(mess){ if(!strncmp_P(rx_buff,ok,2)) st++; else st--;  mess=0;} }break;
   case 2:{ put_message_modem(ipr_115200); st++; timeout=200; }break;
   case 3:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;
   case 4:{ put_message_modem(ciurc); st++; timeout=200; }break;
   case 5:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
   case 6:{ put_message_modem(atw); st++; timeout=200; }break;
   case 7:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;   
   case 8:{ end=1; }break;
   
   }
   if (!mess) Parser();
   if (n_pwr_dn) { st=0;  Hardware_PWR_ON();  }
   if (timeout==0) { end=1;  return STATUS_TIMEOUT; }
   if (err_cnt>10) return STATUS_ERROR;
  }
return STATUS_OK;
}
//------------------------------------------------------------------------------
StatusTypeDef SIM800_Init(void)
{
  /*  Возможно стоит добавить:
  AT+CSCB - запрет широковещательных сообщений
  */
  char st=0, end=0, err_cnt=0;
  while(!end)
  {  
    __watchdog_reset(); 
    switch (st)
    {
    case 0:{ put_message_modem (_atl3); st++; timeout=200; }break; 
    case 1:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;
    case 2:{ put_message_modem (atm1); st++; timeout=200; }break;
    case 3:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;
    case 4:{ put_message_modem (_atclvl); st++; timeout=200; }break;
    case 5:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;  
    case 6:{ put_message_modem (mic_lvl); st++; timeout=200; }break;
    case 7:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 8:{ put_message_modem (set_gsm); st++; timeout=200; }break;
    case 9:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 10:{ put_message_modem (_atcpbs); st++; timeout=200; }break;
    case 11:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 12:{ put_message_modem (colp); st++; timeout=200; }break;
    case 13:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;     
    case 14:{ put_message_modem (sms_del_all); st++; timeout=200; }break; // сотрем все смс
    case 15:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 16:{ put_message_modem(sms_storage_sm); st++; timeout=300; } // может оно и не надо
    case 17:{ if (mess) { if (!strncmp_P(rx_buff,cpms,6)) {mess=0; st++; } else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} } } }break;
    case 18:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break;
    case 19:{ put_message_modem (sms_format_text); st++; timeout=200; }break; // выполняем 1 раз при настройке
    case 20:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;     
    case 21:{ put_message_modem (dtmf_en); st++; timeout=200; }break;
    case 22:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 23:{ put_message_modem(sms_mess_ind); st++; timeout=200; }break; // реакция на смс
    case 24:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break; 
    case 25:{ put_message_modem(clip); st++; timeout=200; }break; 
    case 26:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break;     
    case 27:{ end=1; }break;   
    }
    if (!mess) Parser();
    if (timeout==0) { end=1;  return STATUS_TIMEOUT; }
    if (err_cnt>10) return STATUS_ERROR;
    
    
  }
return STATUS_OK;
}
//----------------------------------------------------------------------------------
StatusTypeDef SSB_Config(void) // добавить выдергивание номеров 1..5, а лучше чтоб номера дергало только при отправке смс и само переводило в юникод
{
 char st=0;
 char err_cnt=0;
 char empty_sim=0;
 char end=0;
// char i=0;
 mess=0;
 
 while (!end)
 {
  __watchdog_reset();
  switch (st)
  {
  case 0:{ put_message_modem (check_parol); st++; timeout=200; }break; 
  case 1:
    { 
      if (mess)
      {      
        if (!strncmp_P(rx_buff,parol_ch,strlen_P(parol_ch))) { empty_sim=0; st++; } // на симке есть ячейка PAROL, перейдем к st=2 (считаем пароль и конфиг)
        else if (!strncmp_P(rx_buff,ok,2)) { empty_sim=1; st+=3;}                // если нет записи, то модем ответит только ОК и надо перескочить на st=4
        else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } 
        mess=0;
//        delay_ms(10);
      }
    }break;
  case 2: 
    { 
     p[0]=rx_buff[17]; p[1]=rx_buff[18]; p[2]=rx_buff[19]; p[3]=rx_buff[20];
     power_mon_bit=rx_buff[11]-0x30; 
     sms_mode=rx_buff[13]-0x30; 
     sms_razresh=rx_buff[15]-0x30; // разрешение посылки SMS
     st++;     
    }break;
  case 3: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break;
  case 4: { if(empty_sim==1) st++; else st+=2; }break;
  case 5: { Write_SIM_default(); st++; }
 
  case 6: { put_message_modem(check_config); st++; timeout=200; }break;
  case 7:
    { 
      if (mess)
      { 
        if (!strncmp_P(rx_buff,cbpr,6)) // ответ от SIM
        {
          pa[1] = rx_buff[11] ;  // терморег1
          pa[2] = rx_buff[12] ;  // терморег2
          pa[3] = rx_buff[13];   // терморег3
          pa[4] = rx_buff[14];   // терморег4
          pa[5] = rx_buff[15];   // nc
          pa[6] = rx_buff[16];   // nc   
          pa[9] = rx_buff[19];   // если 0x30 - не спрашиваем пароль, 0x31 - не спрашиваем у мастера, все остальное - всегда спрашиваем 
          pa[10] = rx_buff[20];  // если 0x31, то говорим вводимые цифры голосом, любое другое - молчим

          st++; 
        }
        else if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} 
        mess=0; 
        delay_ms(10);
      }
    }break;
  case 8: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break;
  case 9: { put_message_modem(check_master_num); st++; timeout=300; }break;
  case 10: 
    { 
     if (mess)
     {      
       if (!strncmp_P(rx_buff,cbpr,6))  
       {
         Get_Number(rx_buff,mass_master_number);
         st++; 
       }
       else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++;}  
       mess=0;
     }
    }break;
  case 11: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st-=2; err_cnt++; } }  mess=0;} }break;  
/*
  case 12: 
    {
      switch(i)
      {
      case 0:{ put_message_modem(check_number1); }break; 
      case 1:{ put_message_modem(check_number2); }break; 
      case 2:{ put_message_modem(check_number3); }break; 
      case 3:{ put_message_modem(check_number4); }break; 
      case 4:{ put_message_modem(check_number5); }break;  
      }
       st++; timeout=300; 
    }break;
  case 13: 
    {
      if (mess)
      {
       if (!strncmp_P(rx_buff,cbpr,6)) 
       {
        if (rx_buff[10]=='*') cm[i]=0; else cm[i]=1; // здесь начало номера точно известно и смотрим только 1-й символ
        st++;
       }
       else if (!strncmp_P(rx_buff,error,2)) { st--; err_cnt++; } 
       mess=0;
      }
    }break;
  case 14: 
    { 
      if (mess) 
      { if (!strncmp_P(rx_buff,ok,2)) 
         { if(i==4) st++; else { i++; st-=2; } } // читаем 5 номеров
        else  if (!strncmp_P(rx_buff,error,5)) { st-=2; err_cnt++; }  
       mess=0;
      } 
     }break;   
*/  
  case 12: { end=1;  }
  
  }//switch
  if (!mess) Parser();
  if (timeout==0) return STATUS_TIMEOUT;
  if (err_cnt>10) return STATUS_ERROR;
 }//while
 return STATUS_OK;
}
//----------------------------------------------------------------------------------
StatusTypeDef Write_SIM_default(void)
{
  char st=0, end=0;
  mess=0;
  while(!end)
  {
    __watchdog_reset();
    switch (st)
    {
    case 0: { put_message_modem (set_gsm); st++; timeout=100; }break; // пишем настройки SIM по умолчанию
    case 1: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 2: { put_message_modem (set_master); st++; timeout=100; }break;
    case 3: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 4: { put_message_modem (set_parol); st++; timeout=100; }break;
    case 5: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 6: { put_message_modem (set_config); st++; timeout=100; }break;
    case 7: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 8: { put_message_modem (set_number1); st++; timeout=100; }break;
    case 9: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 10: { put_message_modem (set_number2); st++; timeout=100; }break;
    case 11: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 12: { put_message_modem (set_number3); st++; timeout=100; }break;
    case 13: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 14: { put_message_modem (set_number4); st++; timeout=100; }break;
    case 15: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break;
    case 16: { put_message_modem (set_number5); st++; timeout=100; }break;
    case 17: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break; // закончили новые настройки   
    case 18: { end=1; }
    
    }
    if (!mess) Parser();
    if (timeout==0) return STATUS_TIMEOUT;
  }
return STATUS_OK;  
}
//------------------------------------------------------------------------------
// number - номер на который шлем (0-master, 1..5), text - текст (русский), type - тип смс
StatusTypeDef SIM800_SMS_Send(unsigned char number, char *text, char type) 
{
 char end=0;
 char st=0;
 char err_cnt=0; 
 char err_flag=0;
 
// if (sms_razresh==0) return STATUS_OK; // если запрещено посылать СМС, то сразу выйдем

 while(!end)
 { 
  __watchdog_reset(); 
  switch(st)
  {
  case 0:
    {
     switch (number)
     {
     case 0:{ put_message_modem (check_master_num); }break; 
     case 1:{ put_message_modem (check_number1); }break; 
     case 2:{ put_message_modem (check_number2); }break; 
     case 3:{ put_message_modem (check_number3); }break; 
     case 4:{ put_message_modem (check_number4); }break; 
     case 5:{ put_message_modem (check_number5); }break; 
     }
     timeout=300; st++;
    }break; 
  case 1:
    {
     if (mess)
      {
       if (!strncmp_P(rx_buff,cbpr,6)) 
       {
         Get_Number(rx_buff,mass_number);
         st++;
       }
      }
    }break;
  case 2:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st-=2; err_cnt++; } }  mess=0;} }break;
  case 3:{ put_message_modem (set_unicod); st++; timeout=200; }break;
  case 4:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break; 
  case 5:{ if(type) put_message_modem (sms_parameters_soxr); else put_message_modem (sms_parameters); st++; timeout=200; }
  case 6:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break; 
  case 7:
    {       
     put_message_modem (sms_number); // AT+CMGS="
     char i=0;
     while(mass_number[i]!=0) // пошлем считанный номер
     {
      putchar_modem(0x30); putchar_modem(0x30); // 00
      if (mass_number[i]=='+') { putchar_modem(0x32); putchar_modem(0x42); } //2B      
      else { putchar_modem(0x33); putchar_modem(mass_number[i]); }       
      i++;
     }
     putchar_modem(0x22); putchar_modem(0x0D); // "/r
     st++; timeout=300;
     Delay_ms(del); // не торопимся, а то может заглючить :)
    }break;// теперь надо поймать ">" 
  case 8:{ if (!strncmp(rx_buff,"> ",2)){ st++;  rx_cnt=0; mess=0; } }break; // ждем >
  case 9:
    { 
      
      char aaa[5]; u16 tmp;  // aaa[5] вместо aaa[4] т.к. надо учесть конец строки, иначе глюки с памятью вылезают
      while(*text)
      {
       if (*text>=192) tmp=*text+0x350; else tmp=*text; // со 192 начинаются русские символы, +0x350 - переводим в юникод
       sprintf(aaa,"%4.4X",tmp);
       put_message_modem_RAM(aaa);
       *text++;
      }
      putchar_modem(0x1A);  // Ctrl-Z
      st++;
      timeout=6000; // 60 s
      mess=0; //?? 
      Delay_ms(del);  // не торопимся, а то может заглючить :)
    }break;
  case 10:
    { //должно придти +CMGS:
      if (mess) 
       { 
         if (!strncmp(rx_buff,"+CMGS:",6)) st++;
         if (!strncmp_P(rx_buff,error,5) || !strncmp(rx_buff,"+CME ERROR:",11)) {mess=0; err_flag=1; st=12; } // может придти ERROR или +CME ERROR: выход с ошибкой
         mess=0;
       }  
    }break;
  case 11:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break; 
  case 12:{ put_message_modem (set_gsm); st++; timeout=200; }break; // вернем кодировку GSM
  case 13:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break;
  case 14:{ end=1; }break;
  }
  if (!mess) Parser();
  if(timeout==0) { while(1); /*return STATUS_TIMEOUT;*/ } // здесь циклимся, чтоб если отправку заглючит - пересброситься - в принципе это костыль
  if (err_cnt>10) return STATUS_ERROR;  
 }//while
 if (err_flag) return STATUS_ERROR;
 else return STATUS_OK; 
}
//------------------------------------------------------------------------------


//----------------------------------------------------------------------------------
StatusTypeDef SIM800_SMS_Read(void)
{
  char st=0, end=0;
  char err_cnt=0;
  char *s;
  char sms_index;
  char corr_sms=0; // флаг что смс наша
   
  // сюда переходим по +CMTI: и в sms_buff строка вида +CMTI: "SM",1 
  s=sms_buff;
  while(*s && *s++!=','); // ищем запятую
  sms_index=*s;                                  // символ после запятой - sms_index  (1..9) /индекс sms может быть только 1..9, т.к. все смс мы постоянно стираем (99% это будет 1, но возможно все :) )
  if (sms_index<0x31 || sms_index>0x39) st=20; // если индекс не 1..9, то стираем все - заглушка
  
  
  while(!end)
  {
    __watchdog_reset();
    switch(st)
    {
    case 0:{ put_message_modem (set_gsm); st++; timeout=200; }break;
    case 1:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; }   mess=0;} }break; 

    case 2:{ put_message_modem(sms_read); putchar_modem(sms_index); putchar_modem(0x0D); Delay_ms(10); st++; timeout=300;}break;
    case 3:
      { 
        if (mess)
        {
          if (!strncmp_P(rx_buff,cmgr,6)) st=10; // тут можно добавить выдирание номера отправителя и проверку на белый список и т.п.
          else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; }; 
          mess=0;
        }
      }break;
    case 10:  // тут в буфере имеем текст смс
      { 
        if (mess)
        {
          if(rx_buff[0]=='P' || rx_buff[0]=='p') { strcpy(sms_buff,rx_buff); corr_sms=1;  } // в sms_buff копируем текст для дальнейшего разбора
          else  corr_sms=0;  // смс начинается не с P (p) - стираем его нахер 
          mess=0;     
          st++;
        }
      }break;
    case 11:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else if (!strncmp_P(rx_buff,error,5)) { st-=2; err_cnt++; }   mess=0;} }break;  
    case 12:{ if (corr_sms) st++; else st=20; }break;
    case 13:{ if(p[0]==sms_buff[1] && p[1]==sms_buff[2] && p[2]==sms_buff[3] && p[3]==sms_buff[4]) st++; else st=20; }break; // проверяем пароль, если не подходит - стираем смс
    case 14:{ Execute_SMS_Command(sms_buff); st=20; }break; // выполняем команду и стираем смс
    
    case 20:{ put_message_modem (sms_del_all); st++; timeout=200; }break;
    case 21:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; };   mess=0;} }break;
    case 22:{ end=1; }break; 

    }//switch
    if (!mess) Parser();
    if (timeout==0) return STATUS_TIMEOUT;
    if (err_cnt>10) return STATUS_ERROR;
  }//while
return STATUS_OK;
}
//----------------------------------------------------------------------------------
void Execute_SMS_Command(char *text)
{
  char tmp_answ[40]; // для добавления ответа к стандартным смс
  
  memmove(text,text+5,strlen(text));  // сдвигаем строку на 5 символов влево - в теории не должно заглючить, непонятно тольно почему копируется strlen(text) символов 
  
  sprintf_P(answer,err_cmd); // "Ошибка в команде" - если не будет совпадений по командам - отправится эта СМС
  
  if (!strncmp_P(text,cmd_11,3) || !strncmp_P(text,RELE1_ON,8) || !strncmp_P(text,rele1_on,8)) 
  { 
    rele1=1;  relay_status[1]=1; sprintf_P(answer,rele_on,1); //  Реле 1 включено
    if (pa[1]==0x31) { if(relay_zapret[0]!=1) relay_zapret[0]=1; strcat_P(answer,t_reg_off); } // если канал не отключен в SIM - вырубим терморегулятор
  }     
  else if (!strncmp_P(text,cmd_10,3) || !strncmp_P(text,RELE1_OFF,9) || !strncmp_P(text,rele1_off,9)) 
  { 
    rele1=0;  relay_status[1]=0; sprintf_P(answer,rele_off,1); //  Реле 1 выключено
    if (pa[1]==0x31) { if(relay_zapret[0]!=1) relay_zapret[0]=1; strcat_P(answer,t_reg_off); }
  }    
  else if (!strncmp_P(text,cmd_21,3) || !strncmp_P(text,RELE2_ON,8) || !strncmp_P(text,rele2_on,8)) 
  { 
    rele2=1; relay_status[2]=1; sprintf_P(answer,rele_on,2);
    if (pa[2]==0x31) { if(relay_zapret[1]!=1) relay_zapret[1]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_20,3) || !strncmp_P(text,RELE2_OFF,9) || !strncmp_P(text,rele2_off,9)) 
  {
    rele2=0;  relay_status[2]=0; sprintf_P(answer,rele_off,2); 
    if (pa[1]==0x31) { if(relay_zapret[1]!=1)  relay_zapret[1]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_31,3) || !strncmp_P(text,RELE3_ON,8) || !strncmp_P(text,rele3_on,8)) 
  { 
    rele3=1;  relay_status[3]=1; sprintf_P(answer,rele_on,3); 
    if (pa[3]==0x31) { if(relay_zapret[2]!=1)  relay_zapret[2]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_30,3) || !strncmp_P(text,RELE3_OFF,9) || !strncmp_P(text,rele3_off,9)) 
  { 
    rele3=0;  relay_status[3]=0; sprintf_P(answer,rele_off,3);
    if (pa[3]==0x31) { if(relay_zapret[2]!=1)  relay_zapret[2]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_41,3) || !strncmp_P(text,RELE4_ON,8) || !strncmp_P(text,rele4_on,8)) 
  { 
    rele4=1; relay_status[4]=1; sprintf_P(answer,rele_on,4); 
    if (pa[4]==0x31) { if(relay_zapret[3]!=1) relay_zapret[3]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_40,3) || !strncmp_P(text,RELE4_OFF,9) || !strncmp_P(text,rele4_off,9)) 
  {
    rele4=0; relay_status[4]=0; sprintf_P(answer,rele_off,4); 
    if (pa[4]==0x31) { if(relay_zapret[3]!=1) relay_zapret[3]=1; strcat_P(answer,t_reg_off); }
  }
  else if (!strncmp_P(text,cmd_51,3) || !strncmp_P(text,RELE5_ON,8) || !strncmp_P(text,rele5_on,8)) 
  { rele5=1; relay_status[5]=1; sprintf_P(answer,rele_on,5); }
  else if (!strncmp_P(text,cmd_50,3) || !strncmp_P(text,RELE5_OFF,9) || !strncmp_P(text,rele5_off,9)) 
  { rele5=0; relay_status[5]=0; sprintf_P(answer,rele_off,5); }
  else if (!strncmp_P(text,cmd_61,3) || !strncmp_P(text,RELE6_ON,8) || !strncmp_P(text,rele6_on,8)) 
  { rele6=1; relay_status[6]=1; sprintf_P(answer,rele_on,6); }
  else if (!strncmp_P(text,cmd_60,3) || !strncmp_P(text,RELE6_OFF,9) || !strncmp_P(text,rele6_off,9)) 
  { rele6=0; relay_status[6]=0; sprintf_P(answer,rele_off,6); }
  else if (!strncmp_P(text,cmd_71,3) || !strncmp_P(text,RELE7_ON,8) || !strncmp_P(text,rele7_on,8)) 
  { rele7=1; relay_status[7]=1; sprintf_P(answer,rele_on,7); }
  else if (!strncmp_P(text,cmd_70,3) || !strncmp_P(text,RELE7_OFF,9) || !strncmp_P(text,rele7_off,9)) 
  { rele7=0; relay_status[7]=0; sprintf_P(answer,rele_off,7); } 
  
  else if (!strncmp_P(text,cmd_811,4) || !strncmp_P(text,TERM1_ON,8) || !strncmp_P(text,term1_on,8)) 
  {
    if (pa[1]==0x31) { if(relay_zapret[0]==1) relay_zapret[0]=0; sprintf_P(answer,t_reg_N_on,1); } // если канал не отключен в SIM - вырубим терморегулятор
    else { sprintf_P(answer,dat4ik_off,1); }                                                       // или скажем что датчик отключен
  }
  else if (!strncmp_P(text,cmd_810,4) || !strncmp_P(text,TERM1_OFF,9) || !strncmp_P(text,term1_off,9))  
  { 
    if (pa[1]==0x31) { if(relay_zapret[0]==0) relay_zapret[0]=1; sprintf_P(answer,t_reg_N_off,1); }
    else { sprintf_P(answer,dat4ik_off,1); }      
  }
  else if (!strncmp_P(text,cmd_821,4) || !strncmp_P(text,TERM2_ON,8) || !strncmp_P(text,term2_on,8)) 
  { 
    if (pa[2]==0x31) { if(relay_zapret[1]==1) relay_zapret[1]=0; sprintf_P(answer,t_reg_N_on,2); }
    else { sprintf_P(answer,dat4ik_off,2); }  
  }
  else if (!strncmp_P(text,cmd_820,4) || !strncmp_P(text,TERM2_OFF,9) || !strncmp_P(text,term2_off,9))  
  { 
    if (pa[2]==0x31) { if(relay_zapret[1]==0) relay_zapret[1]=1; sprintf_P(answer,t_reg_N_off,2); }
    else { sprintf_P(answer,dat4ik_off,2); }
  }
  else if (!strncmp_P(text,cmd_831,4) || !strncmp_P(text,TERM3_ON,8) || !strncmp_P(text,term3_on,8)) 
  { 
    if (pa[3]==0x31) { if(relay_zapret[2]==1) relay_zapret[2]=0; sprintf_P(answer,t_reg_N_on,3); }
    else { sprintf_P(answer,dat4ik_off,3); }
  }
  else if (!strncmp_P(text,cmd_830,4) || !strncmp_P(text,TERM3_OFF,9) || !strncmp_P(text,term3_off,9))  
  { 
    if (pa[3]==0x31) { if(relay_zapret[2]==0) relay_zapret[2]=1; sprintf_P(answer,t_reg_N_off,3); }
    else { sprintf_P(answer,dat4ik_off,3); }
  }
  else if (!strncmp_P(text,cmd_841,4) || !strncmp_P(text,TERM4_ON,8) || !strncmp_P(text,term4_on,8)) 
  { 
    if (pa[4]==0x31) { if(relay_zapret[3]==1) relay_zapret[3]=0; sprintf_P(answer,t_reg_N_on,4); }
    else { sprintf_P(answer,dat4ik_off,4); }
  }
  else if (!strncmp_P(text,cmd_840,4) || !strncmp_P(text,TERM4_OFF,9) || !strncmp_P(text,term4_off,9))  
  { 
    if (pa[4]==0x31) { if(relay_zapret[3]==0) relay_zapret[3]=1; sprintf_P(answer,t_reg_N_off,4); }
    else { sprintf_P(answer,dat4ik_off,4); }
  }
  
  //--------------------------------------      
  
  else if (!strncmp_P(text,cmd_00,3) || !strncmp_P(text,STATUS,6) || !strncmp_P(text,status,6)) 
  {
    if(pa[1]==0x31) { if (!vv[0]) sprintf_P(answer,t_N_ok,1,temp[0]); else sprintf_P(answer,t_N_err,1); } else sprintf_P(answer,t_N_off,1);
    if(pa[2]==0x31) { if (!vv[1]) sprintf_P(tmp_answ,t_N_ok,2,temp[1]); else sprintf_P(tmp_answ,t_N_err,2);} else sprintf_P(tmp_answ,t_N_off,2);
    strcat(answer,tmp_answ);
    if(pa[3]==0x31) { if (!vv[2]) sprintf_P(tmp_answ,t_N_ok,3,temp[2]); else sprintf_P(tmp_answ,t_N_err,3); } else sprintf_P(tmp_answ,t_N_off,3);
    strcat(answer,tmp_answ);
    if(pa[3]==0x31) { if (!vv[3]) sprintf_P(tmp_answ,t_N_ok,4,temp[3]); else sprintf_P(tmp_answ,t_N_err,4); } else sprintf_P(tmp_answ,t_N_off,4);
    strcat(answer,tmp_answ);      
    sprintf_P(tmp_answ,rele_N,1,relay_status[1]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,2,relay_status[2]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,3,relay_status[3]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,4,relay_status[4]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,hyster,hyst); strcat(answer,tmp_answ);
  }
  else if (!strncmp_P(text,cmd_01,3))
  {
    if(pa[1]==0x31) { if (!vv[0]) sprintf_P(answer,t_N_ok,1,temp[0]); else sprintf_P(answer,t_N_err,1); } else sprintf_P(answer,t_N_off,1);
    sprintf_P(tmp_answ,us,1,ust[0]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,1,relay_status[1]); strcat(answer,tmp_answ);
    if (relay_zapret[0]==1) sprintf(tmp_answ,"Term OFF"); else sprintf(tmp_answ,"Term ON");
    strcat(answer,tmp_answ);
  }
  else if (!strncmp_P(text,cmd_02,3))
  {
    if(pa[2]==0x31) { if (!vv[1]) sprintf_P(answer,t_N_ok,2,temp[1]); else sprintf_P(answer,t_N_err,2); } else sprintf_P(answer,t_N_off,2);
    sprintf_P(tmp_answ,us,2,ust[1]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,2,relay_status[2]); strcat(answer,tmp_answ);
    if (relay_zapret[1]==1) sprintf(tmp_answ,"Term OFF"); else sprintf(tmp_answ,"Term ON");
    strcat(answer,tmp_answ); 
  }
  else if (!strncmp_P(text,cmd_03,3))
  {
    if(pa[3]==0x31) { if (!vv[2]) sprintf_P(answer,t_N_ok,3,temp[1]); else sprintf_P(answer,t_N_err,3); } else sprintf_P(answer,t_N_off,3);
    sprintf_P(tmp_answ,us,3,ust[2]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,3,relay_status[3]); strcat(answer,tmp_answ);
    if (relay_zapret[2]==1) sprintf(tmp_answ,"Term OFF"); else sprintf(tmp_answ,"Term ON");
    strcat(answer,tmp_answ);   
  }
  else if (!strncmp_P(text,cmd_04,3))
  {
    if(pa[4]==0x31) { if (!vv[3]) sprintf_P(answer,t_N_ok,4,temp[1]); else sprintf_P(answer,t_N_err,4);} else sprintf_P(answer,t_N_off,4);
    sprintf_P(tmp_answ,us,4,ust[3]); strcat(answer,tmp_answ);
    sprintf_P(tmp_answ,rele_N,4,relay_status[4]); strcat(answer,tmp_answ);
    if (relay_zapret[3]==1) sprintf(tmp_answ,"Term OFF"); else sprintf(tmp_answ,"Term ON");
    strcat(answer,tmp_answ);  
  }
  
  //--------------------------------------
  else if (text[0]=='#' || !strncmp_P(text,SET,3) || !strncmp_P(text,set,3)) // #1025, SET1025
  {
    char temp[4]={0,0,0,0};  // 4 т.к. строка завершается \0
    char tmp_ust=0, err_ust=0;
    char k=0;
    if (text[0]=='#') k=2; else k=4; // #1123 и SET1123 - уставка начинается или с 3 или с 5 символа
    for(int i = 0; i < 3; ++i)
    {
      if (text[i+k]>=0x30 && text[i+k]<=0x39) temp[i]=text[i+k]; // проверяем что в уставке только цифры
      else err_ust=1;
    }
    if (!err_ust && (text[k-1]>=0x30 && text[k-1]<=0x34)) // если уставка нормальная и символ номера канала (k-1) = 0, 1...4 
    {
      tmp_ust=atoi(temp); // декодируем уставку в число
      if (tmp_ust>120) tmp_ust=120; // ограничение уставки 120 град
      if (tmp_ust<1) tmp_ust=1; 
      switch (text[k-1]) // номер канала перед уставкой на месте (k-1)
      {
      case 0x31:{ust_m1=tmp_ust; ust[0]=tmp_ust; sprintf_P(answer,ustavka,1,ust[0]); }break; // уст1
      case 0x32:{ust_m2=tmp_ust; ust[1]=tmp_ust; sprintf_P(answer,ustavka,2,ust[1]); }break; // уст2
      case 0x33:{ust_m3=tmp_ust; ust[2]=tmp_ust; sprintf_P(answer,ustavka,3,ust[2]); }break; // уст3
      case 0x34:{ust_m4=tmp_ust; ust[3]=tmp_ust; sprintf_P(answer,ustavka,4,ust[3]); }break; // уст4
      case 0x30:
        {
          if (tmp_ust>30) tmp_ust=30;
          else if (tmp_ust<5) tmp_ust=5;
          range_m=tmp_ust; range=tmp_ust;
          sprintf_P(answer,level,range); //  уровень .. градусов
        }break; // уровень 
      }
    }//err_ust
  }//#
  //---------------------------------------
  else if (text[0]=='h' || text[0]=='H' || !strncmp_P(text,HYS,3) || !strncmp_P(text,hys,3)) // гистерезис  h05   H30
  {
    char temp[3]={0,0,0};  // 3 т.к. строка завершается \0
    char tmp_h=0, err_h=0; 
    char k=0;
    if (text[1]=='y'|| text[1]=='Y') k=3; else k=1; // H12 и HYS12 - уставка начинается или с 2 или с 4 символа
    for(int i = 0; i < 2; ++i)
    {
      if (text[i+k]>=0x30 && text[i+k]<=0x39) temp[i]=text[i+k]; // проверяем что в уставке только цифры
      else err_h=1;
    }
    if (!err_h) // если  нормальная
    {
      tmp_h=atoi(temp); // декодируем  в число
      if (tmp_h>30)  tmp_h=30;  // ограничение 30
      if (tmp_h<2)  tmp_h=2; 
      hyst=tmp_h; hyst_m=hyst; sprintf_P(answer,hysterezis,hyst); 
    }   
  }
  //--------------------------------------
  else if(!strncmp_P(text,master,6) || !strncmp_P(text,master_sm,6)) 
  {
    memmove(text,text+6,strlen(text)); 
    if (SIM800_Write_to_SIM(text,0,"MASTER")==STATUS_OK) sprintf_P(answer,mas_ok); // если все норм
  } 
  else if(!strncmp_P(text,number1,7) || !strncmp_P(text,number1_sm,7)) { memmove(text,text+7,strlen(text)); if (SIM800_Write_to_SIM(text,1,"NUMBER1")==STATUS_OK) sprintf_P(answer,n_ok,1); } 
  else if(!strncmp_P(text,number2,7) || !strncmp_P(text,number2_sm,7)) { memmove(text,text+7,strlen(text)); if (SIM800_Write_to_SIM(text,1,"NUMBER2")==STATUS_OK) sprintf_P(answer,n_ok,2); }  
  else if(!strncmp_P(text,number3,7) || !strncmp_P(text,number3_sm,7)) { memmove(text,text+7,strlen(text)); if (SIM800_Write_to_SIM(text,1,"NUMBER3")==STATUS_OK) sprintf_P(answer,n_ok,3); }
  else if(!strncmp_P(text,number4,7) || !strncmp_P(text,number4_sm,7)) { memmove(text,text+7,strlen(text)); if (SIM800_Write_to_SIM(text,1,"NUMBER4")==STATUS_OK) sprintf_P(answer,n_ok,4); }
  else if(!strncmp_P(text,number5,7) || !strncmp_P(text,number5_sm,7)) { memmove(text,text+7,strlen(text)); if (SIM800_Write_to_SIM(text,1,"NUMBER5")==STATUS_OK) sprintf_P(answer,n_ok,5); }
  else if(!strncmp_P(text,config,6) || !strncmp_P(text,config_sm,6)) 
  { 
    memmove(text,text+6,strlen(text)); 
    if (SIM800_Write_to_SIM(text,2,"CONFIG")==STATUS_OK) 
    { // *000011****
      pa[1] = text[1] ;  // терморег1
      pa[2] = text[2] ;  // терморег2
      pa[3] = text[3];   // терморег3
      pa[4] = text[4];   // терморег4
      pa[5] = text[5];   // nc
      pa[6] = text[6];   // nc  
      pa[9] = text[9];   // если 0x30 - не спрашиваем пароль, 0x31 - не спрашиваем у мастера, все остальное - всегда спрашиваем
      pa[10] = text[10];  // если 0x31, то говорим вводимые цифры голосом, любое другое - молчим   
      sprintf_P(answer,cfg_ok);   
    }
  }
  
  else if(!strncmp_P(text,parol,5) || !strncmp_P(text,parol_sm,5)) 
  { //  *1*1*1*1234
    memmove(text,text+5,strlen(text)); 
    if (SIM800_Write_to_SIM(text,3,"PAROL")==STATUS_OK) 
    {
      p[0]=text[7]; p[1]=text[8]; p[2]=text[9]; p[3]=text[10];
      power_mon_bit=text[1]-0x30; 
      sms_mode=text[3]-0x30;
      sms_razresh=rx_buff[15]-0x30; // разрешение посылки SMS
    }
  }
  
  else if(!strncmp_P(text,rst,5)) 
  { 
    ust_m1=25; ust_m2=25; ust_m3=25; ust_m4=25;                                   // setup не трогаем т.к. модем перенастраивать не надо
    relay_zapret[0]=0; relay_zapret[1]=0; relay_zapret[2]=0; relay_zapret[3]=0;
    hyst_m=2; 
    range_m=10;  
    ust[0]=ust_m1; ust[1]=ust_m2; ust[2]=ust_m3; ust[3]=ust_m4; hyst=hyst_m; range=range_m; // считаем настройки с EEPROM
    
    if(Write_SIM_default()==STATUS_OK) 
    { 
      SSB_Config(); // здесь после сброса надо перечитать все
      sprintf_P(answer,default_settings);  
    }
  } // сбросим все
  
  else if (!strncmp_P(text,restart,7)) while(1);  // экстренная команда, вдруг глючит, а смс пробъется. ответной смс тут не будет, а прибор перезапустится
  
  // ответ на команду посылаем всегда, независимо от настройки
  SIM800_SMS_Send(0,answer,sms_mode);// отошлем ответ мастеру (если выбило по таймауту, то значит с отсылкой что то не то - перезапустимся на всякий (написано внутри)) 
  
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
StatusTypeDef SIM800_Write_to_SIM(char *text, char cell, char *name) 
{
  char end=0, st=0, corr_cell=0;
  char err_cnt=0;
  char n[max_numb_len]; // записываемый в ячейку номер
  char cnt=0;
  char write_cell[60]; // AT+CPBW=5,"",129,"NUMBER2" - (26 символов + max_numb_len) 60 хватит
  // делаем защиту от дебилов - проверяем корректность пришедшей смс в зависимости от того, какую ячейку пишем
  switch (cell)
  {
  case 0: // проверяем Master
    {
     while(*text)
     {
      if ((*text>=0x30 || *text<=0x39 || *text=='+') && *text!=0) { n[cnt++]=*text; corr_cell=1; } else corr_cell=0;
      *text++;
     }
     if (cnt<=(max_numb_len-1)) n[cnt]=0; else n[max_numb_len-1]=0; // завершим строку (с защитой от переполнения) 
     sprintf_P(write_cell,write_number,1,n,name);
    }break; 
  case 1: // проверяем Number1..5
    {
      if (text[0]=='*') // если 1-я * - то пишем только её
      {
       n[0]='*'; n[1]=0x00; corr_cell=1;
      }
      else // или пишем нормальный номер, начинающийся с 8 или +
      {
       while(*text)
       {
        if ((*text>=0x30 || *text<=0x39 || *text=='+') && *text!=0) { n[cnt++]=*text; corr_cell=1; } else corr_cell=0;
        *text++;
       }
       if (cnt<=(max_numb_len-1)) n[cnt]=0; else n[max_numb_len-1]=0; // завершим строку (с защитой от переполнения)
      }
     switch(name[6])
     {
     case '1':{sprintf_P(write_cell,write_number,4,n,name);}break;
     case '2':{sprintf_P(write_cell,write_number,5,n,name);}break;
     case '3':{sprintf_P(write_cell,write_number,6,n,name);}break;
     case '4':{sprintf_P(write_cell,write_number,7,n,name);}break;
     case '5':{sprintf_P(write_cell,write_number,8,n,name);}break;
     }
    }break;
  case 2: // проверяем Config
    {
     for(int i = 0; i < 11; ++i)
      {
        if ((*text>=0x30 || *text<=0x39 || *text=='*') && *text!=0) { n[i]=*text; corr_cell=1; } else corr_cell=0;
       *text++;
      }
      n[11]=0;  
     sprintf_P(write_cell,write_number,3,n,name);
     if ( n[0]!='*' || n[7]!='*' || n[8]!='*' || 
          (n[9]!='*' && n[9]!='0' && n[9]!='1') || 
            (n[10]!='*' && n[10]!='1') || 
             (n[1]!='0' && n[1]!='1') || (n[2]!='0' && n[2]!='1') || (n[3]!='0' && n[3]!='1') ||
               (n[4]!='0' && n[4]!='1') || (n[5]!='0' && n[5]!='1') || (n[6]!='0' && n[6]!='1') ) corr_cell=0; // // проверили корректность ячейки CONFIG
    }break;
  case 3: // проверяем Parol
    {
     for(int i = 0; i < 11; ++i)
      {
        if ((*text>=0x30 || *text<=0x39 || *text=='*') && *text!=0) { n[i]=*text; corr_cell=1; } else corr_cell=0;
       *text++;
      }
      n[11]=0;   
      sprintf_P(write_cell,write_number,2,n,name);  
     if ( n[0]!='*' || n[2]!='*' || n[4]!='*' || n[6]!='*' || 
          (n[1]!='0' && n[1]!='1') || (n[3]!='0' && n[3]!='1') || (n[5]!='0' && n[5]!='1') ||
            (n[7]<0x30 && n[7]>0x39) || (n[8]<0x30 && n[8]>0x39) || (n[9]<0x30 && n[9]>0x39) || (n[10]<0x30 && n[10]>0x39) ) corr_cell=0; // проверили корректность ячейки PAROL
                                                                                                                 
    }break;
  }//switch
  
if (corr_cell)
{
  while (!end)
  {
    switch(st) //AT+CSCS="GSM" уже сделано при чтении смс
    {
    case 0: { put_message_modem_RAM(write_cell); st++; timeout=100;}break;
    case 1: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break;
    case 2: { if (cell==0) sprintf(mass_master_number,n); end=1; }break; // если писали мастер номер - то скопируем его в массив и выйдем
    }
    if (!mess) Parser();
    if (timeout==0) return STATUS_TIMEOUT; 
    if (err_cnt>10) return STATUS_ERROR;
  }
  return STATUS_OK;
} 
return STATUS_ERROR;
}
//------------------------------------------------------------------------------
void Check_Incoming_Call(void)
{
  char st=0, end=0;
  char attempt=3; // кол-во попыток ввода пароля
 
  
  overflow=15*100; // 15 сек
  dtmf_cnt=0;
  while(!end)
  {
    __watchdog_reset();
    switch(st)
    {
    case 0:{ put_message_modem (ata); st++; }break; 
    case 1:{ if (mess) {  if (!strncmp_P(rx_buff,ok,2)) st++; else {} mess=0; } }break;
    case 2:
      { 
        Delay_ms(300); 
         if ((ring_master && pa[9]==0x31) || pa[9]==0x30) { gudok_ok(); st=6; }  // если звоним с мастер-номера и не надо пароль или просто пароль выключен
         else  { gudok(); st++; } 
      }break;
    case 3:
      {
        if(attempt!=0) {dtmf_cnt=0; st++; } else st=10; // если не кончились попытки ввода, то ждем пароля, иначе повесим трубку и выйдем
      }break;
    case 4:
      { 
        if(dtmf_cnt==4) st++; // overflow обновляется при каждом нажатии кнопки (приходе +DTMF) в парсере
        if (mess) { if (!strncmp_P(rx_buff,no_carrier,strlen_P(no_carrier))) end=1; mess=0;}
      }break;// ждем 4-х символов dtmf
    case 5:
      { 
        if (!strncmp(dtmf_buff,p,4)) { st++; Delay_ms(500); password(); Delay_ms(200); prinyat(); } // пароль совпал
        else 
         { 
           st=3; attempt--;
           Delay_ms(300); say_error(); // говорим ошибка
         }
      }break;
    case 6:{ Check_DTMF_Cmd(); end=1;  }break;  
    
    case 10:{ put_message_modem(hold);  st++; timeout=200; }break;
    case 11:
      { 
        if (mess) {  if (!strncmp_P(rx_buff,ok,2)) end=1; else {st--;} mess=0;} 
        if (timeout==0) end=1;
      }break;
    }//switch

    if (overflow==0 && st<10) st=10; 
    if (!mess) Parser();
  }
}

//------------------------------------------------------------------------------
//------------ обработка ДТМФ команд -------------------------------------------
void Check_DTMF_Cmd(void)
{
  char st=0, end=0;
  char max_count=3; // если забыть это и max_count=0, то будет нормально подглючивать из-за строчки  if(dtmf_cnt==max_count) st++; 
  while(!end)
  {
   __watchdog_reset();
   switch (st)
   {
   case 0:{ dtmf_cnt=0; dtmf_buff[0]=0; dtmf_buff[1]=0; max_count=3; overflow=15*100; st++; }break; 
   case 1:
     { 
       if (dtmf_buff[0]=='#') max_count=5;
       else if (dtmf_buff[0]=='*' && dtmf_buff[1]=='8') max_count=4; // 
         else if (dtmf_buff[0]=='*') max_count=3;
          else dtmf_cnt=0; // если не команда - то сбрасываем dtmf буфер на начало
       if(dtmf_cnt==max_count) st++; 
       if (mess) { if (!strncmp_P(rx_buff,no_carrier,strlen_P(no_carrier))) end=1; mess=0; }
     }break;
   case 2: { Execute_DTMF_Cmd(); st=0; }break;
   case 10:{ put_message_modem(hold);  st++;}break;
   case 11:{ if (mess) {  if (!strncmp_P(rx_buff,ok,2)) end=1; else {} mess=0;} }break;
   }
   if (!mess) Parser();
   if (overflow==0 && st<10) st=10;
  }  
}
//------------------------------------------------------------------------------
void Execute_DTMF_Cmd(void)
{ 
  // ручное вмешательство в работу реле 1..4 отключает терморегулятор 
  if (!strncmp_P(dtmf_buff,cmd_11,3)) 
  { 
    rele1=1;  Delay_ms(300);say_relay(); say_one(); say_relay_on();relay_status[1]=1; 
    if (pa[1]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[0]!=1) relay_zapret[0]=1; } // если канал не отключен в SIM - вырубим терморегулятор
  }
  else if (!strncmp_P(dtmf_buff,cmd_10,3)) 
  { 
    rele1=0;  Delay_ms(100);say_relay(); say_one(); say_relay_off();relay_status[1]=0; 
    if (pa[1]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[0]!=1) relay_zapret[0]=1; }
  }
  else if (!strncmp_P(dtmf_buff,cmd_21,3)) 
  { 
    rele2=1;  Delay_ms(300); say_relay(); say_two(); say_relay_on();relay_status[2]=1; 
    if (pa[2]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[1]!=1) relay_zapret[1]=1; }
  }
  else if (!strncmp_P(dtmf_buff,cmd_20,3)) 
  { 
    rele2=0; Delay_ms(100);say_relay(); say_two(); say_relay_off();relay_status[2]=0; 
    if (pa[2]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[1]!=1) relay_zapret[1]=1; }
  }
  else if (!strncmp_P(dtmf_buff,cmd_31,3)) 
  {
    rele3=1; Delay_ms(300);say_relay(); say_three(); say_relay_on();relay_status[3]=1; 
    if (pa[3]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[2]!=1) relay_zapret[2]=1; }
  }
  else if (!strncmp_P(dtmf_buff,cmd_30,3)) 
  {
    rele3=0; Delay_ms(100);say_relay(); say_three(); say_relay_off();relay_status[3]=0; 
    if (pa[3]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[2]!=1) relay_zapret[2]=1; }
  }
  else if (!strncmp_P(dtmf_buff,cmd_41,3)) 
  {
    rele4=1;Delay_ms(300);say_relay(); say_four(); say_relay_on();relay_status[4]=1; 
    if (pa[4]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[3]!=1) relay_zapret[3]=1; }   
  }
  else if (!strncmp_P(dtmf_buff,cmd_40,3)) 
  { 
    rele4=0;Delay_ms(300);say_relay(); say_four(); say_relay_off();relay_status[4]=0; 
    if (pa[4]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[3]!=1) relay_zapret[3]=1; } 
  }
  else if (!strncmp_P(dtmf_buff,cmd_51,3)) { rele5=1;Delay_ms(300);say_relay(); say_five(); say_relay_on();relay_status[5]=1; }
  else if (!strncmp_P(dtmf_buff,cmd_50,3)) { rele5=0;Delay_ms(300);say_relay(); say_five(); say_relay_off();relay_status[5]=0; }
  else if (!strncmp_P(dtmf_buff,cmd_61,3)) { rele6=1;Delay_ms(300);say_relay(); say_six(); say_relay_on();relay_status[6]=1; }
  else if (!strncmp_P(dtmf_buff,cmd_60,3)) { rele6=0;Delay_ms(300);say_relay(); say_six(); say_relay_off();relay_status[6]=0; }
  else if (!strncmp_P(dtmf_buff,cmd_71,3)) { rele7=1;Delay_ms(300);say_relay(); say_seven(); say_relay_on();relay_status[7]=1; }
  else if (!strncmp_P(dtmf_buff,cmd_70,3)) { rele7=0;Delay_ms(300);say_relay(); say_seven(); say_relay_off();relay_status[7]=0; }
    
  else if (!strncmp_P(dtmf_buff,cmd_811,4)) { if (pa[1]==0x31) { relay_zapret[0]=0; termoregulator(); say_one(); Delay_ms(300); say_on();} else {datchik();say_one();say_off();} } // включили терморегулятор или сказали что в сим датчик выключен
  else if (!strncmp_P(dtmf_buff,cmd_810,4)) { if (pa[1]==0x31) { relay_zapret[0]=1; termoregulator(); say_one(); Delay_ms(300); say_off();} else {datchik();say_one();say_off();}  }
  else if (!strncmp_P(dtmf_buff,cmd_821,4)) { if (pa[2]==0x31) { relay_zapret[1]=0; termoregulator(); say_two(); Delay_ms(300); say_on();} else {datchik();say_two();say_off();} }
  else if (!strncmp_P(dtmf_buff,cmd_820,4)) { if (pa[2]==0x31) { relay_zapret[1]=1; termoregulator(); say_two(); Delay_ms(300); say_off();} else {datchik();say_two();say_off();} }
  else if (!strncmp_P(dtmf_buff,cmd_831,4)) { if (pa[3]==0x31) { relay_zapret[2]=0; termoregulator(); say_three(); Delay_ms(300); say_on();} else {datchik();say_three();say_off();} } 
  else if (!strncmp_P(dtmf_buff,cmd_830,4)) { if (pa[3]==0x31) { relay_zapret[2]=1; termoregulator(); say_three(); Delay_ms(300); say_off();} else {datchik();say_three();say_off();}  } 
  else if (!strncmp_P(dtmf_buff,cmd_841,4)) { if (pa[4]==0x31) { relay_zapret[3]=0; termoregulator(); say_four(); Delay_ms(300); say_on();} else {datchik();say_four();say_off();}  } 
  else if (!strncmp_P(dtmf_buff,cmd_840,4)) { if (pa[4]==0x31) { relay_zapret[3]=1; termoregulator(); say_four(); Delay_ms(300); say_off();} else {datchik();say_four();say_off();} } 
  
  else if (!strncmp_P(dtmf_buff,cmd_00,3))  // проговариваем статус
  {             
    if (relay_status[1]==1) { say_relay(); say_one();   Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[2]==1) { say_relay(); say_two();   Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[3]==1) { say_relay(); say_three(); Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[4]==1) { say_relay(); say_four();  Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[5]==1) { say_relay(); say_five();  Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[6]==1) { say_relay(); say_six();   Delay_ms(200); say_relay_on(); Delay_ms(500); }
    if (relay_status[7]==1) { say_relay(); say_seven(); Delay_ms(200); say_relay_on(); Delay_ms(500); }
    
    say_temp_info(1);
    say_temp_info(2);
    say_temp_info(3);
    say_temp_info(4);
   
    Delay_ms(400); gisterezis(); Delay_ms(400); say_numeric(hyst); Delay_ms(300); say_gradus(hyst); // скажем гистерезис
    Delay_ms(400); uroven(); Delay_ms(400); say_numeric(range); Delay_ms(300); say_gradus(range); // скажем уровень
    
    if (!zb) {power();norm();} else {power();power_off();}
    if (baterry_bit==0) { Delay_ms(200); baterry_low();}    
  }// *00
  else if (!strncmp_P(dtmf_buff,cmd_01,3)) say_temp_info(1);
  else if (!strncmp_P(dtmf_buff,cmd_02,3)) say_temp_info(2);
  else if (!strncmp_P(dtmf_buff,cmd_03,3)) say_temp_info(3);
  else if (!strncmp_P(dtmf_buff,cmd_04,3)) say_temp_info(4);
  
  else if (dtmf_buff[0]=='#') // если 1-й символ #, то приняли уставку
  {
    char temp[3]; // int tmp_ust=0;
    char tmp_ust;
    temp[0]=dtmf_buff[2]; temp[1]=dtmf_buff[3]; temp[2]=dtmf_buff[4];
    tmp_ust=atoi(temp); // декодируем уставку в число
    if (tmp_ust>120) tmp_ust=120; if (tmp_ust<1) tmp_ust=1;
    switch (dtmf_buff[1])
    {
     case 0x31:{ust_m1=tmp_ust; ust[0]=tmp_ust; ustavka_(); say_one(); Delay_ms(200); say_numeric(ust[0]); Delay_ms(300); say_gradus(ust[0]); }break; // уст1
     case 0x32:{ust_m2=tmp_ust; ust[1]=tmp_ust; ustavka_(); say_two(); Delay_ms(200); say_numeric(ust[1]); Delay_ms(300); say_gradus(ust[1]); }break; // уст2
     case 0x33:{ust_m3=tmp_ust; ust[2]=tmp_ust; ustavka_(); say_three(); Delay_ms(200); say_numeric(ust[2]); Delay_ms(300); say_gradus(ust[2]); }break; // уст3
     case 0x34:{ust_m4=tmp_ust; ust[3]=tmp_ust; ustavka_(); say_four(); Delay_ms(200); say_numeric(ust[3]); Delay_ms(300); say_gradus(ust[3]); }break; // уст4
     case 0x30:
      {
       if (tmp_ust>30) tmp_ust=30;
       else if (tmp_ust<5) tmp_ust=5;
       range_m=tmp_ust; range=tmp_ust;
       uroven(); Delay_ms(400); say_numeric(range); Delay_ms(300); say_gradus(range); // говорим - уровень .. градусов
      }break; // уровень
    }      
  }

  overflow=15*100;
}

//------------------------------------------------------------------------------
void say_temp_info(char ch)
{
 if (pa[ch]==0x31) // если канал в SIM включен
 {
  switch (ch)
  {
  case 1:
    {
      if (!vv[0]) 
      { 
        temperatura(); say_one(); Delay_ms(200); say_numeric(temp[0]); Delay_ms(300); say_gradus(temp[0]); Delay_ms(300); 
        ustavka_(); say_one(); Delay_ms(200); say_numeric(ust[0]); Delay_ms(300); say_gradus(ust[0]); Delay_ms(300);
        termoregulator(); say_one(); Delay_ms(300); if (relay_zapret[0]==0) say_on(); else say_off();
      }
      else { Delay_ms(200);channel_();Delay_ms(200); say_one();Delay_ms(200);neispraven(); }       
    }break;
  case 2:
    {
      if (!vv[1]) 
      { 
        temperatura(); say_two(); Delay_ms(200); say_numeric(temp[1]); Delay_ms(300); say_gradus(temp[1]); Delay_ms(300); 
        ustavka_(); say_two(); Delay_ms(200); say_numeric(ust[1]); Delay_ms(300); say_gradus(ust[1]); Delay_ms(300);
        termoregulator(); say_two(); Delay_ms(300); if (relay_zapret[1]==0) say_on(); else say_off();
      }
      else { Delay_ms(200);channel_();Delay_ms(200); say_two();Delay_ms(200);neispraven(); }       
    }break;
  case 3:
    {
      if (!vv[2]) 
      { 
        temperatura(); say_three(); Delay_ms(200); say_numeric(temp[2]); Delay_ms(300); say_gradus(temp[2]); Delay_ms(300); 
        ustavka_(); say_three(); Delay_ms(200); say_numeric(ust[2]); Delay_ms(300); say_gradus(ust[2]); Delay_ms(300);
        termoregulator(); say_three(); Delay_ms(300); if (relay_zapret[2]==0) say_on(); else say_off();
      }
      else { Delay_ms(200);channel_();Delay_ms(200); say_three();Delay_ms(200);neispraven(); }       
    }break;
  case 4:
    {
      if (!vv[3]) 
      { 
        temperatura(); say_four(); Delay_ms(200); say_numeric(temp[3]); Delay_ms(300); say_gradus(temp[3]); Delay_ms(300); 
        ustavka_(); say_four(); Delay_ms(200); say_numeric(ust[3]); Delay_ms(300); say_gradus(ust[3]); Delay_ms(300);
        termoregulator(); say_four(); Delay_ms(300); if (relay_zapret[3]==0) say_on(); else say_off();
      }
      else { Delay_ms(200);channel_();Delay_ms(200); say_four();Delay_ms(200);neispraven(); }       
    }break;    
  }//switch
 }
 else // канал в SIM выключен
 {
  datchik(); 
  if(ch==1) say_one();
   else if(ch==2) say_two();
    else if(ch==3) say_three();
     else if(ch==4) say_four();
  say_off();    
 }
}
//------------------------------------------------------------------------------
void Termoregulator(char ch)
{
  int t_on, t_off;
  
  temp[ch-1]=Get_Temperature(ch);
  
  if(!vv[ch-1]) // если датчик в норме
  {
    if (temp[ch-1]==1000) // проверим не в аварии ли датчик
    { 
      vv[ch-1]=1; 
      if (ch==1)  rele1=0;
       else if(ch==2) rele2=0;
        else if(ch==3) rele3=0;
         else if(ch==4) rele4=0;
      relay_status[ch]=0; sprintf_P(answer,avar_dt,ch); 
      if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode); // если разрешено - пошлем СМС
    }
    else // датчик не в аварии
    {
      
      t_on  = ust[ch-1]-(hyst-hyst/2);  // температура включения
      t_off = ust[ch-1]+(hyst/2); // температура выключения


      if (relay_zapret[ch-1]==0) // если терморегулятор включен ( *8X1 )
      {   
        if (temp[ch-1]<t_on && relay_status[ch]==0)
        {
            if (ch==1)  rele1=1;
             else if(ch==2) rele2=1;
              else if(ch==3) rele3=1;
               else if(ch==4) rele4=1;
            relay_status[ch]=1; 
        }
        
        if (temp[ch-1]>=t_off && relay_status[ch]==1)
        {
            if (ch==1) rele1=0;
             else if(ch==2) rele2=0;
              else if(ch==3) rele3=0;
               else if(ch==4) rele4=0;
            relay_status[ch]=0;
        }  
      }// relay_zapret

      if ((temp[ch-1]<=(ust[ch-1]-range) || temp[ch-1]>=(ust[ch-1]+range)) && sms_stat[ch-1]==0) // Т вне пределов
      {
          sprintf_P(answer,t_out,temp[ch-1],ch);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // если разрешено - пошлем СМС
          sms_stat[ch-1]=1;
      } 
      if (temp[ch-1]>=t_on && temp[ch-1]<=t_off && sms_stat[ch-1]==1) // Т в норме
      {
          sprintf_P(answer,t_norm,temp[ch-1],ch);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // если разрешено - пошлем СМС
          sms_stat[ch-1]=0; 
      }  

    }
    
  }
   
  
}
//-==================================подпрограмма мониторенья питания системы------------------------  
void Power_Monitor(void)
{
  int u=0;
    // 180 V -330, 190 - 360, 200 - 380, 210 - 400, 220 - 430 (при выключенных реле)
    // при вкл всех реле напруга проседает: 220 -> 360
    // от аккума при всех вкл реле - 12В -> 270, выкл реле 12 V -> 270
    u=0;
    for(int i = 0; i < 5; ++i)
    {
      u+=read_adc(0);
      Delay_ms(5);
    }
    u/=5;

    if (zb==0) // если питание в норме - будем смотреть просадку
    {  
      if (u<300)
      {
        Delay_ms(300);  // исключим ложные проседания
        u=read_adc(0);
        if (u<300)
        {
          sprintf_P(answer,p_off);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // если разрешено - пошлем СМС
          zb=1;
        }
      }
    }
    else  // если питание от акума - будем смотреть вдруг уже нормальное
    { 
      if (u>345)
      {
        sprintf_P(answer,p_on);
        if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // если разрешено - пошлем СМС
        zb=0;
        baterry_bit = 1; // при появлении питания скинем бит разряженного акума
      }
      if (u<260)
      {
        if (baterry_bit)
        {
          baterry_bit = 0;
          sprintf_P(answer,batt_low);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // если разрешено - пошлем СМС
        }
      }
    }
 
}
//------------------------------------------------------------------------------
int main( void )
{ 
  init_cpu();
  
//  initI2C();
//  lcd_init();
  
  ADMUX=ADC_VREF_TYPE & 0xff;
  ADCSRA=0x87;
    
  if (setup!=1)
  {
    ust_m1=25; ust_m2=25; ust_m3=25; ust_m4=25;
    relay_zapret[0]=0; relay_zapret[1]=0; relay_zapret[2]=0; relay_zapret[3]=0;
    hyst_m=2;
    range_m=10;   
    if(SIM800_First_Setup()!=STATUS_OK) while(1); 
  }    
  
  if (SIM800_PWR_ON()!=STATUS_OK) while(1); 
  
  if (SIM800_Init()!=STATUS_OK) while(1);
  
  if (SSB_Config()!=STATUS_OK) while(1);
  
  
  ust[0]=ust_m1; ust[1]=ust_m2; ust[2]=ust_m3; ust[3]=ust_m4;
  hyst=hyst_m;  range=range_m;
  task=0;
  //--------------------================================Основной цикл======================================---------------------
  while(1)
  {
    
    __watchdog_reset();
    
    if (!mess) Parser();
    
    if (mess) //  Пришло что то с модема???
    {     
      if (!strncmp_P(rx_buff,ring,strlen_P(ring))) //  посмотрим а не пришел ли RING
      { 
        ring_cnt++;
        if (ring_cnt>1) { ring_cnt=0; Check_Incoming_Call(); } // трубку берем со 2-го гудка (2-го RING)
      }  
      mess=0;
    }
    
    if (sms_flag)// если пришло СМС, читаем его и сбрасываем флаг => готовы читать следующее
    { 
      task=1; 
      if (SIM800_SMS_Read()!=STATUS_OK) while(1); 
      sms_flag=0; task=0; 
    }
    
    
    if (power_mon_bit) Power_Monitor(); // если мониторим пропадание питалова...
    
    // Если канал задействован - смотрим темературу и работаем с ней    
    if (pa[1]==0x31) Termoregulator(1);
    if (pa[2]==0x31) Termoregulator(2);
    if (pa[3]==0x31) Termoregulator(3);
    if (pa[4]==0x31) Termoregulator(4);
    
    //----============================================================================----------------------------  
    
  }//while(1)
  
}//main
