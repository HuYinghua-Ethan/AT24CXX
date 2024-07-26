/**
* @file		beep.c
* @brief	RC522指纹识别模块
* @version  1.0
* @author   Ethan
* @date     2024.7.13
*/


#include "24cxx.h" 
#include "delay.h"

/**
* @Description	
*  连线：
*  A0--GND
*  A1--GND
*  A2--GND
*  VSS--GND
*  VCC--3.3V
*  WP--GND
*  SCL--PB10
*  SDA--PB11
*/
 
/*
使用说明：
在main.c初始化AT24CXX芯片，调用C04_IIC_Init()函数即可。
如果用的是不同型号的AT24CXX芯片，只需要在at24cxx.h中修改相关的宏定义即可，本例程用的是AT24C02
#define EE_TYPE AT24C02

在main.c用来测试 at24c02 
while(AT24CXX_Check()) //检测不到24c02
{
	LCD_Display_Words(0,0,"未检测到存储模块");
	delay_ms(500);
}

*/



 
 /*  
 空闲状态: IIC总线的SDA和SCL两条信号线同时处于高电平时，规定位总线的空闲状态。
 此时各个器件的输出级场效应管均处在截止状态，即释放总线，由两条信号线各自的上拉电阻把电平拉高。   
 */
 
 
/**
* @Description	24C02存储模块初始化
* @param	None
* @return   None
*/
void C04_IIC_Init(void)
{					     
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE );	//使能GPIOB时钟
	   
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB,GPIO_Pin_10|GPIO_Pin_11); 	//输出高
}
 
 
/**
* @Description	产生IIC起始信号
  当SCL为高电平期间，SDA由高到低的跳变;
  启动信号是一种电平跳变时序信号，而不是一个电平信号。
* @param	None
* @return   None
*/
void C04_IIC_Start(void)
{
	C04_SDA_OUT();     //sda线输出
	C04_IIC_SDA=1;	  	  
	C04_IIC_SCL=1;
	delay_us(4);
 	C04_IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	C04_IIC_SCL=0;//钳住I2C总线，准备发送或接收数据 
}



/**
* @Description	产生IIC停止信号
  当SCL为高电平期间，SDA由低到高的跳变;
  停止信号也是一种高电平跳变时序信号，而不是一个电平信号
* @param	None
* @return   None
*/
void C04_IIC_Stop(void)
{
	C04_SDA_OUT();//sda线输出
	C04_IIC_SCL=0;
	C04_IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	C04_IIC_SCL=1; 
	C04_IIC_SDA=1;//发送I2C总线结束信号
	delay_us(4);							   	
}



/**
* @Description	等待应答信号到来
  发送器每发送一个字节，就在时钟脉冲9期间释放数据线，由接收器反馈一个应答信号。
  应答信号为低电平时，规定为有效应答位(ACK简称应答位)，
  应答信号为高电平时，规定为非应答位(NACK)，一般表示接收器接收该字节没有成功。
* @param	None
* @return   1: 接收应答失败  0: 接收成功
*/
u8 C04_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	C04_SDA_IN();      //SDA设置为输入  
	C04_IIC_SDA=1;
	delay_us(1);	   
	C04_IIC_SCL=1;
	delay_us(1);	 
	while(READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			C04_IIC_Stop();
			return 1;
		}
	}
	C04_IIC_SCL=0;//时钟输出0 	   
	return 0;  
}



/**
* @Description	产生ACK应答
* @param	None
* @return   None
*/
void C04_IIC_Ack(void)
{
	C04_IIC_SCL=0;
	C04_SDA_OUT();
	C04_IIC_SDA=0;
	delay_us(2);
	C04_IIC_SCL=1;
	delay_us(2);
	C04_IIC_SCL=0;
}


/**
* @Description	不产生ACK应答
* @param	None
* @return   None
*/    
void C04_IIC_NAck(void)
{
	C04_IIC_SCL=0;
	C04_SDA_OUT();
	C04_IIC_SDA=1;
	delay_us(2);
	C04_IIC_SCL=1;
	delay_us(2);
	C04_IIC_SCL=0;
}				


/**
* @Description	IIC发送一个字节,返回从机有无应答  1:有应答  0:无应答
* @param	txd 要发送的字节
* @return   None
*/
void C04_IIC_Send_Byte(u8 txd)
{                        
    u8 t;
	C04_SDA_OUT(); 	    
    C04_IIC_SCL=0;		//拉低时钟开始数据传输
    for(t=0;t<8;t++)
    {              
        C04_IIC_SDA=(txd&0x80)>>7; 
        txd <<= 1; 	  
		delay_us(2);   //对TEA5767这三个延时都是必须的
		C04_IIC_SCL=1;
		delay_us(2);
		C04_IIC_SCL=0;
		delay_us(2);
    }	 
} 	 


/**
* @Description	读1个字节 如果主机接收到应答信号，就会发送应答信号给从机
* @param	ack 应答信号  ack=1时，发送ACK，ack=0，发送nACK
* @return   返回从机的应答信号
*/
u8 C04_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;
	C04_SDA_IN();		//SDA设置为输入  接收应答信号要将SDA线设置为输入模式
    for(i=0;i<8;i++ )
	{
        C04_IIC_SCL=0; 
        delay_us(2);
		C04_IIC_SCL=1;
        receive<<=1;
        if(READ_SDA)
			receive++;   
		delay_us(1); 
    }					 
    if (!ack)  //如果主机接收到
        C04_IIC_NAck();//发送nACK
    else
        C04_IIC_Ack(); //发送ACK   
    return receive;
}


 /**
* @Description	在AT24CXX指定地址读出一个数据
* @param	ReadAddr:开始读数的地址
* @return   读到的数据
*/
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
    C04_IIC_Start();                   //产生起始信号
	if(EE_TYPE > AT24C16)
	{
		C04_IIC_Send_Byte(0XA0);	   //发送写命令
		C04_IIC_Wait_Ack();            //等待应答
		C04_IIC_Send_Byte(ReadAddr>>8);//发送高地址
		C04_IIC_Wait_Ack();		       //每发一次数据就等待应答
	}
	else 
		C04_IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	 

	C04_IIC_Wait_Ack(); 
    C04_IIC_Send_Byte(ReadAddr%256);   //发送低地址
	C04_IIC_Wait_Ack();	    
	C04_IIC_Start();  	 	   
	C04_IIC_Send_Byte(0XA1);           //进入接收模式			   
	C04_IIC_Wait_Ack();	 
    temp=C04_IIC_Read_Byte(0);		   
    C04_IIC_Stop();//产生一个停止条件	    
	return temp;
}


/**
* @Description	在AT24CXX指定地址写入一个数据
* @param	WriteAddr:写入数据的目的地址  
  @param    DataToWrite:要写入的数据
* @return   None
*/
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
    C04_IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		C04_IIC_Send_Byte(0XA0);	    //发送写命令
		C04_IIC_Wait_Ack();
		C04_IIC_Send_Byte(WriteAddr>>8);//发送高地址
 	}else
	{
		C04_IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 
	}	 
	C04_IIC_Wait_Ack();	   
    C04_IIC_Send_Byte(WriteAddr%256);   //发送低地址
	C04_IIC_Wait_Ack(); 	 										  		   
	C04_IIC_Send_Byte(DataToWrite);     //发送字节							   
	C04_IIC_Wait_Ack();  		    	   
    C04_IIC_Stop();//产生一个停止条件 
	delay_ms(10);	 
}



/**
* @Description	在AT24CXX里面的指定地址开始写入长度为Len的数据,该函数用于写入16bit或者32bit的数据
* @param	WriteAddr  :开始写入的地址
  @param	DataToWrite:数据数组首地址
  @param    Len        :要写入数据的长度2,4
* @return   None
*/
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}


 /**
* @Description	在AT24CXX里面的指定地址开始读出长度为Len的数据,该函数用于读出16bit或者32bit的数据.
* @param	ReadAddr   :开始读出的地址
  @param    Len        :要读出数据的长度2,4
* @return   数据
*/
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}


 /**
* @Description	检查AT24CXX是否正常,这里用了24XX的最后一个地址(255)来存储标志字.(如果用其他24C系列,这个地址要修改)
* @param	None
* @return   返回1:检测失败  返回0:检测成功
*/
u8 AT24CXX_Check(void)
{
	u8 temp;
	temp=AT24CXX_ReadOneByte(255);//避免每次开机都写AT24CXX			   
	if(temp==0X55)
		return 0;		   
	else//排除第一次初始化的情况
	{
		AT24CXX_WriteOneByte(255,0X55);
	    temp=AT24CXX_ReadOneByte(255);	  
		if(temp==0X55)return 0;
	}
	return 1;											  
}


 /**
* @Description	在AT24CXX里面的指定地址开始读出指定个数的数据
* @param	ReadAddr :开始读出的地址 对24c02为0~255
  @param    pBuffer  :数据数组首地址
  @param    NumToRead:要读出数据的个数
* @return   None
*/
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  


 /**
* @Description	在AT24CXX里面的指定地址开始写入指定个数的数据
* @param	WriteAddr :开始写入的地址 对24c02为0~255
  @param    pBuffer   :数据数组首地址
  @param    NumToWrite:要写入数据的个数
* @return   None
*/
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}
 


