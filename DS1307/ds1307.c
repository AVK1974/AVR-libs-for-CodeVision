/*****************************************************
Project : DS1307 driver
Version : 1.0
Date    : 24.03.2025
Author  : Antony Smith
Company : AVK inc
Comments: avkinc@gmail.com 
License : GPLv3

Chip type               : ATmega
Program type            : lib
AVR Core Clock frequency: 1-20 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 256
*****************************************************/
void rtc_read_dt(void);//чтение из DS1307 времени и даты в соответствующие переменные, на аппартном I2C это происходит быстрее чем родная функция чтения только времени, поэтому так
void rtc_set_time(char hour, char min, char sec);//полностью наналогичная родной функции
void rtc_set_date(char week_day,char day, char month, char year);//полностью наналогичная родной функции
char rtc_read(char adr);//полностью наналогичная родной функции
void rtc_write(char adr,char data);//полностью наналогичная родной функции
unsigned int rtc_read_int(char adr);//читает из DS два байта начиная с adr и собирает их в int, старший байт идет первым
void rtc_write_int(char adr,unsigned int data); //пишет в DS int переменную в два байта DS начиная с adr, старший байт первый 


char hour;
char min;
char sec;
char week_day;
char day;
char month;
char year;



void rtc_read_dt(void)
 {
  twi_adr=0xd0;//адрес ds1307
  twi_seq_r(0,7);//читаем в массив первые семь байт ОЗУ DS1307, в них все что нам нужно
  sec=(twi_data[0]>>4)*10+(twi_data[0]&0x0F);//преобразуем BSD код в десятичный
  min=(twi_data[1]>>4)*10+(twi_data[1]&0x0F);//преобразуем BSD код в десятичный
  hour=(twi_data[2]>>4)*10+(twi_data[2]&0x0F);//аналогично
  week_day=twi_data[3];//а день недели не в BSD, сюрприз
  day=(twi_data[4]>>4)*10+(twi_data[4]&0x0F);
  month=(twi_data[5]>>4)*10+(twi_data[5]&0x0F);
  year=(twi_data[6]>>4)*10+(twi_data[6]&0x0F);
 }

void rtc_set_time(char hour, char min, char sec)
 {
  twi_data[0]=((sec/10)<<4)+sec%10;
  twi_data[1]=((min/10)<<4)+min%10;
  twi_data[2]=((hour/10)<<4)+hour%10;
  twi_adr=0xd0;//адрес ds1307
  twi_seq_w(0,3);//пишем три байта в DS1307 с нулевого адреса
 }

void rtc_set_date(char week_day,char day, char month, char year)
 {
  twi_data[0]=week_day;//день недели не в BSD пишется
  twi_data[1]=((day/10)<<4)+day%10;
  twi_data[2]=((month/10)<<4)+month%10;
  twi_data[3]=((year/10)<<4)+year%10;
  twi_adr=0xd0;//адрес ds1307
  twi_seq_w(0x03,4);//пишем начиная с адреса 0x03  четыре байта в DS1307
 }
 
char rtc_read(char adr)
 {
  twi_adr=0xd0;//адрес ds1307
  twi_seq_r(adr,1);
  return twi_data[0]; 
 }

void rtc_write(char adr,char data)
 {
  twi_adr=0xd0;//адрес ds1307
  twi_data[0]=data;
  twi_seq_w(adr,1);
 }
 
unsigned int rtc_read_int(char adr)
 {
  twi_adr=0xd0;//адрес ds1307
  twi_seq_r(adr,2);
  return (twi_data[0]<<8)+twi_data[1];
 }

void rtc_write_int(char adr,unsigned int data)
 {
  twi_adr=0xd0;//адрес ds1307
  twi_data[0]=(data>>8);
  twi_data[1]=data;
  twi_seq_w(adr,2); 
 }  


