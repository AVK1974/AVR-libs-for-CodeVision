/*****************************************************
Project : DS18B20 
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


   
  
unsigned char ow_reset(void)
  {
   unsigned char state;
   ow_in;//шину на вход
   if(!ow_read)//если линия прижата то значит КЗ на землю
     {
      ow_fault=f_ow_shortlo;//пишем код ошибки
      return 0;//и валим сразу
     }
   ow_out;//линию на выход
   //#asm("cli")//запрещаем прерывания
   ow_lo;//прижимаем к земле
   delay_us(480);//ждем 480
   if(ow_read)//если линия не прижата значит КЗ на питание
     {
      ow_fault=f_ow_shorthi;//пишем код ошибки
      return 0;//и валим сразу
     }
   ow_in;//линию на вход
   delay_us(60);//ждем
   state=ow_read;//читаем линию
   ow_tic=0;
   while(!ow_read)//ждем пока датчик отпустит линию
      {
       ow_tic++;
       if (ow_tic>150)//а если линия не отпускается внезапно, то ждем пока не натикает 16мсек
        {
          ow_fault=f_ow_timeout;//пишем код ошибки
          return 0;//и валим сразу
        }
      }
   
   ow_fault=0;//выходим без ошибок
   if(state) return 0;//если линия не была прижата значит устройств на линии нет но  это не ошибка
   else return 1; //а если прижата то заебок
  }

void ow_write_bit(char bi)
  {
   #asm("cli")//запрещаем прерывания
   ow_out;//линию на выход
   ow_lo;//прижимаем к земле
   if(bi)//шлем единицу, это 10мкс прижимает к земле потом до конца слота (60мкс) отпускаем
     {
      delay_us(10);//10ждем
      ow_in;//отпускаем линию
      delay_us(50);//50ждем
     }
   else //шлем ноль, это весь слот прижимаем к земле
     {
      delay_us(60);//60ждем
      ow_in;//отпускаем линию
      
     }
   #asm("sei")//разрешаем прерывания
  }
  
unsigned char ow_read_bit(void)
  {
   unsigned char tu;
   #asm("cli")//запрещаем прерывания
   ow_out;//линию на выход
   ow_lo;//прижимаем к земле
   delay_us(15);//15ждем
   ow_in;//отпускаем линию
   delay_us(2);//ОБЯЗАТЕЛЬНО!! 2
   if (ow_read)tu=1; //если ведомый передает единицу то он отпускает линию и на ней высокий уровень
     else tu=0;//а если ноль то он держит линию до конца слота
   #asm("sei")//разрешаем прерывания
   ow_tic=0;
   while(!ow_read)//ждем пока датчик отпустит линию
      {
       ow_tic++;
       if (ow_tic>150)//а если линия не отпускается внезапно, то ждем пока не натикает
        {
          ow_fault=f_ow_timeout;//пишем код ошибки
          return 0;//и валим сразу
        }
      }
   delay_us(20);//15и вставляем паузу для следующего слота   
   return tu;  
  }

void ow_write_byte(char by)
  {
    unsigned char temp;
    unsigned char i;
    
    for (i = 0; i < 8; i++)
    {
     temp = by&0x01;//выцепляем нужный разряд
     ow_write_bit(temp);//шлем его
     by >>= 1;//сдвигаем на следующий разряд
    }
  }

void ow_write_adress(char adr)
 {
   unsigned char i;
   for (i = 0; i < 8; i++)
     {
      ow_write_byte(rom_code[adr][i]);
     }
 }
   
  
unsigned char ow_read_byte(void)
  {
    unsigned char data=0;
    unsigned char i;

    for (i = 0; i < 8; i++)
    {
      data >>= 1;
        if (ow_read_bit()){data |= 0x80;}
    }
    return data; 
  }  

unsigned char ow_crc8(char *buffer, char size) //я нихуя не вкурил как она считается, мне было лень разбираться, поэтому я взял эту функцию в инете )))
  {
   char crc = 0;
   char i;
   char j;
   char data=0;
   for (i = 0; i < size; i++) 
     {
       data = buffer[i];
        for (j = 8; j > 0; j--)
         {
          crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
          data >>= 1;
         }
     }
   return crc;
  }  
       
char ow_read_sequ(char *sequ, char count)
 {
  char i;
  for (i=0; i<count; i++) 
    {
     sequ[i]=ow_read_byte(); 
    }
  if(!ow_crc8(sequ, count)) return 1;//если контрольная сумма совпала возвращаем 1
   else return 0; //а если нет то ноль  
 }

void ow_copy_rom(char dist)
 {
  char i;
  for (i=0; i<8; i++) 
    {
     rom_code[dist][i]=rom_curr[i]; 
    }
 }

void ow_start_conv(void)
 {
  ow_reset();
  ow_write_byte(SKIP_ROM);
  ow_write_byte(START_CONV);
 }   
 
char ow_search_rom(void)
 {
  char last_left=0; //последний поворот налево при колизии
  char last_coll=0; //последняя коллизия при прошлом проходе
  char last_dev=0;  //признак что все девайсы найдены
  char step=1;      //шаг поиска, начинаем с первого чтобы счетчик коллизий не делать с -1
  char masterbit;   //это то что будем писать в шину
  char seq;   //сюда читаем биты ответа
  char index=0;// индекс массива в котором сейчас пишем ромкод
  char mask=1;// это и так понятно
  char ow_devices=0;//количество найденных датчиков
  
  if (!ow_reset())//если нету датчиков валим, если там КЗ на линии в код ошибки запишется куда там КЗ в землю или в питание
   {return 0;}
  ow_write_byte(SEARCH_ROM);//команда поиска датчиков
  while (!last_dev)// если не найдены все датчики ищем
    {
      index=(step-1)/8;//вычисляем индекс массива для записи кода +1 томущо степ идет с единицы
      seq=0;//это и так понятно
      seq=ow_read_bit();//читаем бит
      seq<<=1;//сдвигаем его
      seq=seq+ow_read_bit();//добавляем добавочный бит ))
       switch (seq) 
      {
       case 2:
          {masterbit=1;}break;//если бит 1 то и на выход один
       case 1:
          {masterbit=0;}break;//если ноль то и на выыход ноль
       case 0://это коллизия
        {
         if (step>last_coll)//если мы уже за предыдущим последним поворотом направо
           {
            masterbit=0;//то мы поворачиваем налево
            last_left=step;//и запоминием это
           }
         
         if (last_coll==step)//если мы на прошлом последнем повороте налево
           {
            masterbit=1;//то мы поворачиваем направо
           }
         if (step<last_coll)//а если мы не дошли до последней коллизии 
           {
            masterbit=rom_curr[index]&(mask<<=step-(index*8));//то пиздуем по прошлому пути, ясенпень мы смотрим его из прошлого ромкода
            if(!masterbit)//и если мы поворачиваем влево 
             {last_left=step;} //то записываем 
           }
        }break;
       case 3: //это ошибка, не может прийти две единицы, значит никто не отвечает либо КЗ на линии на питание
        {
         ow_fault=f_ow_search;//пишем код ошибки
         return 0; //и валим
        }
      } 
      ow_write_bit(masterbit);//пишем его в ответ
      rom_curr[index]>>=1;
      if(masterbit)rom_curr[index]|= 0x80;//записываем бит в массив
       
      step++;
       if(step>64) // если мы считали все 8 байт
        {
         step=1; //начинаем с начала
         
         last_coll=last_left; //сохранияем последний поворот налево для следующего прохода

         if(last_coll==0&last_left==0)//если поворотов не было, значит мы нашли все датчики
           {last_dev=1;}
         else //инче повторяем поиск
           {  
             ow_reset(); //тут уже не проверяем на отклик, токашто проверяли
             ow_write_byte(SEARCH_ROM);
           }  
         if (ow_crc8(rom_curr,8))//если контрольная сумма не совпала
           {
            ow_fault=f_ow_crc;//пишем код ошибки
            return 0; //и валим
           }
         ow_devices++; //счетчик прибавляем, CRC то совпало
         last_left=0; //сбрасываем последний поворот налево для нового поиска
             ow_copy_rom(ow_devices-1);
         }
    }
  return ow_devices;
 }
 
signed int ow_get_temp(char numb)
 {  
   volatile signed int t;
    if (!ow_reset())//если нет ответа то валим сразу
     return 0;
     if(ow_dsnumber>1)//если больше одного девайса
       {
        ow_write_byte(MATCH_ROM);//то команда совпадения РОМ
        ow_write_adress(numb);//и пишем адрес 
       }
     else {ow_write_byte(SKIP_ROM);}//а если один все намного проще 
     
     ow_write_byte(READ_RAM);//команда чтения ОЗУ
     ow_read_sequ(scratchpad,9);//читаем девять байт ОЗУ
     if(ow_crc8(scratchpad,9))//если контрольная сумма не совпала
       {
        ow_fault=f_ow_crc;
        return 0;
       }
       //темература в первых двух байтах ОЗУ        
        t=scratchpad[1];//читаем старший байт
        t<<=8;          //сдвигаем его
        t=t+scratchpad[0];//приклеиваем младший байт
        t=t/1.6;//т.к. мы всегда конвертим в 12 бит, то приводим температуру к целому числу, разрешеное 0,1 гр
        if(t<0)t--;
        if(t==0)
        {t=1;}//тут идея такая сли мы возвращаем ноль то это не температура а неисправность датчика, хуй с ним нуля не будет 
  return t;
 }
 
 
signed int ow_get_temp_rom(char rom)
 {
  for (it=0;it<ow_dsnumber;it++)//ищем датчик с нужным кодом
   {
    if (rom_code[it][7]==rom)//если ромкод совпал
     return ow_get_temp(it);//то взвращаем температуру с этого датчика
   }
  return 0;//если датчик с таким кодом не нашелся возвращаем ноль, если он будет неисправен ноль он вернет из цикла 
 } 

