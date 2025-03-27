/*****************************************************
Project : max7219 driver
Version : 1.0
Date    : 23.03.2025
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
char conv(char sym); //конвертирование в семисегментный формат
// отправка числа на индикатор
// in -отправляемое число
// set- 1разряд (0-гасим незначащие нули (последний 0 никогда не гасится), 1-не гасим 2 - отображаем в шестнадцатеричном формате c гашением нулей
// 2разряд-сколько разрядов отображаем
// 3разряд-в каком радряде ставим точку (1разряд слева) и на какой стороне мы отбражаем переменную
//0 - отбражае на левой стороне без точек 1 - на левой и точка в первом разряде 2 - во втором и тд
//5 - отображаем на правой стороне без точек 6 - на правой и точка в первом разряде правой стороны, если считать слева то это пятый разряд, ну и так далее
// пример:
//   20-нули гасим, два разряда, без точек, слева
//   136-нули не гасим, три разряда, справа, во втором разряде точка
// если точка стоит в третьем разряде то последние два разряда никогда не гаснут, что бы не было типа .0 или .6 будет 0.0 или 0.6
void convert(unsigned int in, char set);
void show_sec(unsigned int raw, char side); //конвертирует отсчеты раз в минуту в минуты и часы и показывае на стороне side, в часах и минутах показывает до 99-00, дальше просто в часах 
void show_day(char day);//показываем день недели
void max_write(char reg, char data);//запись в max7219, сначала регистр, потом данные
void display_clear(void); //очистка буфера дисплея
void display_write(void);//запись в max7219
void split(unsigned int di);//распидорашивание цифры на разряды в семисегментном формате
void split_six(unsigned int dis);//то же самое но в хекс формат

char r[8]; //разряды индикаторов
char D[4]; //результат распидорашивания цифры, там уже в семисегментном формате
bit shift;//признак числа более 9999
bit tic; //инвертируется раз в секунду, для миганий
flash char hex_table[]={0x7e,0x30,0x6d,0x79,0x33,0x5b,0x5f,0x70,0x7f,0x7b,0x77,0x1F,0x0d,0x3D,0x4F,0x47};//цифры в семисегментном формате от 0 до F
flash char hex_table_day[]={0x00c,0x00,0x1c,0x4e,0x76,0x37,0x1c,0x0f,0x4e,0x67,0x33,0x0f,0x76,0x0f,0x4e,0x1f}; //дни недели в семисегментном формате, первые два нуля тк нулевого дня нет, начиная с воскресенья

void convert(unsigned int in, char set)
  {
    char razr; char ofset; char i; char ii; char dot_set; bit zero; bit six;
     if (set<100)zero=0;
     if (set>=100&set<200)zero=1;
     if (set>=200)six=1;
     else six=0;
       
       if (six)   
          {
           split_six(in);//разбираем входное число на шестнадцетиричные разряды
          }
       else
          { 
           split(in);//разбираем входное число на десятичные разряды
          }
       
       razr=(set%100)/10;     //сколько разрядов отображаем 
       dot_set=set%10; //в каком разряде ставим точку и на какой стороне дисплея отображаем
       if (dot_set<5)ofset=0;//слева
         else ofset=4;//справа   
       if (dot_set>4)dot_set=dot_set-5;//если нам нужно отбразить точку в правой половине дисплея значит мы и переменную там же отбражаем и dot_set больше 5, а отображение точек идет отностельно половины дисплея поэтому так
       
       if(zero==0)//если незначащие нули не показываем
       {
         if(dot_set==3)ii=2;//если точка в третьем разряде (отноистельно числа само собой) то последние два разряда не гасим никогда иначе будет некрасиво типа .0 
         else ii=3;// в последнем разряде ноль не гасим никогда поэтому 3
           
           for (i=0;i<ii;i++) 
            {if (D[i]==0x7e)//если в соответствующем разряде ноль (в семисегментном формате)
               {D[i]=0x00;}//гасим его
             else break;   
            }
       }   
       
       for (i=4;i>4-razr;i--)//пишем необходимое число разрядов в буфер индикатора
          {r[i-1+ofset]=D[i-1];}

       if(shift)r[1+ofset]|=0b10000000;//если число больше 9999 зажигаем точку 
       else 
        { 
         if (dot_set)//зажигаем точку в соответствующем разряде если надо
           { 
            r[0+ofset+dot_set-1]|=0b10000000;
           }
         }

        if(six)
        {
          if(in<256)//два разряда показываем
            {
             r[0+ofset]=0x00;
             r[1+ofset]=0x00;
            }
          if(in<4096)//четыре
            {
             r[0+ofset]=0x00;
            }  
        }
  }
 
 
char conv(char sym) //конвертация цифры в семисегментный формат
 {
   return hex_table[sym];
 } 
 
void max_write(char reg, char data)//на частоте 1мГц выбрасывает за 340 мкс. 
  {  
     char i=16;
     unsigned int mask=0x8000;
     unsigned int load;
      
      load=reg<<8;//склеиваем два байта для передачи
      load|=data;//склеиваем два байта для передачи
    
    CS=0;//разрешаем прием данных
   
   while(i!=0)//выкидываем 16бит 
     { 
       if(load & mask)MOSI=1;//проверяем есть ли в разряде единица, если есть ставим на mosi 1
       else MOSI=0;
       SCK = 1;
       SCK = 0;
       mask >>=1;//сдвигаем маску
       i--;
     }   

      CS=1;//запрещаем прием данных
   } 
  
  
  void display_write(void)  
  {
     max_write(1,r[7]);
     max_write(2,r[6]);
     max_write(3,r[5]);
     max_write(4,r[4]);
     max_write(5,r[3]);
     max_write(6,r[2]);
     max_write(7,r[1]);
     max_write(8,r[0]);
  }
  
  
 void split(unsigned int di)
  {
   if(di<10000)
     { 
      D[0]=conv(di/1000);
      D[1]=conv((di%1000)/100);
      D[2]=conv((di%100)/10);
      D[3]=conv(di%10);
      shift=0;
      }
   else
      {
      D[0]=conv(di/10000);
      D[1]=conv((di%10000)/1000);
      D[2]=conv((di%1000)/100);
      D[3]=conv((di%100)/10);
      shift=1;
      } 
  }

 void split_six(unsigned int dis)
  {  char st;
   if(dis<256)
     { 
      D[2]=conv(dis/16);
      D[3]=conv(dis%16);
      }
   else
      {
      st=dis>>8;
      D[0]=conv(st/16);
      D[1]=conv(st%16);
      st=dis;
      D[2]=conv(st/16);
      D[3]=conv(st%16);
      }   
  }  
  
void display_clear(void)
  {r[0]=r[1]=r[2]=r[3]=r[4]=r[5]=r[6]=r[7]=0;}
  
void show_sec(unsigned int raw, char side)
  {
    char ofset;
    split(raw/60);
    if(side==1)ofset=4;//если нужно показать справа то смещаем на четыре позиции цифры
    else ofset=0;
      if(raw<=5940)//если менее 99 часов то показываем часы минуты
      { 
       if ((raw/60)<10){r[1+ofset]=D[3];}
         else {r[0+ofset]=D[2];r[1+ofset]=D[3];}
       split(raw%60);
       r[2+ofset]=D[2];r[3+ofset]=D[3];
       if(tic==0) r[1+ofset] SET_B(7);//тут мигаем тирешечкой
       else  r[1+ofset] CLR_B(7);
      }
    else //а если более то только часы 
     {
      if (side==0)convert(raw/60,40);
      else convert(raw/60,45);
     }  
   } 

void show_day(char day)
 {
  r[6]=hex_table_day[day*2];r[7]=hex_table_day[day*2+1];
 }  
