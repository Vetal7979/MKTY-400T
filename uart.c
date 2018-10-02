//#include "compiler.h"


char const __flash call_ready[] =       "Call Ready";
char const __flash sms_ready[] =        "SMS Ready";
char const __flash at[] =               "AT\r";
char const __flash echo_off[] =         "ATE0\r";
char const __flash echo_on[] =          "ATE1\r";

char const __flash ring[]=              "RING";
char const __flash no_carrier[]=        "NO CARRIER";
char const __flash no_answer[]=         "NO ANSWER";
char const __flash busy[] =             "BUSY";
char const __flash cops_p[] =           "+COPS:";
char const __flash cops_ans[] =         "+COPS: 0,0,\"";
char const __flash cops_no_ans[] =      "+COPS: 0";
char const __flash power_dn[] =         "NORMAL POWER DOWN";
char const __flash parol_ch[] =         "+CPBR: 2,\"*";
char const __flash cbpr[] =             "+CPBR:";
char const __flash collp[] =            "+COLP";
char const __flash dtmf[] =             "+DTMF:";
char const __flash cpms[] =             "+CPMS:";
char const __flash cmgr[] =             "+CMGR:";
char const __flash cmti[] =             "+CMTI:";
char const __flash at_p[] =             "AT+";

char const __flash cpin[] =             "+CPIN:";
char const __flash cpin_ready[] =       "+CPIN: READY";
char const __flash cpin_not_ready[] =   "+CPIN: NOT READY";


//char const __flash dtmf_en[]=           "AT+DDET=1,0,0\r"; // для SIM800
char const __flash dtmf_en[]=           "AT+DDET=1\r"; // для SIM900
char const __flash ipr_115200[] =       "AT+IPR=115200\r";
char const __flash cops[]         =     "AT+COPS?\r";
char const __flash set_gsm[]=           "AT+CSCS=\"GSM\"\r";
char const __flash _atl3[]=             "ATL3\r";                               //выставили громкость
char const __flash atm1[]=              "ATM1\r";                               //включили динамик
char const __flash _atclvl[]=           "AT+CLVL=80\r";                         //команда установки громкости на 80 процентов
char const __flash _atcpbs[]=           "AT+CPBS=\"SM\"\r" ;
char const __flash _sm[]=               "\r\n";
char const __flash ata[]=               "ATA\r\n";
char const __flash hold[] =             "ATH\r";
char const __flash ok[] =               "OK";
char const __flash error[] =            "ERROR";
char const __flash mic_lvl[] =          "AT+CMIC=0,7\r";

char const __flash aaa[] ="fff";

char const __flash dial_master[] =      "ATD>1;\r";
char const __flash dial_number1[] =     "ATD>4;\r";
char const __flash dial_number2[] =     "ATD>5;\r";
char const __flash dial_number3[] =     "ATD>6;\r";
char const __flash dial_number4[] =     "ATD>7;\r";
char const __flash dial_number5[] =     "ATD>8;\r";


char const __flash colp[]        =      "AT+COLP=1\r";                          // при снятии трубки модем будет выдавать +COLP: ....  и OK
char const __flash set_master[] =       "AT+CPBW=1,\"89000000000\",129,\"MASTER\"\r";
char const __flash set_parol[]  =       "AT+CPBW=2,\"*1*1*1*1234\",129,\"PAROL\"\r";
char const __flash set_config[] =       "AT+CPBW=3,\"*000011****\",129,\"CONFIG\"\r";
char const __flash set_numberx[] =      "AT+CPBW=9,\"*******0001\",129,\"SERIAL\"\r";

char const __flash set_number1[] =      "AT+CPBW=4,\"*\",129,\"NUMBER1\"\r";
char const __flash set_number2[] =      "AT+CPBW=5,\"*\",129,\"NUMBER2\"\r";
char const __flash set_number3[] =      "AT+CPBW=6,\"*\",129,\"NUMBER3\"\r";
char const __flash set_number4[] =      "AT+CPBW=7,\"*\",129,\"NUMBER4\"\r";
char const __flash set_number5[] =      "AT+CPBW=8,\"*\",129,\"NUMBER5\"\r";

char const __flash write_number[] =     "AT+CPBW=%d,\"%s\",129,\"%s\"\r";       // AT+CPBW=%d,"%s",129,"%s"\r

char const __flash check_parol[] =      "AT+CPBR=2\r";
char const __flash check_config[] =     "AT+CPBR=3\r";
char const __flash check_xnumber[] =    "AT+CPBR=9\r";
char const __flash clear_numberx[] =    "AT+CPBW=9,\"89000000000\",129,\"FREE\"\r";
char const __flash check_master_num[] = "AT+CPBR=1\r";
char const __flash check_number1[] =    "AT+CPBR=4\r";
char const __flash check_number2[] =    "AT+CPBR=5\r";
char const __flash check_number3[] =    "AT+CPBR=6\r";
char const __flash check_number4[] =    "AT+CPBR=7\r";
char const __flash check_number5[] =    "AT+CPBR=8\r";

/*SMS*/
char const __flash sms_list_all[]=      "AT+CMGL=\"ALL\"\r";
char const __flash sms_read[]=          "AT+CMGR=";
char const __flash sms_del_all[]=       "AT+CMGD=1,4\r";
char const __flash sms_storage_sm[]=    "AT+CPMS=\"SM\"\r";

char const __flash sms_mess_ind[] =     "AT+CNMI=2,1\r";

char const __flash sms_format_text[] =  "AT+CMGF=1\r";
char const __flash sms_unicod[] =       "AT+CSCS=\"UCS2\"\r";
char const __flash sms_parameters_soxr[] = "AT+CSMP=17,167,0,25\r"; // 25 - сохраняем 24 - всплывающая

char const __flash sms_parameters[] =   "AT+CSMP=17,167,0,24\r";
char const __flash sms_number[] =       "AT+CMGS=\"";
char const __flash sms_end[] =          "\"";

//------------------------------------------
/* DTMF/SMS Command*/
char const __flash cmd_11[] = "*11";
char const __flash cmd_10[] = "*10";
char const __flash cmd_21[] = "*21";
char const __flash cmd_20[] = "*20";
char const __flash cmd_31[] = "*31";
char const __flash cmd_30[] = "*30";
char const __flash cmd_41[] = "*41";
char const __flash cmd_40[] = "*40";
char const __flash cmd_51[] = "*51";
char const __flash cmd_50[] = "*50";
char const __flash cmd_61[] = "*61";
char const __flash cmd_60[] = "*60";
char const __flash cmd_71[] = "*71";
char const __flash cmd_70[] = "*70";
char const __flash cmd_81[] = "*81";
char const __flash cmd_80[] = "*80";
char const __flash cmd_91[] = "*91";
char const __flash cmd_90[] = "*90";

char const __flash cmd_62[] = "*62";
char const __flash cmd_63[] = "*63";
char const __flash cmd_64[] = "*64";
char const __flash cmd_65[] = "*65";
char const __flash cmd_66[] = "*66";
char const __flash cmd_67[] = "*67";

char const __flash cmd_00[] = "*00";

char const __flash _30[] = " 30";
char const __flash _05[] = " 05";
char const __flash _120[] = "120";

char const __flash master[] = "MASTER";    char const __flash master_sm[] = "master";
char const __flash number1[] = "NUMBER1";  char const __flash number1_sm[] = "number1";
char const __flash number2[] = "NUMBER2";  char const __flash number2_sm[] = "number2";
char const __flash number3[] = "NUMBER3";  char const __flash number3_sm[] = "number3";
char const __flash number4[] = "NUMBER4";  char const __flash number4_sm[] = "number4";
char const __flash number5[] = "NUMBER5";  char const __flash number5_sm[] = "number5";
char const __flash config[] = "CONFIG";    char const __flash config_sm[] = "config";
char const __flash parol[] = "PAROL";      char const __flash parol_sm[] = "parol";

char const __flash rst[] = "RESET";

/* SMS */
char const __flash rele_on[] = "04200435043b04350020003%d00200432043a043b044e04470435043d043e"; // Реле %d включено
char const __flash rele_off[] = "04200435043b04350020003%d00200432044b043a043b044e04470435043d043e"; // Реле %d выключено
char const __flash ctrl_on[] = "041a043e043d04420440043e043b044c0020d00200432043a043b044e04470435043d"; // Контроль включен
char const __flash ctrl_off[] = "041a043e043d04420440043e043b044c0020d00200432044b043a043b044e04470435043d"; // Контроль выключен
char const __flash err_cmd[] = "041e044804380431043a0430002004320020043a043e043c0430043d04340435"; //Ошибка в команде
char const __flash mas_ok[] = "042f044704350439043a04300020004d00410053005400450052002004430441043f04350448043d043e002004380437043c0435043d0435043d04300020";
char const __flash n_ok[] = "042f044704350439043a04300020004e0055004d004200450052003%d002004430441043f04350448043d043e002004380437043c0435043d0435043d04300020"; //Ячейка NUMBER%d успешно изменена 
char const __flash cfg_ok[] = "042f044704350439043a043000200043004f004e004600490047002004430441043f04350448043d043e002004380437043c0435043d0435043d04300020";// Ячейка CONFIG успешно изменена 
char const __flash pass_ok[] = "042f044704350439043a04300020005000410052004f004c002004430441043f04350448043d043e002004380437043c0435043d0435043d04300020"; // Ячейка PAROL успешно изменена 
char const __flash default_settings[] = "041d0430044104420440043e0439043a04380020044104310440043e04480435043d044b"; //Настройки сброшены
char const __flash ustavka[] = "04230441044204300432043a04300020d003a0020003%d003%d003%d002000b00421"; // Уставка %d%d%d °С

char const  __flash set_date_modem[] = "AT+CCLK=\"10/04/05,08:55:00+03\"\r";
char const __flash get_date_time_modem[] = "AT+CCLK?\r";

/*
char const  __flash blog1[] = "Постановка на охрану "; 
char const  __flash blog2[] = "Снятие с охраны ";
char const  __flash blog3[] = "Включение терминала ";
char const  __flash blog4[] = "Пропадание питания ";
char const  __flash blog5[] = "Появление питания ";
char const  __flash blog6[] = "Сработал канал 1 ";
char const  __flash blog7[] = "Сработал канал 2 ";
char const  __flash blog8[] = "Сработал канал 3 ";
char const  __flash blog9[] = "Сработал канал 4 ";
char const __flash blog_new[] = "Очистка памяти дневника ";
*/
/*
#define LINE_LENGTH 80          // Change if you need 
#define enter 0x0a
#define ps    0x0d
*/

