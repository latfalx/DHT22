#include "DHT22.h"
enum DHT22_LEVEL{
	DHT22_READ, DHT22_LOW, DHT22_HIGH
};
enum DHT22_LEVEL DHT22_STEP;
//--------------------------------/	Стартовый сигнал датчику
void DHT22_Start(){																
	DHT22_DDR_OUT;
	DHT22_PORT_0;
	_delay_ms(20);
	DHT22_PORT_1;
	_delay_us(30);
	DHT22_DDR_IN; 
}
//--------------------------------/	Ожидания ответа датчика	
unsigned short DHT22_AskReady(){
	_delay_us(40);
	if(~DHT22_STATUS){
		_delay_us(80);
		if(DHT22_STATUS){
			_delay_us(50);
			return 0;
		} else
			return 1;
	} else			
	return 1;
}
//--------------------------------/	Считывания байта
unsigned char DHT22_ReadByte(){
	int i, run, step=0;
	unsigned char rbyte=0;
	for(i=0; i<8; i++){
		run=1;
		while(run){
			switch(DHT22_STEP){
				case DHT22_READ:
					if(DHT22_STATUS) DHT22_STEP = DHT22_HIGH;
					else DHT22_STEP = DHT22_LOW;
					break;
				case DHT22_HIGH:
					step++;
					if(step>100) return 0;
					DHT22_STEP = DHT22_READ;
					break;	
				case DHT22_LOW:
					if(step){
						if(step<11) rbyte &= ~(1<<(7-i));	// Предел step<[7..16]
						else rbyte |= 1<<(7-i);
					step = 0;
					run = 0;
					}
					DHT22_STEP = DHT22_READ;
					break;
			}
		} 
	}
	return rbyte;
}
//--------------------------------/	Склеиваем байты в слово
unsigned short DHT22_GlueByte(unsigned char byte1, unsigned char byte2){
	return (byte1<<8) | byte2;
}
//--------------------------------/	Проверка подлинности слов полученных от датчика
unsigned short DHT22_GetWord(unsigned short *word1, unsigned short *word2){
	unsigned char b1,b2,b3,b4,b5,sum;
	b1 = DHT22_ReadByte();
	b2 = DHT22_ReadByte();
	b3 = DHT22_ReadByte();
	b4 = DHT22_ReadByte();
	b5 = DHT22_ReadByte();
	sum = b1 + b2 + b3 + b4;
	if(b5==sum){
		*word1=DHT22_GlueByte(b1,b2);
		*word2=DHT22_GlueByte(b3,b4);
		return 0;
	} else
		return 1;
}
//--------------------------------/	Запрос данных в виде числа: Влажность, Температура
unsigned short DHT22_AskIntSensor(unsigned short *word1, unsigned short *word2){
	int i;
	for (i=0; i<2; i++){
		DHT22_Start();
		if(!DHT22_AskReady())
			if(!DHT22_GetWord(word1,word2))
				return 0;	
	}
	return 1;
}
//--------------------------------/	Запрос данных в виде строки: Влажность(char[4]), Температура(char[5])
unsigned short DHT22_AskStrSensor(char *shmr, char *stmr){
	unsigned short hmr,tmr;
	if(!DHT22_AskIntSensor(&hmr, &tmr)){
		// Влажность
		if(hmr>=1000){	// Если 100% и более, то пишим _100			
			shmr[0]=20; shmr[1]=49;	shmr[2]=48; shmr[3]=48;	shmr[4]=0;								
		} else {									
			if(!(hmr/100)) shmr[0]=20;	// Если нет десятичных
			else shmr[0]=(hmr/100)+48;	// Десятичные	
			shmr[1]=((hmr/10)%10)+48;	// Единицы
			shmr[2]=0;			// Запятая 0x2C
			shmr[3]=0;			// Дробная часть (hmr%10)+48
			shmr[4]=0;								
		}
		// Температура										
		if(tmr==0) stmr[0]=20;		// 00,0 Убераем первый 0
		else 
			if(tmr>0x8000){		// Определяем знак
				tmr &= 0x7FFF;	// Убераем бит знака
				stmr[0] = 45;	// Ставим -
			} else									
				stmr[0] = 43;	// Ставим +
				
		if(!((tmr/10)/10)) stmr[1]=32;	// Если нет десятичных
		else stmr[1]=(tmr/100)+48;	// Десятичные
		stmr[2]=((tmr/10)%10)+48;	// Единицы
		stmr[3]=0x2C;			// Запятая
		stmr[4]=(tmr%10)+48;		// Дробной часть
		stmr[5]=0;								
	} else { 
	// Если нет данных
			shmr[0]=0xCD; shmr[1]=0x2F; shmr[2]=0xC4; shmr[3]=0x00; shmr[4]=0x00;			// Н / Д
			stmr[0]=0x20; stmr[1]=0xCD; stmr[2]=0x2F; stmr[3]=0xC4; stmr[4]=0x00; stmr[5]=0x00;	// Н / Д
	}
	return 0;
}
