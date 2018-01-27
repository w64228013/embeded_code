/*
*********************2018.1.27*****************************

shell,简单的类shell控制台



****2018.1.27 V0.01****
	实现简单的类似shell的
	指令体:4个字节;
	参数数量:最大两个;
	参数类型:字符串,int,char;

*/ 


/*设置cmd需要的参数长度*/
#define SetParaByteLength(x)   ((x<<1)+x)
#define SetParaIntLength(x)     ((x<<3)+x)
/*the basic length without para*/
#define CmdBasicLength      4
/*the para's length without basic ,the max one*/
#define CmdParaMaxLength    SetParaByteLength(2)

typedef void(*cmd_fun)(char *dataptr);
typedef struct 
{
	const char *cmdname;
	const char *cmdhelp;
	const unsigned char cmdparamLength;/*para's length with the space*/
	const cmd_fun  cmd_function;
}shell_struct;

void help_function(char *dataptr);
void led1_function(char *dataptr);

extern char get_cmdid(char *cmd);

extern void Read_buff(void);
