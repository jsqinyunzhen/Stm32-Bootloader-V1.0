#include "sys.h"
#include "usart.h"		
#include "delay.h"	
#include "led.h"   	 
#include "stmflash.h"	
#include "iap.h"	 
#include "crc.h"
#include "string.h"
uint32_t sys_ms_cnt;


#define	WAIT_UPDATE     0
#define	WAIT_BEGIN_FLAG	1   
#define	REC_BIN	    2
#define	REC_OVER    3
#define	UPDATE_SUCCESS    4								    
u8 my_memcmp(u8 *p1,u8 *p2,u32 len)
{
    if( p1 == 0 || p2 == 0)
    {
        return 1;
    }
    while(len > 0)
    {
        len --;
        if(p1[len]^p2[len])
        {
            return 1;
        }
    }
    return 0;
}
void RS485_SendString(u8 * pstr,u8 len)
{
    u8 i = 0;
    
    PAout(11) = 1;
    PAout(12) = 1;
    delay_ms(5);
    for(i = 0; i < len; i++)
    {
        while((USART1->SR&0X40)==0);//等待上一次串口数据发送完成  
        USART1->DR = (u8) pstr[i];      	//写DR,串口1将发送数据
    }
    delay_ms(5);
    PAout(11) = 0;
    PAout(12) = 0;
}
//u8 buf []={0xff,0xff,0xff,0xff,0xff,0xff};
int main(void)
{		 
	u8 t;
	u8 key = WAIT_UPDATE;
	u16 oldcount=0;	//老的串口接收数据值
	u16 applenth=0;	//接收到的app代码长度
	u32 clearflag=0;  
 	Stm32_Clock_Init(9);	//系统时钟设置
	uart_init(72,115200);	//串口初始化为256000
	delay_init(72);	   	 	//延时初始化 
	LED_Init();		  		//初始化与LED连接的硬件接口
	//LCD_Init();			   	//初始化LCD
 	//KEY_Init();				//按键初始化
//	RS485_SendString(buf,sizeof(buf));
	while(1)
	{
	 	if(key == REC_BIN)
		{
            if(USART_RX_CNT)
            {
                if(oldcount==USART_RX_CNT)//新周期内,没有收到任何数据,认为本次数据接收完成.
                {
                    applenth=USART_RX_CNT;
                    oldcount=0;
                    USART_RX_CNT=0;
                    //printf("用户程序接收完成!\r\n");
                    //printf("代码长度:%dBytes\r\n",applenth);
                    key = REC_OVER;
                }
                else 
                {
                    oldcount=USART_RX_CNT;	
                    clearflag = 0;
                }
            }
            else
            {
                clearflag = 0;
            }
		}
		t++;clearflag++;
		delay_ms(10);
		if(t==30)
		{
			LED0=!LED0;
			t=0;

			//printf("开始更新固件...\r\n");	
		}	  	 
		//key=KEY_Scan(0);
		if(key == REC_OVER)			//WK_UP按键按下
		{
			if(applenth)
			{
				//printf("开始更新固件...\r\n");	
				//LCD_ShowString(60,210,200,16,16,"Copying APP2FLASH...");
				#if 0
				if(crc16(0xFFFF,USART_RX_BUF,applenth) == 0)
				{
					u16 crc = 0;
					crc = crc16(0xFFFF,USART_RX_BUF,applenth-2);
					crc = crc;
				}
                #endif
 				if(((*(vu32*)(0X20001000+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
				{	 
					if(crc16(0xFFFF,USART_RX_BUF,applenth) == 0)
					{
                        u8 successss[] = "update bin success!\r\n";
                        iap_write_appbin(FLASH_APP1_ADDR,USART_RX_BUF,applenth -2);//更新FLASH代码  
                        RS485_SendString(successss,sizeof(successss)-1);
                        
                        key = UPDATE_SUCCESS;
					}
					else
					{
                        u8 successss[] = "update CRC FAILED!!!!\r\n";
                        RS485_SendString(successss,sizeof(successss)-1);
                        key = WAIT_UPDATE;
					}
					//LCD_ShowString(60,210,200,16,16,"Copy APP Successed!!");
					//printf("固件更新完成!\r\n");	
					
				}
				else 
				{
					u8 successss[] = "bin file error!!!!\r\n";
					RS485_SendString(successss,sizeof(successss)-1);
					key = WAIT_UPDATE;
					//LCD_ShowString(60,210,200,16,16,"Illegal FLASH APP!  ");	   
					//printf("非FLASH应用程序!\r\n");
				}
 			}else 
			{
				//printf("没有可以更新的固件!\r\n");
				//LCD_ShowString(60,210,200,16,16,"No APP!");
			}
							 
		}
        if(key == WAIT_UPDATE)
        {
            if(USART_RX_CNT > 15)
		    {
                u32 i = 0;
                u8 wait1ssend[] = "wait 1 Second\r\n";
                u8 upFlag[] = "\r\nupdate\r\n";
                
                for(i = 0; i < USART_RX_CNT-sizeof(upFlag)+1; i++)
                {
                    if(my_memcmp(USART_RX_BUF+i,upFlag,10)==0)
                    {
                        key = WAIT_BEGIN_FLAG;
                        break;
                    }
                }
                
                if(key == WAIT_BEGIN_FLAG)
                {
                    clearflag = 0;
                    RS485_SendString(wait1ssend,sizeof(wait1ssend)-1);
                    USART_RX_CNT=0;
                }
            }
        }
        if(key == WAIT_BEGIN_FLAG)
        {
            if(USART_RX_CNT > 0)
            {
                clearflag = 0;         
							
                USART_RX_CNT = 0;
            }
            if(clearflag > 100)
            {
                key = REC_BIN;
                USART_RX_CNT = 0;
                clearflag =0;
            }
        }
		if(clearflag > 200 || key == UPDATE_SUCCESS) //2s 
		{
			//printf("开始执行FLASH用户代码!!\r\n");

			if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
			{	 
				if(key == UPDATE_SUCCESS)
				{
					RS485_SendString("run new code\r\n",strlen("run new code\r\n"));
				}
				else
				{
					RS485_SendString("run old code\r\n",strlen("run old code\r\n"));
				}
				iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
			}
			else 
			{
				//printf("非FLASH应用程序,无法执行!\r\n");
				RS485_SendString("user code failed\r\n",strlen("user code failed\r\n"));
                key = WAIT_UPDATE;
				//LCD_ShowString(60,210,200,16,16,"Illegal FLASH APP!");	   
			}									 
 
		}
        #if 0
		if(key==3)
		{
			printf("开始执行SRAM用户代码!!\r\n");
			if(((*(vu32*)(0X20001000+4))&0xFF000000)==0x20000000)//判断是否为0X20XXXXXX.
			{	 
				iap_load_app(0X20001000);//SRAM地址
			}else 
			{
				printf("非SRAM应用程序,无法执行!\r\n");
				//LCD_ShowString(60,210,200,16,16,"Illegal SRAM APP!");	   
			}									 
			clearflag=7;//标志更新了显示,并且设置7*300ms后清除显示	 
		}			
        #endif
		 
	}   	   
}







