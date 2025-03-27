/*****************************************************
Project : 1602 I2C 4 bit mode PCF8574 driver
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




#define pcf_adr 0x4e


void lcd_view_temp(int temp, char ofset);//показывает температуру с знакоместа ofset, если отрицательная выводит минус, занимает 5 знакомест
void lcd_view_day(char ofset); //показывает день недели со смещения ofset занимая два знакоместа
void lcd_view_sec(unsigned int raw, char ofset);////конвертирует raw d минутах в минуты и часы и показывает со смещения ofset, в часах и минутах показывает до 99-00, дальше просто в часах, пять знакомест занимает 
void lcd_show_time(void); //показывает время дату и день недели на весь экран.
void lcd_init(void);//инициализация 1602 в 4-разрядном режиме (PCF8574)
void lcd_write(void);//запись дисплея в 4разрядном режиме, (PCF8574), 4,5 мсек занимает

void send_data(char data);//посылка комманды в дисплей
void send_com(char data);//посылка данных

//копирует сообщения из массива mess в буфер экрана 
//ofset - знакоместо экрана с которого будет писаться 
//in номер сообщения
//в массиве сообщения надо разделять звездочками типа flash char message="*set*time*stop*level*"
void lcd_copy (char ofset, char in);
void lcd_buf_clear(void);//очистка буфера экрана, не самого экрана!
char lcd_conv(char sym);//конвертирование цифр в ascii
void lcd_split(unsigned int di);//разделяет чисо di на десятичные раряды и помещает их в DD[], уже в ascii там будет
void lcd_split_six(unsigned int dis);//аналогично но hex


//отправляет число in в буфер дисплея со знакоместа set
//если старший десятичный разряд 0 то нули незначащие не гасим, если 1 - гасим 2 -в hex виде показываем
//16 -отправляем с 16 знакоместа, 116 - гасим при этом нули, 216 - в hex виде
//set1, старшие 4 двоичных разряда - в каком разряде ставим точку (не более 3 разряда)
//младшие сколько разрядов отображаем (не более 5), поэтому удобнее писать в hex формате
//0x04 - четыре без точки; 0x25 - четыре разрядов и точка во втором
//имеем ввиду, что если включено отображение точки то она занимает разряд
//и если мы хотим четыре разряда с точкой то нужно отбражать пять разрядов   
void lcd_convert(unsigned int in, char set, char set1);

void pca8575_read(void);//чтение портов PCF8575 в p0 и p1

flash char hex_table[] = "0123456789ABCDEF";//массив для конвертации в ascii формат

char lr[32]; //буфер индикатора он пишется в дисплей функцией lcd_write() 
char DD[5];//результат разделение числа на разряды, там уже в ascii цифры!! можно прямо писать в дисплей
unsigned char p0;//сюда читается порт 0 PCF8575 
unsigned char p1;//сюда читается порт 1 PCF8575
unsigned char zum;//для пищания
char nibble;//для разделения байта
char led_light;//включение подсветки
int tt1;

void send_com(char data) //функция отправки команды на LCD (RS=0)
{
    nibble = data & 0xF0; //обнуляем младшую тетраду (D3-D0)
    twi_byte_w(nibble | 0x04 | (led_light << 3)); //выставляем старшую тетраду с поднятым E
    twi_byte_w(nibble | (led_light << 3)); //отправляем старшую c E=0 (запись)
    nibble = data << 4; //сдвигаем младшую тетраду в D7-D4
    twi_byte_w(nibble | 0x04 | (led_light << 3)); //выставляем младшую тетраду с поднятым E
    twi_byte_w(nibble | (led_light << 3)); //отправляем младшую тетраду с c E=0 (запись)
}
void send_data(char data) //функция отправки данных на LCD (RS=1)
{
    nibble = data & 0xF0; //обнуляем младшую тетраду (D3-D0)
    twi_byte_w(nibble | 0x05 | (led_light << 3)); //выставляем старшую тетраду с  поднятым E и RS
    twi_byte_w(nibble | 0x01 | (led_light << 3)); //отправляем старшую тетраду с  RS=1, E=0 (запись)
    nibble = data << 4; //сдвигаем младшую тетраду в D7-D4
    twi_byte_w(nibble | 0x05 | (led_light << 3)); //выставляем младшую тетраду с  поднятым E и RS
    twi_byte_w(nibble | 0x01 | (led_light << 3)); //отправляем млажшую тетраду с  RS=1, E=0 (запись)
}

void lcd_init(void)
{
    twi_start();
    twi_byte_w(pcf_adr);
    
    // Сброс в 8-битный режим (только старшая тетрада) 
    //rs=0 комманда, rs=1 данные, запись по заднему фронту E, RW всегда на земле
    //RS P0  1
    //RW P1  2
    //E  P2  4
    //Bl P3  8
    twi_byte_w(0x3c); twi_byte_w(0x38); // 0011, E=1 > E=0, RS=0,RW=0,BL=0
    twi_byte_w(0x3c); twi_byte_w(0x38);
    twi_byte_w(0x3c); twi_byte_w(0x38);

    // Переход в 4-битный режим (только старшая тетрада)
    twi_byte_w(0x2c); twi_byte_w(0x28); // 0010, E=1 > E=0, RS=0

    send_com(0x28); // 4 бита, 2 строки
    send_com(0x01); // Очистка дисплея
    delay_ms(2);
    send_com(0x0C); // Дисплей вкл, курсор выкл
    twi_stop();
}



void lcd_write(void)
{
    char pos;
    twi_start();
    twi_byte_w(pcf_adr); 
    
    send_com(0x80);// Курсор на первую строку
    
    for (pos=0; pos<16; pos++) // Вывод первых 16 символов
    {send_data(lr[pos]);}
    send_com(0xC0); // Курсор на вторую строку
    
    for (pos=16; pos<32; pos++)// Вывод следующих 16 символов из lr 
    {send_data(lr[pos]);}
    
    twi_stop();
}




char lcd_conv(char sym)
{
    return hex_table[sym];
}
 
void lcd_split(unsigned int di)
  {
      DD[0]=lcd_conv(di/10000);
      DD[1]=lcd_conv((di%10000)/1000);
      DD[2]=lcd_conv((di%1000)/100);
      DD[3]=lcd_conv((di%100)/10);
      DD[4]=lcd_conv(di%10);
  }

void lcd_split_six(unsigned int dis)
  {  char st;
   if(dis<256)
     { 
      DD[3]=lcd_conv(dis/16);
      DD[4]=lcd_conv(dis%16);
     }
   else
     {
      st=dis>>8;
      DD[1]=lcd_conv(st/16);
      DD[2]=lcd_conv(st%16);
      st=dis;
      DD[3]=lcd_conv(st/16);
      DD[4]=lcd_conv(st%16);
     }   
  }    


void lcd_convert(unsigned int in, char set, char set1)
  {
   char razr; char ofset; char i; char dot_set; bit zero; bit six;
     if (set<100){zero=0;ofset=set;}//нули гасим
     if (set>=100&set<200){zero=1;ofset=set-100;}//не гасим
     if (set>=200){six=1;ofset=set-200;}else six=0;//отображаем в шестнадцетиричном
     razr=(set1&0x0f);//выделяем число разрядов
     dot_set=set1>>4; //и положение точки
       
       if (six){lcd_split_six(in);}//разбираем входное число на шестнадцетиричные разряды   
       else {lcd_split(in);}//или на десятичные разряды
     
     if (zero)//если нужно гасить нули
      {
       for (i=0;i<(4-dot_set);i++)//в зависимости от того где стоить точка, оставляем либо три, либо два, если точки нет то один ноль 
        {if (DD[i]=='0')DD[i]=' ';else break;}//и заменяем ноль на пробел до первой цифры 
      }  
     
     if (dot_set!=0)//если нужна точка
      {
       DD[0]=DD[1];//сдвигаем символы до точки влево что бы ее можно было вставить
       if (dot_set==3)DD[1]='.';//если в третьем
       else DD[1]=DD[2];
      }
     
     if (dot_set==1)//если точка в первом разряде
      {
       DD[2]=DD[3];//немношко еще сдвигаем
       DD[3]='.';//и ставим точку
      }
     
     if (dot_set==2)//аналогично тут сдвигате не надо
      {
       DD[2]='.';//просто ставим точку
      }

      for (i=0;i<razr;i++)//пишем результ в буфер, выравниваеся по правому краю, поэтому i+5-razr, просто пиздец но непонятно ))))
        {lr[ofset+i]=DD[i+5-razr];}
 }

void lcd_buf_clear(void)
 {
   char i;
   for (i=0;i<32;i++){lr[i]=' ';}//очистка буфера дисплея
 }

void lcd_view_day(char ofset)
 {
   lcd_copy(ofset,week_day+8);          
 }      

void pca8575_read(void)
 {
   twi_start();
   twi_byte_w(0x41);
   p0=twi_byte_r();
   p1=twi_byte_rn();
   twi_stop();
 }
 
 
void lcd_view_temp(int temp, char ofset)
 {
  if (temp==0)//если ноль значит датчик неисправен, выводим --
   {
    lr[ofset+3]=lr[ofset+4]='-';
   }
  if (temp<0)
   {
     
     if(temp<-99)
       {
        lr[ofset]='-';//рисуем минус
        lcd_convert(temp*-1,ofset+1,0x14);
       }
     else
       {
        lr[ofset+1]='-';//рисуем минус
        lcd_convert(temp*-1,ofset+2,0x13);
       } 
   }
   if (temp>0)
   {
   lcd_convert(temp,ofset+100,0x15);//+100 потому что нули гасим незначащие 
   }
 }

void lcd_view_sec(unsigned int raw, char ofset)
  {
    if(raw<=5940)//если менее 99 часов то показываем часы минуты
      { 
       lcd_convert(raw/60,ofset,0x02);
       lcd_convert(raw%60,ofset+3,0x02);
       if(tic) lr[ofset+2]=' ';
       else lr[ofset+2]=':';
      }
    else //а если более то только часы 
     {
      lcd_convert(raw/60,ofset+100,0x05);
     }  
   }     
    
void lcd_show_time(void)
  { 
    lcd_convert(hour,0,0x02);
    if(tic)lr[2]=lr[5]=':';
    else lr[2]=lr[5]=' '; 
    lcd_convert(min,3,0x02);
    lcd_convert(sec,6,0x02);
    lcd_convert(day,16,0x02);
    lcd_convert(month,19,0x02);
    lcd_convert(year,22,0x02);
    lr[18]=lr[21]='-';
    lcd_view_day(25);
  }
  
void lcd_copy (char ofset, char in)
 {
  char index=0;
  for (tt1=0;tt1<=1024;tt1++)//тут мы вычмсляем откуда начинается нужное сообщени
   {
    if (mess[tt1]=='*')//если мы попали на звездочку
      {
       if (index==in)//и это нужная нам звездочка
       break;//вываливаемя и tt1 будет индекс нужного нам сообщения
       else index++;//если нет то продолжаем считать звездочки
      }
   }
  
  for (i=0; i<255; i++)//пишем сообщение в буфер дисплея
   {
    lr[ofset+i]=mess[tt1+i+1];//+1 т.к. индекс то нам отдали звездочки, а не первой буквы сообщения
      if (mess[tt1+i+2]=='*')//и если следующий символ звездочка 
      break;//то значит мы записали всё сообщение и валим
   }
 }
