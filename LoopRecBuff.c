#include "string.h"
#include "stdlib.h"
#include "stdio.h"

LoopBuff uartbuff = 
{
		{NULL}, //ucBuff ucTempbuff
		LimitedBuffTail,//usBuffLength
		LimitedBuffHead,//usRecPos
		LimitedBuffHead,//usReadPos
		MaxReTryCount,//usRetryCount
		0,//ucIsReadBusy
};

/**
  * @brief  内部使用,当重试次数超过MaxReTryCount后调用
  * @note   
  * @param  *AimLoopBuff:缓存块结构体的指针;RecPos:接收的缓存位置
  * @retval None
*/
void Syn_Loop_Buff_Pos(LoopBuff *AimLoopBuff,unsigned short RecPos)
{
	AimLoopBuff->usReadPos = RecPos;
	AimLoopBuff->usRetryCount = MaxReTryCount;
}
/**
  * @brief  未使用,重置读取位置和接收位置
  * @note   
  * @param  *AimLoopBuff:缓存块结构体的指针;
  * @retval None
*/
void Reset_Loop_Buff(LoopBuff *AimLoopBuff)
{
	/*前面需要增加响应的中断停止函数，防止Reset的瞬间触发中断*/
	
	AimLoopBuff->usReadPos = AimLoopBuff->usRecPos = 0;
}
/**
  * @brief  内部调用,确认缓存长度是否有效
  * @note   
  * @param  *AimLoopBuff:缓存块结构体的指针;curLength:新增缓存的长度;AimLength:目标需要的长度;RecPos:接收的缓存位置,IsCheckAddtionalLength:是否需要检测超出的长度;
  * @retval 1:长度正确;0:长度错误
*/
__inline BuffStatus Check_RetryCount_IsOver(LoopBuff *AimLoopBuff,unsigned short curLength,unsigned short AimLength,unsigned short RecPos,unsigned char IsRetainAddtionalLength)
{
	/*如果新增长度 >= 目标长度 说明正常返回true*/
	if(curLength == AimLength)
		{
			AimLoopBuff->usRetryCount = MaxReTryCount;
			return StatusOk;
		}
	else if(curLength > AimLength)
		{
			/*如果新增长度 > 目标长度 并且需要检测额外长度*/	
		 if(IsRetainAddtionalLength != NotCheckAddtionalLength)
			{
				/*执行同步并且返回StatusErr*/
				Syn_Loop_Buff_Pos(AimLoopBuff,RecPos);
				return StatusErr;
			}
			AimLoopBuff->usRetryCount = MaxReTryCount;
			return StatusOk;
		}	
	else
	{
		/*如果重试次数到了则执行同步*/
		if( --AimLoopBuff->usRetryCount == 0 )
		{
//			printf("%d\r\n",AimLoopBuff->usRetryCount);
			Syn_Loop_Buff_Pos(AimLoopBuff,RecPos);
		}
//		else {printf("%d\r\n",AimLoopBuff->usRetryCount);}
		return StatusErr;
	}
}
/**
  * @brief  外部调用,读取响应长度的
  * @note   
  * @param  *AimLoopBuff:缓存块结构体的指针;*Buffdst:弹出数据的拷贝地址;AimLength:目标需要的长度,IsCheckAddtionalLength:是否需要检测超出的长度;
  * @retval *Buffdst:正确的话返回拷贝地址;NULL:错误的话返回空指针地址
*/	
unsigned char* Takeout_Buff(LoopBuff *AimLoopBuff,unsigned char *Buffdst,unsigned short AimLength,BuffConsType IsCheckAddtionalLength)
{
	const unsigned short constRecPos = AimLoopBuff->usRecPos;
	int offset = constRecPos - AimLoopBuff->usReadPos; 
	unsigned short tempoffset = {0}; 
	
	if(offset == 0)//没有新数据返回空指针
		return NULL;
	/*offset>0,Pos正序,新增长度是 = offset*/
	else if(offset > 0)
	{	
		if(Check_RetryCount_IsOver(AimLoopBuff,offset,AimLength,constRecPos,IsCheckAddtionalLength) != StatusOk)
			return NULL;
		/*Pos正序方式读取*/
		/*保存初始ReadPos*/		
		tempoffset = AimLoopBuff->usReadPos;
		
		#if BuffMode == PreciseMode
			AimLoopBuff->usReadPos += AimLength;
		#elif	BuffMode == RoughMode
			AimLoopBuff->usReadPos = constRecPos;
		#endif /*BuffMode == RoughMode*/
		
		/*数据搬到dst地址所指的区域,并返回dst地址*/
		return memcpy(Buffdst,&AimLoopBuff->ucBuff[tempoffset],AimLength);
	}
	/*offset<0,Pos倒序,新增长度 = offset+MaxBuffLength */
	else if(offset < 0)
	{		
		/*如果目标长度 > 新增长度,返回NULL,不对Buff做读取*/
		/*
			发生这种情况的原因有:
		  1,读取的瞬间数据片段还在发送,解决:等待或再多调用几次本函数,超过次数同步
		  2,读取的可能是被破坏的碎片数据,解决:超过次数同步
		*/
		/*先计算出当前 这里:offset 是负数 + 整个buff长度 = 实际新增buff长度*/
		/*返回 NULL 说明数据有问题 return NULL*/
		if (Check_RetryCount_IsOver(AimLoopBuff,offset+MaxBuffLength,AimLength,constRecPos,IsCheckAddtionalLength) != StatusOk)
			return NULL;
		/*Pos倒序,先读取尾部剩余长度tempoffset*/
		tempoffset = MaxBuffLength - AimLoopBuff->usReadPos;
		/*如果目标长度 <= 尾部剩余长度,则按照Pos正序方式读取*/
		if(AimLength <= tempoffset)
		{
			tempoffset = AimLoopBuff->usReadPos;
			
			#if BuffMode == PreciseMode
				AimLoopBuff->usReadPos += AimLength;
			#elif	BuffMode == RoughMode
				AimLoopBuff->usReadPos = constRecPos;
			#endif /*BuffMode == RoughMode*/		
			
			return memcpy(Buffdst,&AimLoopBuff->ucBuff[tempoffset],AimLength);
		}
		/*Pos倒序方式读取*/
		/*1:读取尾部剩余*/
		memcpy(Buffdst,&AimLoopBuff->ucBuff[AimLoopBuff->usReadPos],tempoffset);		
		/*2::读取头部 目标长度 - 尾部长度 */
		memcpy(Buffdst + tempoffset,&AimLoopBuff->ucBuff[0],AimLength - tempoffset);
		
		#if BuffMode == PreciseMode
			AimLoopBuff->usReadPos = AimLength - tempoffset;
		#elif	BuffMode == RoughMode
			AimLoopBuff->usReadPos = constRecPos;
		#endif /*BuffMode == RoughMode*/
		
		/*数据搬到dst地址所指的区域,并返回dst地址*/
		return Buffdst;		
	}
}
/**
  * @brief  外部调用,接收数据
  * @note   
  * @param  *AimLoopBuff:缓存块结构体的指针;data:接收到的每一个字节
  * @retval 
*/
void Putin_Buff(LoopBuff *AimLoopBuff,unsigned char data)
{	
	AimLoopBuff->ucBuff[AimLoopBuff->usRecPos++] = data;
	if( AimLoopBuff->usRecPos > LimitedBuffTail )
	{
		AimLoopBuff->usRecPos = LimitedBuffHead;
	}
}



/*receive example*/
void USART1_IRQHandler(void)                	
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE)== SET)
	{
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
		Putin_Buff(&uartbuff,USART_ReceiveData(USART1));
	}

}

/*read example*/
void Read_Buff(void)
{ 
	unsigned char tempbuff[10];	
	printf("the data is %02x",Takeout_Buff(&uartbuff,tempbuff,4));
}


