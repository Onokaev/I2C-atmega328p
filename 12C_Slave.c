//slave
#include <avr/io.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
//lcd
#define LCD_Dir DDRD					/* Define LCD data port direction */
#define LCD_Port PORTD					/* Define LCD data port */
#define RS PC1							/* Define Register Select (data reg./command reg.) signal pin */
#define EN PC0 							/* Define Enable signal pin */
#define pass(void)

void LCD_Command( unsigned volatile char cmnd);
void LCD_Char( unsigned volatile char data );
void LCD_Init (void);
void LCD_String (char *str);
void LCD_String_xy (char row, char pos, char *str);
void LCD_Clear(void);
void itoa(int _val, char* _s, int _radix);


//i2c
void I2C_Slave_Init(uint8_t slave_address);
uint8_t I2C_Slave_Listen(void);
uint8_t I2C_Slave_Transmit(char data);
char I2C_Slave_Receive(void);
#define SLAVEADDRESS 0x20
#define SLAVE_READ_ADDRESS 0x21


int main(void)
{
	LCD_Init();
	LCD_String_xy(0,0,"Slave device");
	I2C_Slave_Init(SLAVEADDRESS);
	uint8_t count = 0;
	char buffer[10];
	
	while(1)
	{
		switch(I2C_Slave_Listen())
		{
			case 0:
			{
				LCD_String_xy(1,0,"Receiving");
				do 
				{
					sprintf(buffer, "%d", count);
					LCD_String_xy(1,11,buffer);
					count = I2C_Slave_Receive();
				} while (count != -1);   //continue receiving until repeated start or stop condition
				count = 0;
				break;
				
			}
			
			case 1:              //general call address received
			{
				LCD_String_xy(1,0,"Transmitting");
				uint8_t Ack_Status;
				
				do 
				{
						sprintf(buffer, "%d", count);
						Ack_Status = I2C_Slave_Transmit(count);
						count ++;
						LCD_String_xy(1,11,buffer);
						
				} while (Ack_Status == 0);
				
				//general call address
				break;
			}
			
			case 2:
			{
				pass();
				//sla+r received and ack returned
			}
			
			default:
				break;
			
		}
		
	}
}

void I2C_Slave_Init(uint8_t slave_address)
{
	TWAR = slave_address;
	TWCR |= (1 << TWEN) | (1 << TWINT) | (1 << TWEA);  //enable TWI, clear twi interrupt and enable acknowledge bit
}

uint8_t I2C_Slave_Listen(void)
{
	while(1)
	{
		uint8_t status;
		while(!(TWCR & (1 << TWINT)));     //wait to be addressed
		status = (TWSR & 0xF8);
		
		if(status == 0x60 || status == 0x68)   //check to see if sla+w received and acknowledge returned
		{
			return 0;
		}
		if(status == 0x70 || status == 0x78)    //check if it is a general call address
		{
			return 1;
		}
		if(status == 0xA8 || status == 0xB0)      //sla+r received and ack returned
		{
			return 2;
		}
		else
		{
			continue;
		}
	}
}


uint8_t I2C_Slave_Transmit(char data)
{
	uint8_t status;
	TWDR = data;
	TWCR |= (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
	while(!(TWCR & (1 << TWINT)));             //wait for current job to be completed
	status = (TWSR & 0xF8);
	
	if (status == 0xA0)           //stop condition or repeated start has been issued while still addressed as a slave
	{
		TWCR |= (1 << TWINT);
		return -1;
	}
	if (status == 0xB8)       //check if data transmitted and acknowledge bit received
	{
		return 0;
	}
	if (status == 0xC0)
	{
		TWCR |= (1 << TWINT);
		return -2;
	}
	if (status == 0xC8)         //last data byte in twdr transmitted and ack received.
	{
		return -3;
	}
	else
	{
		return -4;
	}
}


char I2C_Slave_Receive(void)
{
	uint8_t status;
	TWCR |= (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
	while (!(TWCR & (1 << TWINT)));                     //WAIT FOR current job to be completed
	status = (TWSR & 0xF8);
	
	if (status == 0x80 || status == 0x90)                 //chcek if data received and acknowledge sent
	{
		return TWDR;
	}
	if (status == 0x88 || status == 0x98)
	{
		return TWDR;
	}
	if (status == 0xA0)        //stop condition has been received while still addressed as a slave
	{
		TWCR |= (1 << TWINT);
		return -1;
	}
	else 
	{
		return -2;
	}
	
}

void LCD_Command( unsigned volatile char cmnd )
{

	LCD_Port = (LCD_Port & 0xF0) | ((cmnd & 0xF0) >> 4); /* sending upper nibble */
	PORTC &= ~ (1<<RS);				/* RS=0, command reg. */
	PORTC |= (1<<EN);				/* Enable pulse */
	_delay_us(1);
	PORTC &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0xF0) | (cmnd & 0x0F);  /* sending lower nibble */
	PORTC |= (1<<EN);
	_delay_us(1);
	PORTC &= ~ (1<<EN);
	_delay_ms(2);
}


void LCD_Char( unsigned volatile char data )
{
	LCD_Port = (LCD_Port & 0xF0) | ((data & 0xF0) >> 4); /* sending upper nibble */
	PORTC |= (1<<RS);				/* RS=1, data reg. */
	PORTC |= (1<<EN);
	_delay_us(1);
	PORTC &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0xF0) | (data & 0x0F); /* sending lower nibble */
	PORTC |= (1<<EN);
	_delay_us(1);
	PORTC &= ~ (1<<EN);
	_delay_ms(2);
}


void LCD_Init (void)					/* LCD Initialize function */
{
	LCD_Dir |= 0x0F;						/* Make LCD command port direction as o/p */
	DDRC |= (1 << DDC0) | (1 << DDC1);
	_delay_ms(20);						/* LCD Power ON delay always >15ms */
	
	LCD_Command(0x33);
	LCD_Command(0x32);		    		/* send for 4 bit initialization of LCD  */
	LCD_Command(0x28);              	/* Use 2 line and initialize 5*7 matrix in (4-bit mode)*/
	LCD_Command(0x0c);              	/* Display on cursor off*/
	LCD_Command(0x06);              	/* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01);              	/* Clear display screen*/
	_delay_ms(2);
	LCD_Command (0x80);					/* Cursor 1st row 0th position */
}


void LCD_String (char *str)				/* Send string to LCD function */
{
	int i;
	
	for(i=0; str[i]!=0 ;i++)				/* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
	
}


void LCD_String_xy (char row, char pos, char *str)	/* Send string to LCD with xy position */
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);		/* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);		/* Command of first row and required position<16 */
	LCD_String(str);					/* Call LCD string function */
}

void LCD_Clear(void)
{
	LCD_Command (0x01);					/* Clear display */
	_delay_ms(2);
	LCD_Command (0x80);					/* Cursor 1st row 0th position */
}
