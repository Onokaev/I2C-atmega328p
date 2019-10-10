#include <avr/io.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#define BITRATE(TWSR) (((F_CPU/100000)-16)/(2*(TWSR & ((0 << TWPS0) | (1 << TWPS1)))))
#define pass(void)
#define count 10
//lcd
#define LCD_Dir DDRD					/* Define LCD data port direction */
#define LCD_Port PORTD					/* Define LCD data port */
#define RS PC1							/* Define Register Select (data reg./command reg.) signal pin */
#define EN PC0 							/* Define Enable signal pin */

void LCD_Command( unsigned volatile char cmnd);
void LCD_Char( unsigned volatile char data );
void LCD_Init (void);
void LCD_String (char *str);
void LCD_String_xy (char row, char pos, char *str);
void LCD_Clear(void);
void itoa(int _val, char* _s, int _radix);


//i2c
void I2C_Init(void);
uint8_t I2C_Write(char write_address);
uint8_t I2C_Repeated_Start(char read_address);
uint8_t I2C_Write1(char data);
char I2C_Read_Ack(void);
char I2C_Read_No_ack(void);
void I2C_Stop(void);
#define SLAVE_WRITE_ADDRESS 0x20
#define SLAVE_READ_ADDRESS 0x21      //general call address with twgce=1




int main(void)
{
	char buffer[10];
	DDRB |= (1 << DDB0);
	LCD_Init();
	I2C_Init();
	
	LCD_String_xy(0,0,"Master device");
	while(1)
	{
		I2C_Write(SLAVE_WRITE_ADDRESS);
		LCD_String_xy(1,0,"Sending");		
		_delay_ms(5);
		
		for(uint8_t i = 0; i < count; i++)
		{
			PORTB |= (1 << PORTB0);
			sprintf(buffer, "%d", i);
			I2C_Write1(i);
			LCD_String_xy(1,10,buffer);
			_delay_ms(200);
			
		}
		
		LCD_String_xy(1,0,"Receiving");
		
		uint8_t Ack_N = I2C_Repeated_Start(SLAVE_READ_ADDRESS);
		
		if(Ack_N == 3)
		{
			PORTB |= (1 << PORTB1);
			LCD_Clear();
			LCD_String("Failed");
		}
		else if (Ack_N == 2)
		{
			PORTB |= (1 << PORTB3);
			LCD_Clear();
			LCD_String("nack received");
		}
		else if(Ack_N == 0)
		{
			LCD_Clear();
			LCD_String("Start Cond??");          //nothing happenning
		}
		else
		{
			LCD_String("No idea");
		}
		_delay_ms(5);
		for(uint8_t i = 0; i < count ; i++)
		{
			if(i < count - 1)
			{
				sprintf(buffer, "%d", I2C_Read_Ack());
				LCD_String_xy(1,11,buffer);
				
			}
			else
			{
				sprintf(buffer, "%d", I2C_Read_No_ack());
				LCD_String_xy(1,11,buffer);
				_delay_ms(500);
			}
			
		}
		
		
		I2C_Stop();
		
	}
}





void I2C_Init(void)
{
	TWBR = BITRATE(TWSR = 0x00);
}

//I2C events

//start and slave address+W
uint8_t I2C_Write(char write_address)
{
	uint8_t status;
	TWCR |= (1 << TWEN) | (1 << TWSTA) | (1 << TWINT);  //enalbe twi, start condition and interrupt
	while(!(TWCR & (1 << TWINT))); //wait until twi has finished its current job
	status = (TWSR & 0xF8);
	
	if(status != 0x08)  //status should read 0x08 if start transmitted successfully. 
	{
		return 0;         //this terminates the program and returns this exit value
	} 
	
	TWDR = write_address;        //copy destination address to data register
	TWCR |= (1 << TWEN) | (1 << TWINT);
	while (!(TWCR & (1 << TWINT)));                //wait for transmission to finish
	status = (TWSR & 0xF8);
	
	if(status != 0x18)
	{
		return 1;
	}
	
	if (status == 0x20)   //check if address sent but nack received
	{
		return 2;
	}
	else
	{
		return 3;         //to indicate SLA+W failed
	}
	
}

uint8_t I2C_Repeated_Start(char read_address)
{
	uint8_t status;
	TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
	while(!(TWCR & (1 << TWINT)));  //LOOP until transmission done
	
	status = (TWSR & 0xF8);      //read status register
	
	if (status != 0x10)   //check whether repeated start conditon has been transmitted
	{
		return 0;           //exit with 0 if repeated start transmission failed
	}
	
	TWDR = read_address;
	TWCR = (1 << TWEN) | (1 << TWINT);
	while (! (TWCR & (1 << TWINT)));      //wait for current twi job to finish
	status = (TWCR & 0xF8);
	
	if (status == 0x40)     //check whether slave address sent and ack received
	{
		return 1;            //return 1 for successful ack
	}
	if (status == 0x48)       //check whether nack received
	{
		return 2;              //return 2 if nack received
	}
	else
	{
		return 3;              //failed
	}
	
}

uint8_t I2C_Write1(char data)
{
	uint8_t status;
	TWDR = data;
	TWCR |= (1 << TWEN) | (1 << TWINT);
	while (! (TWCR & (1 << TWINT)));       //wait for twi to complete current job
	
	status = (TWSR & 0xF8);
	
	if(status == 0x28)   //check if data bytes  transmitted and ack received  
	{
		return 0;
	}
	if (status == 0x30)    //check if data byte transmitted and nack received
	{
		return 1;
	}
	else
	{
		return 2;           //else return 2 for data transmission failure
	}
	
	
}

char I2C_Read_Ack(void)
{
	TWCR |= (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
	while(!(TWCR & (1 << TWINT)));
	return TWDR;      //RETURN received data and send acknowledge to slave
}

char I2C_Read_No_ack(void)
{
	TWCR |= (1 << TWEN) | (1 << TWINT);    //don't return acknowledge
	while(!(TWCR & (1 << TWINT)));
	return TWDR;
}

void I2C_Stop(void)
{
	TWCR |= (1 << TWEN) | (1 << TWINT) | (1 << TWSTO);
	while(TWCR & (1 << TWSTO));                           //wait until stop condition successfully executed
	
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
