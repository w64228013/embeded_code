#include "string.h"
#include "stdlib.h"
#include "stdio.h"
/************************/
#include "SYS_ENUM.h"
#include "LoopRecBuff.h"

//LoopBuff uartbuff = 
//{
//		NULL, //ucBuff address
//		MaxBuffLength,//usBuffLength
//		LimitedBuffHead,//usRecPos
//		LimitedBuffHead,//usReadPos
//		MaxReTryCount,//usRetryCount
//		0,//ucIsReadBusy
//};

unsigned char ucBuffArray[MaxBuffLength] = {0};

/**
  * @brief  内部使用,当重试次数超过MaxReTryCount后调用
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;RecPos:接收的缓存位置
  * @retval None
*/
void Syn_Loop_Buff_Pos(LoopBuff *t_AimLoopBuffPtr,unsigned short RecPos)
{
	t_AimLoopBuffPtr->usReadPos = RecPos;
	t_AimLoopBuffPtr->usRetryCount = MaxReTryCount;
}
/**
  * @brief  未使用,重置读取位置和接收位置
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;
  * @retval None
*/
void Reset_Loop_Buff(LoopBuff *t_AimLoopBuffPtr)
{
	/*前面需要增加响应的中断停止函数，防止Reset的瞬间触发中断*/
	
	t_AimLoopBuffPtr->usReadPos = t_AimLoopBuffPtr->usRecPos = 0;
}
/**
  * @brief  内部调用,确认缓存长度是否有效
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;curLength:新增缓存的长度;AimLength:目标需要的长度;RecPos:接收的缓存位置,IsCheckAddtionalLength:是否需要检测超出的长度;
  * @retval RET_OK:长度正确;RET_ERR:长度错误
*/
__inline return_value Check_RetryCount_IsOver(LoopBuff *t_AimLoopBuffPtr,unsigned short curLength,unsigned short AimLength,unsigned short RecPos,unsigned char IsRetainAddtionalLength)
{
	/*如果新增长度 >= 目标长度 说明正常返回true*/
	if(curLength == AimLength)
		{
			t_AimLoopBuffPtr->usRetryCount = MaxReTryCount;
			return RET_OK;
		}
	else if(curLength > AimLength)
		{
			/*如果新增长度 > 目标长度 并且需要检测额外长度*/	
		 if(IsRetainAddtionalLength != NotCheckAddtionalLength)
			{
				/*执行同步并且返回StatusErr*/
				Syn_Loop_Buff_Pos(t_AimLoopBuffPtr,RecPos);
				return RET_ERR;
			}
			t_AimLoopBuffPtr->usRetryCount = MaxReTryCount;
			return RET_OK;
		}	
	else
	{
		/*如果重试次数到了则执行同步*/
		if( --t_AimLoopBuffPtr->usRetryCount == 0 )
		{
//			printf("%d\r\n",t_AimLoopBuffPtr->usRetryCount);
			Syn_Loop_Buff_Pos(t_AimLoopBuffPtr,RecPos);
		}
//		else {printf("%d\r\n",t_AimLoopBuffPtr->usRetryCount);}
		return RET_ERR;
	}
}
/**
  * @brief  外部调用,读取响应长度的
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;*Buffdst:弹出数据的拷贝地址;AimLength:目标需要的长度,IsCheckAddtionalLength:是否需要检测超出的长度;
  * @retval *Buffdst:正确的话返回拷贝地址;NULL:错误的话返回空指针地址
*/	
unsigned char* Takeout_Buff(LoopBuff *t_AimLoopBuffPtr,unsigned char *Buffdst,unsigned short AimLength,BuffConsType IsCheckAddtionalLength)
{
	const unsigned short constRecPos = t_AimLoopBuffPtr->usRecPos;
	int offset = constRecPos - t_AimLoopBuffPtr->usReadPos; 
	unsigned short tempoffset = {0}; 
	/*没有新数据,或者AimLength为0*/
	if((offset == 0)||(AimLength==0))
		return NULL;
	/*offset>0,Pos正序,新增长度是 = offset*/
	else if(offset > 0)
	{	
		if(Check_RetryCount_IsOver(t_AimLoopBuffPtr,offset,AimLength,constRecPos,IsCheckAddtionalLength) != RET_OK)
			return NULL;
		/*Pos正序方式读取*/
		/*保存初始ReadPos*/		
		tempoffset = t_AimLoopBuffPtr->usReadPos;
		
		#if BuffMode == PreciseMode
			t_AimLoopBuffPtr->usReadPos += AimLength;
		#elif	BuffMode == RoughMode
			t_AimLoopBuffPtr->usReadPos = constRecPos;
		#endif /*BuffMode == RoughMode*/
		
		/*数据搬到dst地址所指的区域,并返回dst地址*/
		return memcpy(Buffdst,&t_AimLoopBuffPtr->ucBuff[tempoffset],AimLength);
	}
	/*offset<0,Pos倒序,新增长度 = offset+t_AimLoopBuffPtr->usBuffLength */
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
		if (Check_RetryCount_IsOver(t_AimLoopBuffPtr,offset+t_AimLoopBuffPtr->usBuffLength,AimLength,constRecPos,IsCheckAddtionalLength) != RET_OK)
			return NULL;
		/*Pos倒序,先读取尾部剩余长度tempoffset*/
		tempoffset = t_AimLoopBuffPtr->usBuffLength - t_AimLoopBuffPtr->usReadPos;
		/*如果目标长度 <= 尾部剩余长度,则按照Pos正序方式读取*/
		if(AimLength <= tempoffset)
		{
			tempoffset = t_AimLoopBuffPtr->usReadPos;
			
			#if BuffMode == PreciseMode
				t_AimLoopBuffPtr->usReadPos += AimLength;
			#elif	BuffMode == RoughMode
				t_AimLoopBuffPtr->usReadPos = constRecPos;
			#endif /*BuffMode == RoughMode*/		
			
			return memcpy(Buffdst,&t_AimLoopBuffPtr->ucBuff[tempoffset],AimLength);
		}
		/*Pos倒序方式读取*/
		/*1:读取尾部剩余*/
		memcpy(Buffdst,&t_AimLoopBuffPtr->ucBuff[t_AimLoopBuffPtr->usReadPos],tempoffset);		
		/*2::读取头部 目标长度 - 尾部长度 */
		memcpy(Buffdst + tempoffset,&t_AimLoopBuffPtr->ucBuff[0],AimLength - tempoffset);
		
		#if BuffMode == PreciseMode
			t_AimLoopBuffPtr->usReadPos = AimLength - tempoffset;
		#elif	BuffMode == RoughMode
			t_AimLoopBuffPtr->usReadPos = constRecPos;
		#endif /*BuffMode == RoughMode*/
		
		/*数据搬到dst地址所指的区域,并返回dst地址*/
		return Buffdst;		
	}
	return NULL;
}
/**
  * @brief  外部调用,接收数据
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;data:接收到的每一个字节
  * @retval 
*/
void Putin_Buff(LoopBuff *t_AimLoopBuffPtr,unsigned char data)
{	
	t_AimLoopBuffPtr->ucBuff[t_AimLoopBuffPtr->usRecPos++] = data;
	if( t_AimLoopBuffPtr->usRecPos >= t_AimLoopBuffPtr->usBuffLength )
	{
		t_AimLoopBuffPtr->usRecPos = LimitedBuffHead;
	}
}
/**
  * @brief  外部调用,接收数据根据长度
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;*ucBuffPtr:接受数据的指针;usBuffLength:数据长度;
  * @retval 
*/
void Putin_Buff_By_Length(LoopBuff *t_AimLoopBuffPtr,unsigned char *ucBuffPtr,unsigned short usBuffLength)
{
	while(usBuffLength--)
		Putin_Buff(t_AimLoopBuffPtr,*(ucBuffPtr++));
}
/**
  * @brief  外部调用,初始化接收块
  * @note   
  * @param  *t_AimLoopBuffPtr:缓存块结构体的指针;*ucBuffArrayPtr:块的内存地址;usBuffLength:块的长度
  * @retval RET_OK:成功;RET_ERR:失败
*/
return_value Initializaion_Loop_Buff(LoopBuff *t_AimLoopBuffPtr,unsigned char *ucBuffArrayPtr,unsigned short usBuffLength)
{
	if((usBuffLength > MaxBuffLength)||(ucBuffArrayPtr == NULL))
		return RET_ERR;
	t_AimLoopBuffPtr->ucBuff = ucBuffArrayPtr;/*地址赋值*/
	t_AimLoopBuffPtr->usBuffLength = usBuffLength;/*长度赋值*/	
	t_AimLoopBuffPtr->usRecPos = LimitedBuffHead;
	t_AimLoopBuffPtr->usReadPos = LimitedBuffHead;
	t_AimLoopBuffPtr->usRetryCount = MaxReTryCount;
	t_AimLoopBuffPtr->ucIsReadBusy = 0;
	return RET_OK;	
}

/*receive example*/
//void USART1_IRQHandler(void)                	
//{
//	if(USART_GetITStatus(USART1,USART_IT_RXNE)== SET)
//	{
//		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
//		Putin_Buff(&uartbuff,USART_ReceiveData(USART1));
//	}
//}

/*read example*/
//void Read_Buff(void)
//{ 
//	unsigned char tempbuff[10];	
//	printf("the data is %02x",Takeout_Buff(&uartbuff,tempbuff,4));
//}


