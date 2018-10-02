
//----------------------Подпрограммы работы со звуком---------------------------------
void spi(u8 Data)
{
  SPDR= Data;
  while(!(SPSR & (1<<SPIF))); // wait for transmission  
}

void spi_write16(unsigned int dat)
{
  spi(dat>>8);
  spi(dat);
}

//#pragma inline = forced
u8 SpiRx (void)
{
  SPDR = 0xFF; // Dummy byte
  while(!(SPSR & (1<<SPIF))); // wait for transmission  
  return SPDR;
}





void voice_to_speaker(unsigned long ReadAddr,unsigned long NumByteToRead)
{
   unsigned char c;
  /* Select the FLASH: Chip Select low */
  memory_select=0; // SPI_FLASH2_CS_LOW();

  /* Send "Read from Memory " instruction */
  spi(0x03);

  /* Send ReadAddr high nibble address byte to read from */
   spi((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
   spi((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte to read from */
   spi(ReadAddr & 0xFF);

  while (NumByteToRead--) /* while there is data to be read */
  {
    __watchdog_reset();    
    c=SpiRx();
    OCR1AL=c;
    delay_us(135);
  }

  /* Deselect the FLASH: Chip Select high */
  memory_select=1; // SPI_FLASH2_CS_HIGH();
}

void spell_all_voice(void)
{
 voice_to_speaker(300000,402733); 
}

void minus(void)
{
  voice_to_speaker(111795,4144); 
  
}

void alarm_on(void)
{
  voice_to_speaker(257481,12016);
}

void channel_(void)
{
  voice_to_speaker(90627,3516);
}

void power(void)
{
  voice_to_speaker(199687,4848);
}

void power_on(void)
{
  //power();
  //delay_ms(500);
  voice_to_speaker(204536,5152);
}

void power_off(void)
{
  //power();
  //delay_ms(500);
  voice_to_speaker(85870,4756);
}

void zamknut(void)
{
  voice_to_speaker(388859,4880);
}
void razomknut(void)
{
  voice_to_speaker(214842,6420);
}
void gradus(void)
{
 voice_to_speaker(70851,4744); 
}

void gradusa(void)
{
 voice_to_speaker(75596,4760); 
}

void gradusov(void)
{
 voice_to_speaker(80357,5512); 
}

void kty_failed(void)
{
 voice_to_speaker(287173,13632); 
}

void ustavka_(void)
{
 voice_to_speaker(323874,4876);  
}
void ahtung(void)
{
  voice_to_speaker(357701,5480);
}

void temperatura(void)
{
  voice_to_speaker(300806,5688);
}

void minimum (void)
{voice_to_speaker(105402,6392);}

void maximum (void)
{voice_to_speaker(98961,6440);}

void say_eight(void)
{voice_to_speaker(367622,4256);}

void say_nine (void)
{ voice_to_speaker(39228,4256);}


void system(void)
{
 {voice_to_speaker(272635,4392);} 
}
void norm (void)
{voice_to_speaker(363182,4440);}

void say_one(void)
{
  voice_to_speaker(147211,3256);
}
void say_two(void)
{
  voice_to_speaker(49822,2792);
}
void say_three(void)
{
  voice_to_speaker(313505,1774);
}
void say_four(void)
{
  voice_to_speaker(19060,3924);
}

void say_five(void)
{
  voice_to_speaker(68282,2568);
}
void say_six(void)
{
  voice_to_speaker(244790,3600);
}
void say_seven(void)
{
  voice_to_speaker(242555,2234);
}



 void say_relay(void)
{
 voice_to_speaker(228800,3496);
}

   void say_relay_on(void)
{
  //say_relay();
  //delay_ms(500);
  voice_to_speaker(352380,5320);
}
  void say_relay_off(void)
{
   //say_relay();
   //delay_ms(500);
   voice_to_speaker(338417,5544);
 
}
void gudok(void)
{
  voice_to_speaker(385018,3840);
}
 
void gudok_ok(void)
{
 voice_to_speaker(155781,16576); 
  
}
void say_10(void)
{
  voice_to_speaker(28874,4512);
}
void say_11(void)
{
  voice_to_speaker(62841,5440);
}
void say_12(void)
{
  voice_to_speaker(57464,5376);
}
void say_13(void)
{
  voice_to_speaker(319169,4704);
}
void say_14(void)
{
  voice_to_speaker(22985,5888);
}
void say_15(void)
{
  voice_to_speaker(209689,5152);
}
void say_16(void)
{
  voice_to_speaker(396717,6016);
}
void say_17(void)
{
  voice_to_speaker(237338,5216);
}
void say_18(void)
{
  voice_to_speaker(378361,6656);
}
void say_19(void)
{
  voice_to_speaker(43485,6336);
}
void say_20(void)
{
  voice_to_speaker(52615,4848);
}
void say_30(void)
{
  voice_to_speaker(315280,3888);
}
void say_40(void)
{
  voice_to_speaker(254648,2832);
}
void say_50(void)
{
  voice_to_speaker(150468,5312);
}
void say_60(void)
{
  voice_to_speaker(248391,6256);
}
void say_70(void)
{
  voice_to_speaker(232297,5040);
}
void say_80(void)
{
  voice_to_speaker(371880,6480);
}
void say_90(void)
{
  voice_to_speaker(33387,5840);
}
void say_100(void)
{
  voice_to_speaker(269498,3136);
}
void control_(void)
{
  voice_to_speaker(94144,4816);
}
void say_on(void)
{
  voice_to_speaker(347771,4608);
}
void say_off(void)
{
  voice_to_speaker(333664,4752);
}
void say_regime(void)
{
  voice_to_speaker(221263,7536);
}
void baterry_low(void)
{
 voice_to_speaker(0,9456);
}
 void say_system_fail(void)
{
 voice_to_speaker(277028,10144);
} 
 
 void say_vokzal(void)
{
 voice_to_speaker(402734,24432);
} 

 void say_haha(void)
{
 voice_to_speaker(427167,15935);
} 

 void say_zero(void)
 {
   voice_to_speaker(393740,2976);
 }
//голос Татьяны Николаевны
 void say_kanal1_tz(void)
{
 voice_to_speaker(443103,12684);
} 
 void say_kanal2_tz(void)
{
 voice_to_speaker(455788,15274);
} 
 void say_kanal3_tz(void)
{
 voice_to_speaker(471063,11583);
} 
 void say_kanal4_tz(void)
{
 voice_to_speaker(482647,15624);
} 
//-----------------------------
 void kanal1_tz(void)
 {
   voice_to_speaker(498272,8724); // Авария в котельной
 }
 void kanal2_tz(void)
 {
   voice_to_speaker(506997,10992); // Газовый клапан открыт
 }
 void kanal3_tz(void)
 {
   voice_to_speaker(517990,8076); // Пожар в котельной
 }
 void kanal4_tz(void)
 {
   voice_to_speaker(526067,12024); // Проникновение в котельную
 }
 void amper(void)
 {
   voice_to_speaker(538092,3896);
 }
 void avaria(void)
 {
   voice_to_speaker(541989,4560);
 }
 void bar(void)
 {
   voice_to_speaker(546550,2628);
 }
 void davlenie(void)
 {
   voice_to_speaker(549179,5776);
 }
 void maximalnoi(void)
 {
   voice_to_speaker(554956,5508);
 }
 void minimalnoi(void)
 {
   voice_to_speaker(560465,5448);
 }
 void nagruzka(void)
 {
   voice_to_speaker(565914,5088);
 }
 void napryajenie(void)
 {
   voice_to_speaker(571003,6632);
 }
 void ne_otve4aet(void)
 {
   voice_to_speaker(577636,7158);
 }
 void neispraven(void)
 {
   voice_to_speaker(584795,6066);
 }
 void neispravno(void)
 {
   voice_to_speaker(590862,6234);
 }
 void nije_ustavki(void)
 {
   voice_to_speaker(597097,8024);
 }
 void nije(void)
 {
   voice_to_speaker(605122,3654);
 }
 void nominalnoi(void)
 {
   voice_to_speaker(608777,6036);
 }
 void nominalnogo(void)
 {
   voice_to_speaker(614814,6162);
 }
 void osveschenie(void)
 {
   voice_to_speaker(620977,6540);
 }
 void bad_signal(void)
 {
   voice_to_speaker(627518,12360);
 }
 void plus(void)
 {
   voice_to_speaker(639879,2148);
 }
 void pokazaniya(void)
 {
   voice_to_speaker(642028,5532);
 }
 void pribora(void)
 {
   voice_to_speaker(647561,4002);
 }
 void prote4ka(void)
 {
   voice_to_speaker(651564,5100);
 }
 void signala(void)
 {
   voice_to_speaker(656665,4824);
 }
 void svyaz_ok(void)
 {
   voice_to_speaker(661490,7824);
 }
 void uroven_signala(void)
 {
   voice_to_speaker(669315,9462);
 }
 void uroven(void)
 {
   voice_to_speaker(678778,4104);
 }
 void uspeshna(void)
 {
   voice_to_speaker(682883,4992);
 }
 void ustavki(void)
 {
   voice_to_speaker(687876,5196);
 }
 void ustroistvo(void)
 {
   voice_to_speaker(693073,5886);
 }
 void vishe_ustavki(void)
 {
   voice_to_speaker(698960,8112);
 }
 void vishe(void)
 {
   voice_to_speaker(707073,3756);
 }
 void vneshnee(void)
 {
   voice_to_speaker(710830,5336);
 }
 void vnutrennee(void)
 {
   voice_to_speaker(716167,6200);
 }
 void volt(void)
 {
   voice_to_speaker(722368,3668);
 }
 void zapis(void)
 {
   voice_to_speaker(726037,4404);
 }
 void zapyataya(void)
 {
   voice_to_speaker(730442,5256);
 }
// след свободный адрес - 735699 // 12.07.2012

 void chas(void)
 {
   voice_to_speaker(735699,3878);
 }
 void chasa(void)
 {
   voice_to_speaker(739578,4262);
 }
 void chasov(void)
 {
   voice_to_speaker(743841,5667);
 } 
 void datchik(void)
 {
   voice_to_speaker(749509,5054);
 } 
 void datchik_dvijenia(void)
 {
   voice_to_speaker(754564,9575);
 } 
 void diapazon(void)
 {
   voice_to_speaker(764139,6461);
 } 
 void dostup(void)
 {
   voice_to_speaker(770601,5136);
 } 
 void dostupen(void)
 {
   voice_to_speaker(775738,5837);
 } 
 void dver(void)
 {
   voice_to_speaker(781576,3376);
 } 
 void emkost(void)
 {
   voice_to_speaker(784953,5745);
 } 
 void end_fuel(void)                    // заканчивается топливо
 {
   voice_to_speaker(790699,11667);
 } 
 void generator(void)
 {
   voice_to_speaker(802367,5779);
 } 
 void gisterezis(void)
 {
   voice_to_speaker(808147,7551);
 }
 void klapan(void)
 {
   voice_to_speaker(815699,4387);
 }
 void kod(void)
 {
   voice_to_speaker(820087,2954);
 }
 void litr(void)
 {
   voice_to_speaker(823042,3573);
 }
 void litra(void)
 {
   voice_to_speaker(826616,4018);
 }
 void litrov(void)
 {
   voice_to_speaker(830635,5423);
 }
 void minut(void)
 {
   voice_to_speaker(836059,4746);
 }
 void minuta(void)
 {
   voice_to_speaker(840806,4927);
 }
 void minuty(void)
 {
   voice_to_speaker(845734,5185);
 }
 void na(void)
 {
   voice_to_speaker(850920,2505);
 }
 void nedostupen(void)
 {
   voice_to_speaker(853426,6460);
 }
 void nevernyi(void)
 {
   voice_to_speaker(859887,6204);
 }
 void nomer(void)
 {
   voice_to_speaker(866092,3937);
 }
 void ob_ekt(void)
 {
   voice_to_speaker(870030,4643);
 }
 void obogrev(void)
 {
   voice_to_speaker(874674,5200);
 }
 void obryv_svyazi(void)
 {
   voice_to_speaker(879875,8068);
 }
 void ojidanie(void)
 {
   voice_to_speaker(887944,5930);
 }
 void ojidanie_vvoda(void)
 {
   voice_to_speaker(893875,9653);
 }
 void okno(void)
 {
   voice_to_speaker(903529,3633);
 }
 void opoveschenie(void)
 {
   voice_to_speaker(907163,7399);
 }
 void say_error(void)                      // ошибка
 {
   voice_to_speaker(914563,4992);
 }
 void otkaz(void)
 {
   voice_to_speaker(919556,4942);
 }
 void otkazano(void)
 {
   voice_to_speaker(924499,5659);
 }
 void otkluchen(void)
 {
   voice_to_speaker(930159,5851);
 }
 void otklucheno(void)
 {
   voice_to_speaker(936011,5766);
 }
 void otkryt(void)
 {
   voice_to_speaker(941778,4609);
 }
 void otkryta(void)
 {
   voice_to_speaker(946388,4885);
 }
 void password(void)
 {
   voice_to_speaker(951274,3928);
 }
 void parolya(void)
 {
   voice_to_speaker(955203,3888);
 }
 void pojar(void)
 {
   voice_to_speaker(959092,4350);
 }
 void pomeschenie(void)
 {
   voice_to_speaker(963443,6519);
 }
 void predel(void)
 {
   voice_to_speaker(969963,4054);
 }
 void prinyat(void)
 {
   voice_to_speaker(974018,3891);
 }
 void procent(void)
 {
   voice_to_speaker(977910,5421);
 }
 void procenta(void)
 {
   voice_to_speaker(983332,5596);
 }
 void procentov(void)
 {
   voice_to_speaker(988929,6459);
 }
 void proniknovenie(void)
 {
   voice_to_speaker(995389,7476);
 }
 void razreshen(void)
 {
   voice_to_speaker(1002865,5990);
 }
 void razresheno(void)
 {
   voice_to_speaker(1008856,5908);
 }
 void secund(void)
 {
   voice_to_speaker(1014764,6225);
 }
 void secunda(void)
 {
   voice_to_speaker(1020990,5990);
 }
 void secundy(void)
 {
   voice_to_speaker(1026981,6129);
 }
 void temperatury(void)
 {
   voice_to_speaker(1033111,6285);
 }
 void termoregulator(void)
 {
   voice_to_speaker(1039397,7819);
 }
 void utechka(void)
 {
   voice_to_speaker(1047217,5151);
 }
 void utechka_gaza(void)
 {
   voice_to_speaker(1052369,8262);
 }
 void v(void)
 {
   voice_to_speaker(1060632,2201);
 }
 void v_dostupe(void)
 {
   voice_to_speaker(1062834,5874);
 }
 void v_dostupe_otkazano(void)
 {
   voice_to_speaker(1068709,10255);
 }
 void vhodnaya(void)
 {
   voice_to_speaker(1078965,5547);
 }
 void vlajnost(void)
 {
   voice_to_speaker(1084513,5932);
 }
 void vvod(void)
 {
   voice_to_speaker(1090446,4416);
 }
 void zakryt(void)
 {
   voice_to_speaker(1094863,4889);
 }
 void zakryta(void)
 {
   voice_to_speaker(1099753,5364);
 }
 void zapolnena(void)
 {
   voice_to_speaker(1105118,6532);
 }
 void zapreschen(void)
 {
   voice_to_speaker(1111651,6537);
 }
 void zaprescheno(void)
 {
   voice_to_speaker(1118189,6207);
 }
 void gaz_kl_close(void)
 {
   voice_to_speaker(1124397,11892);
 }
 void gaz_kl_open(void)
 {
   voice_to_speaker(1136290,11778);
 }
 void change_polarity(void)
 {
   voice_to_speaker(1148069,40701);
 }

// следующий свободный адрес - 1188771