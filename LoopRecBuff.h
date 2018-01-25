/*
*********************2018.1.23*****************************

LoopRecBuff,接收环形缓冲



****2018.1.23 V0.01****
Pos正序:表示RecPos>ReadPos 
Pos倒序:表示RecPos<ReadPos

模式区别:主要是完成读取后ReadPos的位置选择。

精确模式:ReadPos 按照 AimLength 长度偏移,常用于数据帧分段接收。
粗略模式:ReadPos 按照 RecPos 的位置偏移。此模式可能需要再修改,使得功能更加明确。

错误处理:
1,新增数据长度 小于 AimLength,每次Pop_Buff(),内部usRetryCount倒计数,返回NULL,
当usRetryCount计数结束,同步ReadPos和RecPos。

*/ 

#define MaxBuffLength 							256
#define MaxTempBuffLength 						128
#define MaxReTryCount  							15+1
#define LimitedBuffTail  						(MaxBuffLength - 1)
#define LimitedBuffHead   						0	


#define PreciseMode  							0
#define RoughMode		 						1
#define BuffMode  	 							RoughMode	 


/************************************************************************************/
typedef struct 
{
	unsigned char  ucBuff[MaxBuffLength];
	unsigned short  usBuffLength;						/*缓存长度 没用到*/
	unsigned short  usRecPos;							/*接收位置*/
	unsigned short  usReadPos;							/*读取位置*/
	unsigned short  usRetryCount;						/*数据块重试次数*/
	unsigned char   ucIsReadBusy;						/*缓存块是否读取繁忙,为可能的多线程预留字节*/
}LoopBuff;

typedef enum 
{
	StatusErr = 0,
	StatusOk = 1,
	
}BuffStatus;

extern LoopBuff uartbuff;
