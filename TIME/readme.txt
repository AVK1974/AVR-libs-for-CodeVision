работа со временем через DS1307 используя DS1307.c и кооректировка времени +-10 сек в сутки. Будет работать даже если устройство выключено. 
определьть адреса где будем хранить данные
#define corr_last_h_m         0x08 //старший байт последнего дня коррекции 
#define corr_last_l_m         0x09 //младший байт последнего дня коррекции
#define corr_m                0x0A //величина коррекции



В начале main нужно сделать
   rtc_write(0x07,0x10);//настройка DS1301 на включение выхода на 1 Гц, мы строго работаем в 24часовом формате, поэтому остальные флаги трогать не нужно, часы пойдут как только мы запишем в них время
   rtc_read_dt();
   
   day_calc();//вычисляем текущий день года
   
  //грузим настойки из DSки   
   corr_last=rtc_read_int(corr_last_h_m);//читаем день года последней коррекции
   corr=rtc_read(corr_m);

потом в теле раз в 1-2 секунды вызываем control_time()
corr это величина коррекции. отрицательнаы в минус , положительные в плюс, размерность 100мсек
больше 99 и меньше -99 нельзя таким образом максимум мы можем скорректировать 9,9 сек.
Корректировка происходит раз в 10 дней. Если устройство было выключено, посчитается сколько дней прошло с последней корректировки и сообветственно произойдет корректировка на необходимое время. максимальный простой до корректировки чуть больше года
