#ifndef __USART_H
#define __USART_H
#include "sys.h"
//#include "stdio.h"	 
 
#define USART_REC_LEN  			16*1024 //�����������ֽ��� 41K
#define EN_USART1_RX 			1		//ʹ�ܣ�1��/��ֹ��0������1����
	  	
extern u8  USART_RX_BUF[USART_REC_LEN]; //���ջ���,���USART_REC_LEN���ֽ�.ĩ�ֽ�Ϊ���з� 
extern u16 USART_RX_STA;         		//����״̬���	
extern u16 USART_RX_CNT;				//���յ��ֽ���
//����봮���жϽ��գ��벻Ҫע�����º궨��
void uart_init(u32 pclk2,u32 bound);

#endif	   
















