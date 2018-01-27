#include "string.h"
#include "stdlib.h"
#include "stdio.h"
/************************/
#include "shell.h"
#include "LoopRecBuff.h"




shell_struct cmd_list[] =
{
	{"Registered since 2018.1.26","V0.01",NULL,NULL},
	{"help","this is a cmdlist.",NULL,help_function},
	{"led1","Led1 state.",SetParaByteLength(2),led1_function},
	//{"led2on",  "Enable led2.",led2on_process},
	//{"led2off", "Disable led2.",led2off_process},
	//{"reset",   "Software reset mcu.",reset_process},
};
/**
  * @brief  内部调用,字符串转16进制
  * @note   
  * @param  *str:字符串指针;byteNum:字节数量
  * @retval temp:返回16进制数;如果错误,返回-1.
*/	
static int str2hex(char *str,unsigned char byteNum)
{
	unsigned int temp = 0,i=0,datelength = byteNum*2;
	char *tempptr = str;	
	for (; *str; str++)
	{		
		temp++;
	}
	if (temp != datelength)
		return -1;
	temp = 0;
	for (i = 0; i < datelength; i++)
	{
		if ((tempptr[i] >= 0x30) && (tempptr[i] <= 0x39))
		{
			temp |= ((unsigned char)(tempptr[i] - 0x30))<<(i*4);
		}
		else if ((tempptr[i] >= 0x41) && (tempptr[i] <= 0x46))//大写字母
		{
			temp |= ((unsigned char)tempptr[i] - 0x37)<<(i*4);
		}
		else if ((tempptr[i] >= 0x61) && (tempptr[i] <= 0x66))//小写字母
		{
			temp |= ((unsigned char)tempptr[i] - 0x57)<<(i*4);
		}
		else
		{
			return -1;
		}
	}
	return temp;
}
/**
  * @brief  内部调用,确认参数的个数,获得参数地址指针
  * @note   
  * @param  *paraHead:参数的指针;Aimparanum:目标参数个数
  * @retval *retptr:一个参数返回参数指针;两个参数返回第二个参数指针;错误返回NULL
*/
static char* check_paraNum_and_get_para(char *paraHead,unsigned char Aimparanum)
{
	unsigned char i=0, paranum=0;
	char* retptr = paraHead + 1;
	if( *paraHead != ' ')
		return NULL;
	while (*paraHead)
	{
		if (*paraHead == ' ')
		{
			paranum++;
			*paraHead = 0;
			if (paranum > 1)
				retptr = paraHead + 1;
		}
		paraHead++;
	}
	if (Aimparanum != paranum)
		return NULL;
	return retptr;
}
/**
  * @brief  外部调用,获得cmd的cmdid
  * @note   
  * @param  *cmd:需要确认的cmd
  * @retval cmdid:返回cmdid;错误返回NULL
*/
char get_cmdid(char *cmd)
{
	unsigned char cmdid = 0;
	for (cmdid = 0; cmdid< sizeof(cmd_list) / sizeof(cmd_list[0]); cmdid++)
	{
		if (strcmp(cmd_list[cmdid].cmdname, cmd) == 0)
		{
			printf("cmdid = %d\r\n", cmdid);			
			return cmdid;
		}
	}
	return NULL;
}

/*function examples*/

void help_function(char *dataptr)
{
	unsigned char  cmdid = 0;
	for (cmdid = 0; cmdid< sizeof(cmd_list) / sizeof(cmd_list[0]); cmdid++)
	{
		if(cmdid>1)
			printf("%s,%s-?\r\n",cmd_list[cmdid].cmdname,cmd_list[cmdid].cmdhelp);
		else
			printf("%s,%s\r\n", cmd_list[cmdid].cmdname,cmd_list[cmdid].cmdhelp);
	}
}

void led1_function(char *dataptr)
{
  	char *strpara1, *strpara2;
	unsigned int type[2] = { 0 };	
	/*begining check para is right or not*/
	if(dataptr == NULL)
	{
		printf("Error:%s,Line(%d),there is no para in \r\n",__FILE__,__LINE__);
		return;
	}	
	strpara1 = dataptr + 1;
	strpara2 = check_paraNum_and_get_para(dataptr,2);
	/*check para's valuenum*/
	if (strpara2 == NULL)
	{
		printf("Error:%s,Line(%d),the valuenum is invaild\r\n",__FILE__,__LINE__);
		return;
	}
	type[0] = str2hex(strpara1,1);
	type[1] = str2hex(strpara2,1);
	/*check para's valuetype*/
	if ((type[0] == 0xFFFFFFFF) ||
		(type[1] == 0xFFFFFFFF))
	{
		printf("Error:%s,Line(%d),the valuetype is invaild\r\n",__FILE__,__LINE__);
		return;
	}

	printf("Fripara = %02x ,secpara = %02x \r\n",type[0],type[1]);
}

/*reading cmd examples*/
void Read_buff(void)
{	
	unsigned char tempbuff[CmdParaMaxLength]={0};
	char cmdid=0;
	cmdid = get_cmdid((char*)Takeout_Buff(&uartbuff,tempbuff,CmdBasicLength,NotCheckAddtionalLength));
	if(cmdid==NULL)
		return;
	cmd_list[cmdid].cmd_function((char*)Takeout_Buff(&uartbuff,tempbuff,cmd_list[cmdid].cmdparamLength,CheckAddtionalLength));
	return;			
}
