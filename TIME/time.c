/*****************************************************
Project : работа со временем и коррекция времени через DS1307
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
void control_time(void);//работа с DS1307 чтение времени и даты, корректировка времени, запускать раз 1-2 сек.

void day_calc(void);//вычисляет текущий день в году (с учетом високосного года) и записывает его в переменную day_current


unsigned int day_current; //текущий день в году
unsigned int corr_last; //день в году когда была последняя коррекция времени
signed int day_delta;//сколько дней прошло с последней коррекции
signed char corr;//коррекция хода часов, меньше нуля - коррекция назад, больше - вперед, размерность 100 мсек
flash const unsigned int month_light[13]={0,31,59,90,120,151,181,212,242,273,303,334}; //тут число дней на первое число которое прошло с начала года для скорости обработки и уменьшения кода, первый ноль это январь к нему еще ничего прибавлять не нужно

void day_calc(void)
  {
    day_current=month_light[month-1]+day;//миус один потому что массив начинается с нуля, а в DSке январь это 1
    if (month>2&((year-20)%4==0))//тут определяем високосный год, 20й год был високосный и если мы отнимаем его от текужего года и при делениина четыре у нас ноль значит год високосный
       {day_current++;} //ну и ясен красен прибавлять единицу нужно если февраль уже кончился 
  }

void control_time(void)
  {
   
   unsigned int daysec; //для вычисления числа секунд в дне, тк int недостаточно, будем извращаться
     
     day_calc();
     if (menu!=1){rtc_read_dt();}//читаем время если мы не в меню настройки часов
       day_delta=day_current-corr_last;//вычисляем количество дней прошедших с последней коррекции
         if(day_delta<0){day_delta=day_delta+365;}//если мы перескочили на новый год
   
   if (day_delta>=20)//если прошло больше 20 дней нужна коррекция
      {
        if(hour>0&hour<17)//т.к. мы можем по масимуму сдвинуть время на час мы должны кооректировать с часа ночи, а счетчик секунд в сутках ограничен int (если сделать long int код растет на 100байт), поэтому не позднее 17:00 должны провести коррекцию   
          {    
              daysec=hour*3600;//считаем сколько секунд прошло с начала суток
              daysec=daysec+(min*60);
              daysec=daysec+sec;
              
              daysec=daysec+(corr*day_delta)/10;//добавляем коррекцию (или вычитаем)
              
              hour=daysec/3600;//перводим скорректированное время в секундах обратно в часы
              min=(daysec%3600)/60;//минуты
              sec=daysec-((hour*3600)+(min*60));//и секунды
              
              rtc_set_time(hour,min,sec);//и записываем скорректированное время
              
              corr_last=day_current;//тк коррекция прошла ставим текуш=щий день последней коррекцией
              rtc_write(corr_last_h_m,day_current>>8);//и записываем его в DSку
              rtc_write(corr_last_l_m,day_current);


          }
      }        
  }
