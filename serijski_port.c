#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

//Velicina prijemnog bafera (mora biti 2^n)
#define USART_RX_BUFFER_SIZE 64

char Rx_Buffer[USART_RX_BUFFER_SIZE];			//prijemni FIFO bafer
volatile unsigned char Rx_Buffer_Size = 0;	//broj karaktera u prijemnom baferu
volatile unsigned char Rx_Buffer_First = 0;
volatile unsigned char Rx_Buffer_Last = 0;

ISR(USART_RX_vect)
{
  	Rx_Buffer[Rx_Buffer_Last++] = UDR0;		//ucitavanje primljenog karaktera
	Rx_Buffer_Last &= USART_RX_BUFFER_SIZE - 1;	//povratak na pocetak u slucaju prekoracenja
	if (Rx_Buffer_Size < USART_RX_BUFFER_SIZE)
		Rx_Buffer_Size++;					//inkrement brojaca primljenih karaktera
}

void usartInit(unsigned long baud)
{
	UCSR0A = 0x00;	//inicijalizacija indikatora
					//U2Xn = 0: onemogucena dvostruka brzina
					//MPCMn = 0: onemogucen multiprocesorski rezim

	UCSR0B = 0x98;	//RXCIEn = 1: dozvola prekida izavanog okoncanjem prijema
					//RXENn = 1: dozvola prijema
					//TXENn = 1: dozvola slanja

	UCSR0C = 0x06;	//UMSELn[1:0] = 00: asinroni rezim
					//UPMn[1:0] = 00: bit pariteta se ne koristi
					//USBSn = 0: koristi se jedan stop bit
					//UCSzn[2:0] = 011: 8bitni prenos

	UBRR0 = F_CPU / (16 * baud) - 1;

	sei();	//I = 1 (dozvola prekida)
}

unsigned char usartAvailable()
{
	return Rx_Buffer_Size;		//ocitavanje broja karaktera u prijemnom baferu
}

void usartPutChar(char c)
{
	while(!(UCSR0A & 0x20));	//ceka da se setuje UDREn (indikacija da je predajni bafer prazan)
	UDR0 = c;					//upis karaktera u predajni bafer
}

void usartPutString(char *s)
{
	while(*s != 0)				//petlja se izvrsava do nailaska na nul-terminator
	{
		usartPutChar(*s);		//slanje tekuceg karaktera
		s++;					//azuriranje pokazivaca na tekuci karakter
	}
}

void usartPutString_P(const char *s)
{
	while (1)
	{
		char c = pgm_read_byte(s++);	//citanje sledeceg karaktera iz programske memorije
		if (c == '\0')					//izlazak iz petlje u slucaju
			return;						//nailaska na terminator
		usartPutChar(c);				//slanje karaktera
	}
}

char usartGetChar()
{
	char c;

	if (!Rx_Buffer_Size)						//bafer je prazan?
		return -1;
  	c = Rx_Buffer[Rx_Buffer_First++];			//citanje karaktera iz prijemnog bafera
	Rx_Buffer_First &= USART_RX_BUFFER_SIZE - 1;	//povratak na pocetak u slucaju prekoracenja
	Rx_Buffer_Size--;							//dekrement brojaca karaktera u prijemnom baferu

	return c;
}

unsigned char usartGetString(char *s)
{
	unsigned char len = 0;

	while(Rx_Buffer_Size) 			//ima karaktera u faferu?
		s[len++] = usartGetChar();	//ucitavanje novog karaktera

	s[len] = 0;						//terminacija stringa
	return len;						//vraca broj ocitanih karaktera
}
#define BR_KORISNIKA 10

 char korisnici[BR_KORISNIKA][32] =

{

    "Sundjer Bob Kockalone",

    "Dijego Armando Maradona",

    "Bond. Dzejms bond.",

    "Zoran Kostic Cane",

    "Kim Dzong Un",

    "Avatar Aang",

    "Toph Beifong",

    "Quentin Tarantoni" ,

    "Geralt of Rivia",

    "Pizza de Pasta"
};
char PIN[BR_KORISNIKA][5] =
{"5346",
 "2133",
 "7445",
 "8756",
 "7435",
 "1213",
 "6523",
 "6352",
 "6323",
 "3625"};

int main()
{
	usartInit(9600);
  	char identifikacija[32];	//uneto ime preko serijskog porta
  	char PIN_uneti[5] = {'\0'};	//uneti pin --||--
  	char u_bazi = -1;			//u ovu promenljivu upisuje se redni broj korisnika, ako je pronadjen u bazi
  	char broj_pokusaja = 0;		//broj pokusaja da se unese ispravan PIN

	while(1)
	{
      u_bazi = -1;			//resetovanje vrednosti
      broj_pokusaja = 0;	//resetovanje vrednosti
      usartPutString("Unesite Ime i Prezime: \r\n");

      while(!usartAvailable());
      _delay_ms(100);
      usartGetString(identifikacija);
      for(int i = 0; i< 10; i++){	//provera da li je korisnik u bazi, ako jeste, u u_bazi se upisuje redni broj korisnika
        if(!strcmp(identifikacija,korisnici[i]))
          u_bazi = i;
      }//end for petlje
      if(u_bazi >= 0){	//tj. ako je pronadjen korisnik
        while(broj_pokusaja < 3){

         	usartPutString("Unesite PIN \r\n");

        	for(int i =0; i < 4; i++){//unos PINa karakter po karakter
      	 	while(!usartAvailable());
       		_delay_ms(10);
            usartPutChar('*');
        	PIN_uneti[i] = usartGetChar();
            }//end for petlje za unos PINa
        	PIN_uneti[4] = '\0';//terminacije unetog PINa

        	usartPutString("\r\n");

      			if(!(strcmp(PIN_uneti,PIN[u_bazi]))){//provera da li je ispravan PIN
              	usartPutString("PIN OK \r\n");
        		_delay_ms(1500);
                break;	}
        		else {
                	if(broj_pokusaja == 2)
                      usartPutString("Pojeo sam ti karticu   NJAM \r\n");
                  	else
                      usartPutString("GRESKA!\r\n");
                broj_pokusaja++;
                _delay_ms(1500);
                }//kraj provere ispravnosti unetog PINa
        }//kraj while-a za 3 pokusaja
      }
      else	{//ako nije pronadjen korisnik u bazi
      	  usartPutString("Korisnik nije pronadjen !\r\n");
          _delay_ms(1500); }//kraj glavnog if-else-a

	}

	return 0;
}


