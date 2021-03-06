//   ����-400�
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

char cycle_buff[BUFF_SIZE];  // ��������� �����
u16 r_index=0, w_index=0, rx_count=0;  // ������ ������, ������, ���-�� ���� � ������

char rx_buff[Rx_buff_size]; // ������� �����
char sms_buff[SMS_buff_size]; // ����� ���
char answer[70]; // �������� ���

u8 rx_cnt=0;
unsigned char mess; 
char dtmf_buff[6], dtmf_cnt=0; // ����� dtmf ������
u16 timeout; // ������� ������ �� ������
char task=0xFF;  // ����� ������, FF - �� ����������
                // 0 - �������� ����

/* ����� (����� ����� ������� ���� ��� ��������� � 1 ���������� ������) */   
unsigned char ok_flag, err_flag=0, sms_flag=0, cpin_rdy=0, ring_flag=0, cmgs_flag=0;
unsigned char call_rdy=0, sms_rdy=0, n_pwr_dn=0, cfun=0;


char mass_number[max_numb_len];
char mass_inc_number[max_numb_len]; // ����� ��� �������� ������
char mass_master_number[max_numb_len]; // ������ ����� (���������)
char ring_master=0; // 1 ���� ���������� ��� ������ ������ �����


unsigned char pa[11]; // ��������� ������ CONFIG *000011**11 (11 ��������) ��� � ����� �� �������, ��������� ����.
unsigned char vv[4]; // ���� � 1 �� ������ �������
char p[4]; // �������� ������ �� ����� (������ 4 ����� � ASCii)
unsigned char power_mon_bit;    // ��������� ������� ��� ���
unsigned char sms_mode;         // ��� ���
unsigned char sms_razresh;       // ���������� ������� ���

unsigned long overflow; // ������� ��� ������� ������
unsigned char ring_cnt=0;  // ���� ������ ����� �� 2-�� RING, � �� �����
unsigned char wait_on,wait_off;    // ���� �� �������� ��������� � �������� ����������
unsigned char retry_count;         // ���� �� ������� �������
unsigned char relay_status[8]={0,0,0,0,0,0,0,0};  // 0 ������� �� ���������� - ����� �� ���� �������� � ���� 



unsigned char sms_stat[4]={0,0,0,0};  // ��� ������������ ������ ���
//unsigned char cm[5]={0,0,0,0,0}; // ���� �� ������� �� ������ 1..5


unsigned char temperature_status;
unsigned char baterry_bit = 1;
unsigned char zb=0;
unsigned char ust[4], hyst;
int temp[4];
char range;


/*
char flag_rele3=0; // ���� ����, ��� ���� ������� �� ������������ ��������
unsigned long rele3_count=0; // ������� ������� ������ ������ 
*/
unsigned int reztemp;//��������� ������ ���
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
// �� ��������, ���� � �����������
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
  
  Delay_ms(del);  // �� ���������, � �� ����� ��������� :)
}

//------------------------------------------------------------------------------
void Get_Number(char *buff, char *mass)  // �������� ����� �� *buff � ������ ��� � mass
{
//  char i=0;
  while(*buff++!='"');
  while(*buff!='"') // �������� ������ ����� �� ���� "
   {
    *mass++=*buff++; // ����� ����� � mass_number
   }
  *mass=0x00; // ��������� ������ \0
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
// ������� ����� ����� �� -199 �� 199
void say_numeric(int num)
{
  if (num==0) say_zero();
  if (num<0){ minus(); num=num*(-1); } //���� ������������� ����������� ������ ����� � ������� �� �������������
  if (num>=100) { say_100(); num=num-100; Delay_ms(100); } // ������  100 - ������ ���, � �������....   
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
   num/=100; // ���� ��������� 0 � �����
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
// ������� ������, ��������, �������
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
//---------------------------���������� �� ������� ������ �� ������-------------
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
   if (rx_count > 0)  //���� ����� �� ������
   {                            
      sym = cycle_buff[r_index];              //��������� ������ �� ������
      rx_count--;                                   //��������� ������� ��������
      r_index++;                                  //�������������� ������ ������ ������
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
  // ��� SIM900 ������ �� 6 �������, ��� SIM800 - �� 7 (��� ���� �������� ������)
  // define � main.h

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
//   if(sym==0x0A && (rx_cnt!=0 && rx_buff[rx_cnt-1]==0x0D))                      // ������ 0x0A, � ����� ���� ���� 0x0D
   if (sym==0x0D)                                                               // ����� ��������� ������ �� 0x0D, � 0x0A ����������
   { 
     mess=1;                                                                    // ��������� ���� - ���� ���� - ���� ��������� �� �����������, ������� mess=0; ������ ����� ��������� ������, � �� ����� ���������
     rx_buff[rx_cnt]=0; /*rx_buff[rx_cnt-1]=0; */                               // �������� \r\n �� \0\0 (��������� ������) - ���� ��������� �������� �� �������� � �� ������� \r � �����) 
     rx_cnt=0;
     if (rx_buff[0]==0 || !strncmp_P(rx_buff,at_p,3)) mess=0;                   // ������ ������ \r\n  ��� ������ ��� �� ������ - ��� ����������                                    
//     if (rx_buff[rx_cnt-2]==0x0D) rx_buff[rx_cnt-2]=0;                          // ��� ��� �� ������ ����� ������ \r\r\n, �� ���������� ����� �� � AT - ����� �� ������ ������ ����!!
    
     // ����� ������ �� URC                                                     // 
     if (rx_buff[0]=='+')                                                       // ������ ���-��, ������������ � + -> ��������� ��� 
     {
       if (!strncmp_P(rx_buff,cpin,6))                                          // +CPIN:
       { 
         if (!strncmp_P(rx_buff,cpin_ready,12)) cpin_rdy=1;                     // +CPIN: READY                      
         if (!strncmp_P(rx_buff,cpin_not_ins,19) || 
             !strncmp_P(rx_buff,cpin_not_ready,16)) while(1);                   // +CPIN: NOT READY ������ ���� ��������� �����
         
         mess=0; 
       } 
       
       else if (!strncmp(rx_buff,"+CFUN:",6))                                   // +CFUN:
       { 
         if (!strncmp(rx_buff,"+CFUN: 1",12)) cfun=1;  
         mess=0; 
       } 
       
       else if (!strncmp_P(rx_buff,dtmf,6))                                     // +DTMF:
       { 
         if (pa[10]==0x31)// ������������� ���������� �����
           if (rx_buff[dtmf_sym_pos]>=0x30 && rx_buff[dtmf_sym_pos]<=0x39) say_numeric(rx_buff[dtmf_sym_pos]-0x30); else gudok();  
           overflow=15*100; dtmf_buff[dtmf_cnt++]=rx_buff[dtmf_sym_pos];  // ������ ������� ������ ���� ��� +15 ��� �� ���� ��� ������� ������
           if(dtmf_cnt==6) dtmf_cnt=0; 
           mess=0;                                                              // ����������� � �����, ����� �������� ������ ����� ?????
       }
       
       else if (!strncmp_P(rx_buff,cmti,6))                                     // +CMTI:
       { if (!sms_flag) { sms_flag=1; strcpy(sms_buff,rx_buff); }  mess=0; }    //������ �������� ��� ����������� ������� (���� ��� ������������ �����-�� ���, �� ��� ���������)
       
       else if (!strncmp_P(rx_buff,clip_p,6))                                   // +CLIP:
       { 
         Get_Number(rx_buff,mass_inc_number);                                   // �������� �����
         if (!strncmp(mass_master_number,mass_inc_number,strlen(mass_master_number))) ring_master=1; else ring_master=0;
         mess=0;
       }
     }// '+'
     else
     {
       if (!strncmp_P(rx_buff,ring,4) && task!=0) { ring_flag=1; mess=0; }      // ���� ���� ���-�� �� ������� ������������       // mess=0 - ���������� RING ���� �� ������ � ������ 
       else if (!strncmp_P(rx_buff,call_ready,10)) { call_rdy=1; mess=0; }      // Call Ready
       else if (!strncmp_P(rx_buff,sms_ready,9)) { sms_rdy=1; mess=0; }         // SMS Ready
       else if (!strncmp_P(rx_buff,power_dn,17)) { n_pwr_dn=1; mess=0; }        // NORMAL POWER DOWN
       //       else if (!strncmp_P(rx_buff,init_str,4)) { mess=0; rx_cnt=0; }           // ���������� IIII���� (������� ���� 1-� 4 �������, �.�. ������ - ������)
       
       
     }
   }                 
   else if (sym!=0 && sym!=0x0A) rx_buff[rx_cnt++]=sym;                                      // ��� �� ����� - ����� � �����
   
   if (rx_cnt==255) rx_cnt=0;  // ������ �� ������������ (���� �.�. rx_cnt = u8 - ��� ��������� ��������� �� ������������ )
   
 }//while
}
//------------------------------------------------------------------------------
// ������ ��� ������
void System_Error(unsigned char err)
{
 while (1)
 {
  __watchdog_reset();  
  switch (err)
  {
  case 0:{}break;
  case 1:             // ��� SIM ��� SIM ����������
    { 

    }break;
  case 2:{}break;
  }
 }
}
//------------------------------------------------------------------------------
void Hardware_PWR_ON(void)
{
 Delay_ms(100);                                                                 // ��� ���� �������� ������-�� ����� ������������ ���� n_pwr_dn
 n_pwr_dn=0; call_rdy=0; cpin_rdy=0;  sms_rdy=0; // ������� �����
 modem_power = 1;
 Delay_s(3);
 modem_power = 0; 

}
//------------------------------------------------------------------------------

StatusTypeDef SIM800_PWR_ON(void)
{ 
  if (setup==1) // �� ������ ���������
    Hardware_PWR_ON();
  else
    setup=1;    // �������� 1-� ���

 mess=0;

 char op_present=0;
 char st=0;
 char end=0;
 char err_cnt=0;

timeout=2000; // ���� ��������� Call Ready 20 ���

 while(!end)
 {
  __watchdog_reset();  

  switch (st)
  { // ��������� Call Ready �������� AT+CIURC=1 (default)
  case 0: {  if (mess) mess=0; if(call_rdy && cpin_rdy/* && sms_rdy */) st++; }break; // ���� ������� +CPIN: Ready, Call Ready � SMS Ready 

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
        if (!strncmp_P(rx_buff,cops_p,6)) // ������ +COPS:
        {
          if (!strncmp_P(rx_buff,cops_ans,12)) op_present=1; // ������ +COPS: 0,0,"
          else if (!strncmp_P(rx_buff,cops_no_ans,8)) op_present=0; // ������ +COPS: 0
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
// � ����� ���� AT&W
// ��� ��� SIM800, ��� SIM900 ����� ������ � ������ �� ���������
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
//  put_message_modem(sms_mess_ind); // ��������� ������ ���� ���� SMS Ready
//  Delay_s(2);
  put_message_modem(atw);
  Delay_s(2);
  setup=1;  
  Hardware_PWR_ON(); // �������� ����� � ���������� - ������ �������������
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
   case 0:{ put_message_modem (at); st++; timeout=500; }break; // ������� ������� ������� (5 ���), �.�. ����� ���������� � �.�.
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
  /*  �������� ����� ��������:
  AT+CSCB - ������ ����������������� ���������
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
    case 14:{ put_message_modem (sms_del_all); st++; timeout=200; }break; // ������ ��� ���
    case 15:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 16:{ put_message_modem(sms_storage_sm); st++; timeout=300; } // ����� ��� � �� ����
    case 17:{ if (mess) { if (!strncmp_P(rx_buff,cpms,6)) {mess=0; st++; } else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} } } }break;
    case 18:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st-=2; err_cnt++;} }  mess=0;} }break;
    case 19:{ put_message_modem (sms_format_text); st++; timeout=200; }break; // ��������� 1 ��� ��� ���������
    case 20:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break;     
    case 21:{ put_message_modem (dtmf_en); st++; timeout=200; }break;
    case 22:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) {st--; err_cnt++;} }  mess=0;} }break; 
    case 23:{ put_message_modem(sms_mess_ind); st++; timeout=200; }break; // ������� �� ���
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
StatusTypeDef SSB_Config(void) // �������� ������������ ������� 1..5, � ����� ���� ������ ������� ������ ��� �������� ��� � ���� ���������� � ������
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
        if (!strncmp_P(rx_buff,parol_ch,strlen_P(parol_ch))) { empty_sim=0; st++; } // �� ����� ���� ������ PAROL, �������� � st=2 (������� ������ � ������)
        else if (!strncmp_P(rx_buff,ok,2)) { empty_sim=1; st+=3;}                // ���� ��� ������, �� ����� ������� ������ �� � ���� ����������� �� st=4
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
     sms_razresh=rx_buff[15]-0x30; // ���������� ������� SMS
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
        if (!strncmp_P(rx_buff,cbpr,6)) // ����� �� SIM
        {
          pa[1] = rx_buff[11] ;  // ��������1
          pa[2] = rx_buff[12] ;  // ��������2
          pa[3] = rx_buff[13];   // ��������3
          pa[4] = rx_buff[14];   // ��������4
          pa[5] = rx_buff[15];   // nc
          pa[6] = rx_buff[16];   // nc   
          pa[9] = rx_buff[19];   // ���� 0x30 - �� ���������� ������, 0x31 - �� ���������� � �������, ��� ��������� - ������ ���������� 
          pa[10] = rx_buff[20];  // ���� 0x31, �� ������� �������� ����� �������, ����� ������ - ������

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
        if (rx_buff[10]=='*') cm[i]=0; else cm[i]=1; // ����� ������ ������ ����� �������� � ������� ������ 1-� ������
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
         { if(i==4) st++; else { i++; st-=2; } } // ������ 5 �������
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
    case 0: { put_message_modem (set_gsm); st++; timeout=100; }break; // ����� ��������� SIM �� ���������
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
    case 17: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) st--; }  mess=0;} }break; // ��������� ����� ���������   
    case 18: { end=1; }
    
    }
    if (!mess) Parser();
    if (timeout==0) return STATUS_TIMEOUT;
  }
return STATUS_OK;  
}
//------------------------------------------------------------------------------
// number - ����� �� ������� ���� (0-master, 1..5), text - ����� (�������), type - ��� ���
StatusTypeDef SIM800_SMS_Send(unsigned char number, char *text, char type) 
{
 char end=0;
 char st=0;
 char err_cnt=0; 
 char err_flag=0;
 
// if (sms_razresh==0) return STATUS_OK; // ���� ��������� �������� ���, �� ����� ������

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
     while(mass_number[i]!=0) // ������ ��������� �����
     {
      putchar_modem(0x30); putchar_modem(0x30); // 00
      if (mass_number[i]=='+') { putchar_modem(0x32); putchar_modem(0x42); } //2B      
      else { putchar_modem(0x33); putchar_modem(mass_number[i]); }       
      i++;
     }
     putchar_modem(0x22); putchar_modem(0x0D); // "/r
     st++; timeout=300;
     Delay_ms(del); // �� ���������, � �� ����� ��������� :)
    }break;// ������ ���� ������� ">" 
  case 8:{ if (!strncmp(rx_buff,"> ",2)){ st++;  rx_cnt=0; mess=0; } }break; // ���� >
  case 9:
    { 
      
      char aaa[5]; u16 tmp;  // aaa[5] ������ aaa[4] �.�. ���� ������ ����� ������, ����� ����� � ������� ��������
      while(*text)
      {
       if (*text>=192) tmp=*text+0x350; else tmp=*text; // �� 192 ���������� ������� �������, +0x350 - ��������� � ������
       sprintf(aaa,"%4.4X",tmp);
       put_message_modem_RAM(aaa);
       *text++;
      }
      putchar_modem(0x1A);  // Ctrl-Z
      st++;
      timeout=6000; // 60 s
      mess=0; //?? 
      Delay_ms(del);  // �� ���������, � �� ����� ��������� :)
    }break;
  case 10:
    { //������ ������ +CMGS:
      if (mess) 
       { 
         if (!strncmp(rx_buff,"+CMGS:",6)) st++;
         if (!strncmp_P(rx_buff,error,5) || !strncmp(rx_buff,"+CME ERROR:",11)) {mess=0; err_flag=1; st=12; } // ����� ������ ERROR ��� +CME ERROR: ����� � �������
         mess=0;
       }  
    }break;
  case 11:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break; 
  case 12:{ put_message_modem (set_gsm); st++; timeout=200; }break; // ������ ��������� GSM
  case 13:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break;
  case 14:{ end=1; }break;
  }
  if (!mess) Parser();
  if(timeout==0) { while(1); /*return STATUS_TIMEOUT;*/ } // ����� ��������, ���� ���� �������� �������� - �������������� - � �������� ��� �������
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
  char corr_sms=0; // ���� ��� ��� ����
   
  // ���� ��������� �� +CMTI: � � sms_buff ������ ���� +CMTI: "SM",1 
  s=sms_buff;
  while(*s && *s++!=','); // ���� �������
  sms_index=*s;                                  // ������ ����� ������� - sms_index  (1..9) /������ sms ����� ���� ������ 1..9, �.�. ��� ��� �� ��������� ������� (99% ��� ����� 1, �� �������� ��� :) )
  if (sms_index<0x31 || sms_index>0x39) st=20; // ���� ������ �� 1..9, �� ������� ��� - ��������
  
  
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
          if (!strncmp_P(rx_buff,cmgr,6)) st=10; // ��� ����� �������� ��������� ������ ����������� � �������� �� ����� ������ � �.�.
          else if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; }; 
          mess=0;
        }
      }break;
    case 10:  // ��� � ������ ����� ����� ���
      { 
        if (mess)
        {
          if(rx_buff[0]=='P' || rx_buff[0]=='p') { strcpy(sms_buff,rx_buff); corr_sms=1;  } // � sms_buff �������� ����� ��� ����������� �������
          else  corr_sms=0;  // ��� ���������� �� � P (p) - ������� ��� ����� 
          mess=0;     
          st++;
        }
      }break;
    case 11:{ if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else if (!strncmp_P(rx_buff,error,5)) { st-=2; err_cnt++; }   mess=0;} }break;  
    case 12:{ if (corr_sms) st++; else st=20; }break;
    case 13:{ if(p[0]==sms_buff[1] && p[1]==sms_buff[2] && p[2]==sms_buff[3] && p[3]==sms_buff[4]) st++; else st=20; }break; // ��������� ������, ���� �� �������� - ������� ���
    case 14:{ Execute_SMS_Command(sms_buff); st=20; }break; // ��������� ������� � ������� ���
    
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
  char tmp_answ[40]; // ��� ���������� ������ � ����������� ���
  
  memmove(text,text+5,strlen(text));  // �������� ������ �� 5 �������� ����� - � ������ �� ������ ���������, ��������� ������ ������ ���������� strlen(text) �������� 
  
  sprintf_P(answer,err_cmd); // "������ � �������" - ���� �� ����� ���������� �� �������� - ���������� ��� ���
  
  if (!strncmp_P(text,cmd_11,3) || !strncmp_P(text,RELE1_ON,8) || !strncmp_P(text,rele1_on,8)) 
  { 
    rele1=1;  relay_status[1]=1; sprintf_P(answer,rele_on,1); //  ���� 1 ��������
    if (pa[1]==0x31) { if(relay_zapret[0]!=1) relay_zapret[0]=1; strcat_P(answer,t_reg_off); } // ���� ����� �� �������� � SIM - ������� ��������������
  }     
  else if (!strncmp_P(text,cmd_10,3) || !strncmp_P(text,RELE1_OFF,9) || !strncmp_P(text,rele1_off,9)) 
  { 
    rele1=0;  relay_status[1]=0; sprintf_P(answer,rele_off,1); //  ���� 1 ���������
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
    if (pa[1]==0x31) { if(relay_zapret[0]==1) relay_zapret[0]=0; sprintf_P(answer,t_reg_N_on,1); } // ���� ����� �� �������� � SIM - ������� ��������������
    else { sprintf_P(answer,dat4ik_off,1); }                                                       // ��� ������ ��� ������ ��������
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
    char temp[4]={0,0,0,0};  // 4 �.�. ������ ����������� \0
    char tmp_ust=0, err_ust=0;
    char k=0;
    if (text[0]=='#') k=2; else k=4; // #1123 � SET1123 - ������� ���������� ��� � 3 ��� � 5 �������
    for(int i = 0; i < 3; ++i)
    {
      if (text[i+k]>=0x30 && text[i+k]<=0x39) temp[i]=text[i+k]; // ��������� ��� � ������� ������ �����
      else err_ust=1;
    }
    if (!err_ust && (text[k-1]>=0x30 && text[k-1]<=0x34)) // ���� ������� ���������� � ������ ������ ������ (k-1) = 0, 1...4 
    {
      tmp_ust=atoi(temp); // ���������� ������� � �����
      if (tmp_ust>120) tmp_ust=120; // ����������� ������� 120 ����
      if (tmp_ust<1) tmp_ust=1; 
      switch (text[k-1]) // ����� ������ ����� �������� �� ����� (k-1)
      {
      case 0x31:{ust_m1=tmp_ust; ust[0]=tmp_ust; sprintf_P(answer,ustavka,1,ust[0]); }break; // ���1
      case 0x32:{ust_m2=tmp_ust; ust[1]=tmp_ust; sprintf_P(answer,ustavka,2,ust[1]); }break; // ���2
      case 0x33:{ust_m3=tmp_ust; ust[2]=tmp_ust; sprintf_P(answer,ustavka,3,ust[2]); }break; // ���3
      case 0x34:{ust_m4=tmp_ust; ust[3]=tmp_ust; sprintf_P(answer,ustavka,4,ust[3]); }break; // ���4
      case 0x30:
        {
          if (tmp_ust>30) tmp_ust=30;
          else if (tmp_ust<5) tmp_ust=5;
          range_m=tmp_ust; range=tmp_ust;
          sprintf_P(answer,level,range); //  ������� .. ��������
        }break; // ������� 
      }
    }//err_ust
  }//#
  //---------------------------------------
  else if (text[0]=='h' || text[0]=='H' || !strncmp_P(text,HYS,3) || !strncmp_P(text,hys,3)) // ����������  h05   H30
  {
    char temp[3]={0,0,0};  // 3 �.�. ������ ����������� \0
    char tmp_h=0, err_h=0; 
    char k=0;
    if (text[1]=='y'|| text[1]=='Y') k=3; else k=1; // H12 � HYS12 - ������� ���������� ��� � 2 ��� � 4 �������
    for(int i = 0; i < 2; ++i)
    {
      if (text[i+k]>=0x30 && text[i+k]<=0x39) temp[i]=text[i+k]; // ��������� ��� � ������� ������ �����
      else err_h=1;
    }
    if (!err_h) // ����  ����������
    {
      tmp_h=atoi(temp); // ����������  � �����
      if (tmp_h>30)  tmp_h=30;  // ����������� 30
      if (tmp_h<2)  tmp_h=2; 
      hyst=tmp_h; hyst_m=hyst; sprintf_P(answer,hysterezis,hyst); 
    }   
  }
  //--------------------------------------
  else if(!strncmp_P(text,master,6) || !strncmp_P(text,master_sm,6)) 
  {
    memmove(text,text+6,strlen(text)); 
    if (SIM800_Write_to_SIM(text,0,"MASTER")==STATUS_OK) sprintf_P(answer,mas_ok); // ���� ��� ����
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
      pa[1] = text[1] ;  // ��������1
      pa[2] = text[2] ;  // ��������2
      pa[3] = text[3];   // ��������3
      pa[4] = text[4];   // ��������4
      pa[5] = text[5];   // nc
      pa[6] = text[6];   // nc  
      pa[9] = text[9];   // ���� 0x30 - �� ���������� ������, 0x31 - �� ���������� � �������, ��� ��������� - ������ ����������
      pa[10] = text[10];  // ���� 0x31, �� ������� �������� ����� �������, ����� ������ - ������   
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
      sms_razresh=rx_buff[15]-0x30; // ���������� ������� SMS
    }
  }
  
  else if(!strncmp_P(text,rst,5)) 
  { 
    ust_m1=25; ust_m2=25; ust_m3=25; ust_m4=25;                                   // setup �� ������� �.�. ����� ��������������� �� ����
    relay_zapret[0]=0; relay_zapret[1]=0; relay_zapret[2]=0; relay_zapret[3]=0;
    hyst_m=2; 
    range_m=10;  
    ust[0]=ust_m1; ust[1]=ust_m2; ust[2]=ust_m3; ust[3]=ust_m4; hyst=hyst_m; range=range_m; // ������� ��������� � EEPROM
    
    if(Write_SIM_default()==STATUS_OK) 
    { 
      SSB_Config(); // ����� ����� ������ ���� ���������� ���
      sprintf_P(answer,default_settings);  
    }
  } // ������� ���
  
  else if (!strncmp_P(text,restart,7)) while(1);  // ���������� �������, ����� ������, � ��� ���������. �������� ��� ��� �� �����, � ������ ��������������
  
  // ����� �� ������� �������� ������, ���������� �� ���������
  SIM800_SMS_Send(0,answer,sms_mode);// ������� ����� ������� (���� ������ �� ��������, �� ������ � �������� ��� �� �� �� - �������������� �� ������ (�������� ������)) 
  
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
StatusTypeDef SIM800_Write_to_SIM(char *text, char cell, char *name) 
{
  char end=0, st=0, corr_cell=0;
  char err_cnt=0;
  char n[max_numb_len]; // ������������ � ������ �����
  char cnt=0;
  char write_cell[60]; // AT+CPBW=5,"",129,"NUMBER2" - (26 �������� + max_numb_len) 60 ������
  // ������ ������ �� ������� - ��������� ������������ ��������� ��� � ����������� �� ����, ����� ������ �����
  switch (cell)
  {
  case 0: // ��������� Master
    {
     while(*text)
     {
      if ((*text>=0x30 || *text<=0x39 || *text=='+') && *text!=0) { n[cnt++]=*text; corr_cell=1; } else corr_cell=0;
      *text++;
     }
     if (cnt<=(max_numb_len-1)) n[cnt]=0; else n[max_numb_len-1]=0; // �������� ������ (� ������� �� ������������) 
     sprintf_P(write_cell,write_number,1,n,name);
    }break; 
  case 1: // ��������� Number1..5
    {
      if (text[0]=='*') // ���� 1-� * - �� ����� ������ �
      {
       n[0]='*'; n[1]=0x00; corr_cell=1;
      }
      else // ��� ����� ���������� �����, ������������ � 8 ��� +
      {
       while(*text)
       {
        if ((*text>=0x30 || *text<=0x39 || *text=='+') && *text!=0) { n[cnt++]=*text; corr_cell=1; } else corr_cell=0;
        *text++;
       }
       if (cnt<=(max_numb_len-1)) n[cnt]=0; else n[max_numb_len-1]=0; // �������� ������ (� ������� �� ������������)
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
  case 2: // ��������� Config
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
               (n[4]!='0' && n[4]!='1') || (n[5]!='0' && n[5]!='1') || (n[6]!='0' && n[6]!='1') ) corr_cell=0; // // ��������� ������������ ������ CONFIG
    }break;
  case 3: // ��������� Parol
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
            (n[7]<0x30 && n[7]>0x39) || (n[8]<0x30 && n[8]>0x39) || (n[9]<0x30 && n[9]>0x39) || (n[10]<0x30 && n[10]>0x39) ) corr_cell=0; // ��������� ������������ ������ PAROL
                                                                                                                 
    }break;
  }//switch
  
if (corr_cell)
{
  while (!end)
  {
    switch(st) //AT+CSCS="GSM" ��� ������� ��� ������ ���
    {
    case 0: { put_message_modem_RAM(write_cell); st++; timeout=100;}break;
    case 1: { if (mess) { if (!strncmp_P(rx_buff,ok,2)) st++; else { if (!strncmp_P(rx_buff,error,5)) { st--; err_cnt++; } }  mess=0;} }break;
    case 2: { if (cell==0) sprintf(mass_master_number,n); end=1; }break; // ���� ������ ������ ����� - �� ��������� ��� � ������ � ������
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
  char attempt=3; // ���-�� ������� ����� ������
 
  
  overflow=15*100; // 15 ���
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
         if ((ring_master && pa[9]==0x31) || pa[9]==0x30) { gudok_ok(); st=6; }  // ���� ������ � ������-������ � �� ���� ������ ��� ������ ������ ��������
         else  { gudok(); st++; } 
      }break;
    case 3:
      {
        if(attempt!=0) {dtmf_cnt=0; st++; } else st=10; // ���� �� ��������� ������� �����, �� ���� ������, ����� ������� ������ � ������
      }break;
    case 4:
      { 
        if(dtmf_cnt==4) st++; // overflow ����������� ��� ������ ������� ������ (������� +DTMF) � �������
        if (mess) { if (!strncmp_P(rx_buff,no_carrier,strlen_P(no_carrier))) end=1; mess=0;}
      }break;// ���� 4-� �������� dtmf
    case 5:
      { 
        if (!strncmp(dtmf_buff,p,4)) { st++; Delay_ms(500); password(); Delay_ms(200); prinyat(); } // ������ ������
        else 
         { 
           st=3; attempt--;
           Delay_ms(300); say_error(); // ������� ������
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
//------------ ��������� ���� ������ -------------------------------------------
void Check_DTMF_Cmd(void)
{
  char st=0, end=0;
  char max_count=3; // ���� ������ ��� � max_count=0, �� ����� ��������� ������������ ��-�� �������  if(dtmf_cnt==max_count) st++; 
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
          else dtmf_cnt=0; // ���� �� ������� - �� ���������� dtmf ����� �� ������
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
  // ������ ������������� � ������ ���� 1..4 ��������� �������������� 
  if (!strncmp_P(dtmf_buff,cmd_11,3)) 
  { 
    rele1=1;  Delay_ms(300);say_relay(); say_one(); say_relay_on();relay_status[1]=1; 
    if (pa[1]==0x31) { termoregulator(); Delay_ms(300); say_off(); if(relay_zapret[0]!=1) relay_zapret[0]=1; } // ���� ����� �� �������� � SIM - ������� ��������������
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
    
  else if (!strncmp_P(dtmf_buff,cmd_811,4)) { if (pa[1]==0x31) { relay_zapret[0]=0; termoregulator(); say_one(); Delay_ms(300); say_on();} else {datchik();say_one();say_off();} } // �������� �������������� ��� ������� ��� � ��� ������ ��������
  else if (!strncmp_P(dtmf_buff,cmd_810,4)) { if (pa[1]==0x31) { relay_zapret[0]=1; termoregulator(); say_one(); Delay_ms(300); say_off();} else {datchik();say_one();say_off();}  }
  else if (!strncmp_P(dtmf_buff,cmd_821,4)) { if (pa[2]==0x31) { relay_zapret[1]=0; termoregulator(); say_two(); Delay_ms(300); say_on();} else {datchik();say_two();say_off();} }
  else if (!strncmp_P(dtmf_buff,cmd_820,4)) { if (pa[2]==0x31) { relay_zapret[1]=1; termoregulator(); say_two(); Delay_ms(300); say_off();} else {datchik();say_two();say_off();} }
  else if (!strncmp_P(dtmf_buff,cmd_831,4)) { if (pa[3]==0x31) { relay_zapret[2]=0; termoregulator(); say_three(); Delay_ms(300); say_on();} else {datchik();say_three();say_off();} } 
  else if (!strncmp_P(dtmf_buff,cmd_830,4)) { if (pa[3]==0x31) { relay_zapret[2]=1; termoregulator(); say_three(); Delay_ms(300); say_off();} else {datchik();say_three();say_off();}  } 
  else if (!strncmp_P(dtmf_buff,cmd_841,4)) { if (pa[4]==0x31) { relay_zapret[3]=0; termoregulator(); say_four(); Delay_ms(300); say_on();} else {datchik();say_four();say_off();}  } 
  else if (!strncmp_P(dtmf_buff,cmd_840,4)) { if (pa[4]==0x31) { relay_zapret[3]=1; termoregulator(); say_four(); Delay_ms(300); say_off();} else {datchik();say_four();say_off();} } 
  
  else if (!strncmp_P(dtmf_buff,cmd_00,3))  // ������������� ������
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
   
    Delay_ms(400); gisterezis(); Delay_ms(400); say_numeric(hyst); Delay_ms(300); say_gradus(hyst); // ������ ����������
    Delay_ms(400); uroven(); Delay_ms(400); say_numeric(range); Delay_ms(300); say_gradus(range); // ������ �������
    
    if (!zb) {power();norm();} else {power();power_off();}
    if (baterry_bit==0) { Delay_ms(200); baterry_low();}    
  }// *00
  else if (!strncmp_P(dtmf_buff,cmd_01,3)) say_temp_info(1);
  else if (!strncmp_P(dtmf_buff,cmd_02,3)) say_temp_info(2);
  else if (!strncmp_P(dtmf_buff,cmd_03,3)) say_temp_info(3);
  else if (!strncmp_P(dtmf_buff,cmd_04,3)) say_temp_info(4);
  
  else if (dtmf_buff[0]=='#') // ���� 1-� ������ #, �� ������� �������
  {
    char temp[3]; // int tmp_ust=0;
    char tmp_ust;
    temp[0]=dtmf_buff[2]; temp[1]=dtmf_buff[3]; temp[2]=dtmf_buff[4];
    tmp_ust=atoi(temp); // ���������� ������� � �����
    if (tmp_ust>120) tmp_ust=120; if (tmp_ust<1) tmp_ust=1;
    switch (dtmf_buff[1])
    {
     case 0x31:{ust_m1=tmp_ust; ust[0]=tmp_ust; ustavka_(); say_one(); Delay_ms(200); say_numeric(ust[0]); Delay_ms(300); say_gradus(ust[0]); }break; // ���1
     case 0x32:{ust_m2=tmp_ust; ust[1]=tmp_ust; ustavka_(); say_two(); Delay_ms(200); say_numeric(ust[1]); Delay_ms(300); say_gradus(ust[1]); }break; // ���2
     case 0x33:{ust_m3=tmp_ust; ust[2]=tmp_ust; ustavka_(); say_three(); Delay_ms(200); say_numeric(ust[2]); Delay_ms(300); say_gradus(ust[2]); }break; // ���3
     case 0x34:{ust_m4=tmp_ust; ust[3]=tmp_ust; ustavka_(); say_four(); Delay_ms(200); say_numeric(ust[3]); Delay_ms(300); say_gradus(ust[3]); }break; // ���4
     case 0x30:
      {
       if (tmp_ust>30) tmp_ust=30;
       else if (tmp_ust<5) tmp_ust=5;
       range_m=tmp_ust; range=tmp_ust;
       uroven(); Delay_ms(400); say_numeric(range); Delay_ms(300); say_gradus(range); // ������� - ������� .. ��������
      }break; // �������
    }      
  }

  overflow=15*100;
}

//------------------------------------------------------------------------------
void say_temp_info(char ch)
{
 if (pa[ch]==0x31) // ���� ����� � SIM �������
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
 else // ����� � SIM ��������
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
  
  if(!vv[ch-1]) // ���� ������ � �����
  {
    if (temp[ch-1]==1000) // �������� �� � ������ �� ������
    { 
      vv[ch-1]=1; 
      if (ch==1)  rele1=0;
       else if(ch==2) rele2=0;
        else if(ch==3) rele3=0;
         else if(ch==4) rele4=0;
      relay_status[ch]=0; sprintf_P(answer,avar_dt,ch); 
      if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode); // ���� ��������� - ������ ���
    }
    else // ������ �� � ������
    {
      
      t_on  = ust[ch-1]-(hyst-hyst/2);  // ����������� ���������
      t_off = ust[ch-1]+(hyst/2); // ����������� ����������


      if (relay_zapret[ch-1]==0) // ���� �������������� ������� ( *8X1 )
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

      if ((temp[ch-1]<=(ust[ch-1]-range) || temp[ch-1]>=(ust[ch-1]+range)) && sms_stat[ch-1]==0) // � ��� ��������
      {
          sprintf_P(answer,t_out,temp[ch-1],ch);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // ���� ��������� - ������ ���
          sms_stat[ch-1]=1;
      } 
      if (temp[ch-1]>=t_on && temp[ch-1]<=t_off && sms_stat[ch-1]==1) // � � �����
      {
          sprintf_P(answer,t_norm,temp[ch-1],ch);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // ���� ��������� - ������ ���
          sms_stat[ch-1]=0; 
      }  

    }
    
  }
   
  
}
//-==================================������������ ����������� ������� �������------------------------  
void Power_Monitor(void)
{
  int u=0;
    // 180 V -330, 190 - 360, 200 - 380, 210 - 400, 220 - 430 (��� ����������� ����)
    // ��� ��� ���� ���� ������� ���������: 220 -> 360
    // �� ������ ��� ���� ��� ���� - 12� -> 270, ���� ���� 12 V -> 270
    u=0;
    for(int i = 0; i < 5; ++i)
    {
      u+=read_adc(0);
      Delay_ms(5);
    }
    u/=5;

    if (zb==0) // ���� ������� � ����� - ����� �������� ��������
    {  
      if (u<300)
      {
        Delay_ms(300);  // �������� ������ ����������
        u=read_adc(0);
        if (u<300)
        {
          sprintf_P(answer,p_off);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // ���� ��������� - ������ ���
          zb=1;
        }
      }
    }
    else  // ���� ������� �� ����� - ����� �������� ����� ��� ����������
    { 
      if (u>345)
      {
        sprintf_P(answer,p_on);
        if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // ���� ��������� - ������ ���
        zb=0;
        baterry_bit = 1; // ��� ��������� ������� ������ ��� ������������ �����
      }
      if (u<260)
      {
        if (baterry_bit)
        {
          baterry_bit = 0;
          sprintf_P(answer,batt_low);
          if (sms_razresh!=0) SIM800_SMS_Send(0,answer,sms_mode);  // ���� ��������� - ������ ���
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
  //--------------------================================�������� ����======================================---------------------
  while(1)
  {
    
    __watchdog_reset();
    
    if (!mess) Parser();
    
    if (mess) //  ������ ��� �� � ������???
    {     
      if (!strncmp_P(rx_buff,ring,strlen_P(ring))) //  ��������� � �� ������ �� RING
      { 
        ring_cnt++;
        if (ring_cnt>1) { ring_cnt=0; Check_Incoming_Call(); } // ������ ����� �� 2-�� ����� (2-�� RING)
      }  
      mess=0;
    }
    
    if (sms_flag)// ���� ������ ���, ������ ��� � ���������� ���� => ������ ������ ���������
    { 
      task=1; 
      if (SIM800_SMS_Read()!=STATUS_OK) while(1); 
      sms_flag=0; task=0; 
    }
    
    
    if (power_mon_bit) Power_Monitor(); // ���� ��������� ���������� ��������...
    
    // ���� ����� ������������ - ������� ���������� � �������� � ���    
    if (pa[1]==0x31) Termoregulator(1);
    if (pa[2]==0x31) Termoregulator(2);
    if (pa[3]==0x31) Termoregulator(3);
    if (pa[4]==0x31) Termoregulator(4);
    
    //----============================================================================----------------------------  
    
  }//while(1)
  
}//main
