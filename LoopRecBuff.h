/*
*********************2018.1.23*****************************

LoopRecBuff,接收环形缓冲



****2018.1.23 V0.01****
Pos正序:表示RecPos>ReadPos 
Pos倒序:表示RecPos<ReadPos

模式区别:主要是完成读取后ReadPos的位置选择。

PreciseMode:ReadPos 按照 AimLength 长度偏移,常用于数据帧分段接收。
RoughMode:ReadPos 按照 RecPos 的位置偏移。此模式可能需要再修改,使得功能更加明确。

错误处理:
1,新增数据长度 小于 AimLength,每次Pop_Buff(),内部usRetryCount倒计数,返回NULL,
当usRetryCount计数结束,同步ReadPos和RecPos。

****2018.1.26 V0.02****
新增: enum BuffConsType;

修改:
	 Check_RetryCount_IsOver(),添加BuffConsType IsCheckAddtionalLength形参,内部相应改动，对额外长度也做检测
	 Pop_Buff(),添加BuffConsType IsCheckAddtionalLength形参,内部相应改动,调用时可以选择是否检测额外长度

	 函数名改变:functions' names have changed
	 Pop_Buff() 改为-> Takeout_Buff()
	 Push_Buff() 改为-> Putin_Buff()

****2018.2.2 V0.03****
删除:enum BuffStatus;

新增:
	"SYS_ENUM.h"文件,
	Initializaion_Loop_Buff(),实现接收块的初始化;

修改:
	启用unsigned short usBuffLength,	
	Putin_Buff(),中长度改变,
	unsigned char   ucBuff[MaxBuffLength] 改为-> unsigned char*  ucBuff;

********************Usage********************************

  
LoopBuff *t_loopBuffPtr = NULL;
unsigned char *ucbuffaddressPtr = NULL;
unsigned short usBuffLength = "you need the size of the buff";
t_loopBuffPtr 		  = malloc(sizeof(LoopBuff));
ucbuffaddressPtr	  = malloc(usBuffLength);


if(Initializaion_Loop_Buff(t_loopBuffPtr,ucbuffaddressPtr,usBuffLength) == RET_OK)
	return "you can use this buff";
else 
	return "the initialization is failing";


******************Usage_End******************************

*/ 
#ifndef __LOOPRECBUFF_h
#define __LOOPRECBUFF_h

#include "SYS_ENUM.h"



#define MaxBuffLength 							256
#define MaxTempBuffLength 						128
#define MaxReTryCount  							15+1
#define LimitedBuffTail  						(MaxBuffLength - 1)
#define LimitedBuffHead   						0	

#define PreciseMode  							0
#define RoughMode		 							1
#define BuffMode  	 							PreciseMode	 

#define HAS_MALLOC								0
/************************************************************************************/
typedef struct 
{
	//unsigned char   ucBuff[MaxBuffLength];
	unsigned char*  ucBuff;								/*缓存指针*/
	unsigned short  usBuffLength;						/*缓存区域长度*/
	unsigned short  usRecPos;							/*接收位置*/
	unsigned short  usReadPos;							/*读取位置*/
	unsigned short  usRetryCount;						/*数据块重试次数*/
	unsigned char   ucIsReadBusy;						/*缓存块是否读取繁忙,为可能的多线程预留字节*/
}LoopBuff;

/*
typedef enum 
{
	StatusErr = 0,
	StatusOk = 1,
	
}BuffStatus;
*/

typedef enum 
{
	CheckAddtionalLength = 0,
	NotCheckAddtionalLength = 1,
	
}BuffConsType;

/*if you dont use malloc or others ,you should extern first*/
extern unsigned char ucBuffArray[MaxBuffLength];

extern LoopBuff uartbuff;
extern unsigned char* Takeout_Buff(LoopBuff *AimLoopBuff,unsigned char *Buffdst,unsigned short AimLength,BuffConsType IsCheckAddtionalLength);
extern void Putin_Buff(LoopBuff *AimLoopBuff,unsigned char data);
extern return_value Initializaion_Loop_Buff(LoopBuff *t_AimLoopBuffPtr,unsigned char *ucBuffArrayPtr,unsigned short usBuffLength);

#endif
