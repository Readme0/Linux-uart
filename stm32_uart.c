#include "include.h"
//宏定义  
#define FALSE       -1  
#define TRUE        0  
#define PACK_HEAD   0x7F
#define PACK_TAIL   0xF7

#define BUFLENGTH   100

static int uart_fd,client_fd;     //文件描述符  

/******************************************************************* 
 * 名称：            UART0_Open 
 * 功能：            打开串口并返回串口设备文件描述 
 * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2) 
 * 出口参数：        正确返回为1，错误返回为0 
 *******************************************************************/  
int UART0_Open(int fd,char* port)
{
    fd = open( port, O_RDWR|O_NOCTTY|O_NDELAY);  
    if (FALSE == fd)  
    {  
	perror("Can't Open Serial Port");  
	return(FALSE);  
    }  
    //恢复串口为阻塞状态                                 
    if(fcntl(fd, F_SETFL, 0) < 0)  
    {  
	printf("fcntl failed!\n");  
	return(FALSE);  
    }       
    else  
    {  
	printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));  
    }  

    //测试是否为终端设备      
    if(0 == isatty(STDIN_FILENO))  
    {  
	printf("standard input is not a terminal device\n");  
	return(FALSE);  
    }  
    else  
    {  
	printf("isatty success!\n");  
    }                
    printf("fd->open=%d\n",fd);  
    return fd;  
}  
/******************************************************************* 
 * 名称：                UART0_Close 
 * 功能：                关闭串口并返回串口设备文件描述 
 * 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2) 
 * 出口参数：        void 
 *******************************************************************/  

void UART0_Close(int fd)  
{  
    close(fd);  
}  

/******************************************************************* 
 * 名称：                UART0_Set 
 * 功能：                设置串口数据位，停止位和效验位 
 * 入口参数：        fd        串口文件描述符 
 *                              speed     串口速度 
 *                              flow_ctrl   数据流控制 
 *                           databits   数据位   取值为 7 或者8 
 *                           stopbits   停止位   取值为 1 或者2 
 *                           parity     效验类型 取值为N,E,O,,S 
 *出口参数：          正确返回为1，错误返回为0 
 *******************************************************************/  
int UART0_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)  
{
    int   i;  
    int   status;  
    int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};  
    int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};  

    struct termios options;  

    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1. 
     */  
    if( tcgetattr( fd,&options)  !=  0)  
    {  
    	perror("SetupSerial 1");      
    	return(FALSE);   
    }  

    //设置串口输入波特率和输出波特率  
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)  
    {  
	if(speed == name_arr[i])  
	{
	    cfsetispeed(&options, speed_arr[i]);   
	    cfsetospeed(&options, speed_arr[i]);    
	}  
    }       

    //修改控制模式，保证程序不会占用串口  
    options.c_cflag |= CLOCAL;  
    //修改控制模式，使得能够从串口中读取输入数据  
    options.c_cflag |= CREAD;  

    //设置数据流控制  
    switch(flow_ctrl)  
    {  
	case 0 ://不使用流控制  
	    options.c_cflag &= ~CRTSCTS;  
	    break;     

	case 1 ://使用硬件流控制  
	    options.c_cflag |= CRTSCTS;  
	    break;  
	case 2 ://使用软件流控制  
	    options.c_cflag |= IXON | IXOFF | IXANY;  
	    break;  
    }  
    //设置数据位  
    //屏蔽其他标志位  
    options.c_cflag &= ~CSIZE;  
    switch (databits)  
    {    
	case 5    :  
	    options.c_cflag |= CS5;  
	    break;  
	case 6    :  
	    options.c_cflag |= CS6;  
	    break;  
	case 7    :      
	    options.c_cflag |= CS7;  
	    break;  
	case 8:      
	    options.c_cflag |= CS8;  
	    break;    
	default:     
	    fprintf(stderr,"Unsupported data size\n");  
	    return (FALSE);   
    }  
    //设置校验位  
    switch (parity)  
    {    
	case 'n':  
	case 'N': //无奇偶校验位。  
	    options.c_cflag &= ~PARENB;   
	    options.c_iflag &= ~INPCK;      
	    break;   
	case 'o':    
	case 'O'://设置为奇校验      
	    options.c_cflag |= (PARODD | PARENB);   
	    options.c_iflag |= INPCK;               
	    break;   
	case 'e':   
	case 'E'://设置为偶校验    
	    options.c_cflag |= PARENB;         
	    options.c_cflag &= ~PARODD;         
	    options.c_iflag |= INPCK;        
	    break;  
	case 's':  
	case 'S': //设置为空格   
	    options.c_cflag &= ~PARENB;  
	    options.c_cflag &= ~CSTOPB;  
	    break;   
	default:    
	    fprintf(stderr,"Unsupported parity\n");      
	    return (FALSE);   
    }   
    // 设置停止位   
    switch (stopbits)  
    {    
	case 1:     
	    options.c_cflag &= ~CSTOPB; break;   
	case 2:     
	    options.c_cflag |= CSTOPB; break;  
	default:     
	    fprintf(stderr,"Unsupported stop bits\n");   
	    return (FALSE);  
    }  

    //修改输出模式，原始数据输出  
    options.c_oflag &= ~OPOST;  

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  
    //options.c_lflag &= ~(ISIG | ICANON);  

    //设置等待时间和最小接收字符  
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */    
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */  

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读  
    tcflush(fd,TCIFLUSH);  

    //激活配置 (将修改后的termios数据设置到串口中）  
    if (tcsetattr(fd,TCSANOW,&options) != 0)    
    {  
	perror("com set error!\n");    
	return (FALSE);   
    }  
    return (TRUE);   
}  
/******************************************************************* 
 * 名称：                UART0_Init() 
 * 功能：                串口初始化 
 * 入口参数：     fd       :  文件描述符    
 *               speed  :  串口速度 
 *               flow_ctrl  数据流控制 
 *               databits   数据位   取值为 7 或者8 
 *               stopbits   停止位   取值为 1 或者2 
 *               parity     效验类型 取值为N,E,O,,S
 * 出口参数：        正确返回为1，错误返回为0 
 *******************************************************************/  
int UART0_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity)  
{  
    int err;  
    //设置串口数据帧格式  
    if (UART0_Set(fd,19200,0,8,1,'N') == FALSE)  
    {                                                           
	return FALSE;  
    }  
    else  
    {  
	return  TRUE;  
    }  
}  

// typedef struct _pBuf
// {
//     unsigned char   head;//包头
//     unsigned char   length;
//     unsigned char   cmd;
//     unsigned char   data[len];
//     unsigned char   crc;
//     unsigned char   tail;//包尾
// }pBuf;

void SendPackageMsg(unsigned char cmd,unsigned char *msg,unsigned char msglen,unsigned char **pBuf,unsigned char *pBuflen)
{
    unsigned char SendBuf[50];
    unsigned char crc=0;
    int i=0,j=0;

    bzero(SendBuf,sizeof(SendBuf));
    SendBuf[i++] = PACK_HEAD;
    SendBuf[i++] = msglen+1;
    SendBuf[i++] = cmd;
    memcpy(&SendBuf[i],msg,msglen);
    i+=msglen;

    for(j=1;j<i;j++){
	crc += SendBuf[j];
    }

    SendBuf[i++] = crc;
    SendBuf[i++] = PACK_TAIL;

    //调试信息，方便查看自己发送数据是否有误
/*    printf("uart send data:\n");    
    for(j=0;j<i;j++){
	printf("0x%X ",SendBuf[j] );   
    }   
    printf("\n");
*/
    *pBuf = (unsigned char*)malloc(sizeof(SendBuf));
    *pBuflen = i;
    
    memcpy(*pBuf,SendBuf,*pBuflen);
}

#define DOOR_OPEN   1   
#define DOOR_CLOSE  2   
/******************************************************************* 
 * 名称：     UART0_Recv 
 * 功能：     接收串口数据 
 * 入口参数：  fd          :文件描述符     
 *            buf     :接收串口中数据存入rcv_buf缓冲区中 
 *            data_len    :一帧数据的长度 
 * 出口参数：  正确返回为1，错误返回为0 
 *******************************************************************/  
int UART0_RecvHander(int fd, unsigned char *buf,int data_len)
{
    int i,j;
    unsigned char cmd;
    unsigned char SendDat[10]={0};
    unsigned char *SendBuf,lenth;

    printf("receive len=%d,data is:\n",data_len);
    for(i=0;i<data_len;i++)
        printf("0x%02X", buf[i]);
    printf("\n");

    //判断包头包尾是否合法
    if((buf[0] == PACK_HEAD) && (buf[data_len-1] == PACK_TAIL))  {
    	cmd = buf[2];
        printf("uart cmd=%d\n",buf[2]);
    	switch(cmd)
    	{
    	    case DOOR_OPEN:     //开闸门
        		printf("地感检测到有JEEP车进去小区\n");
                memset(SendDat,0,sizeof(SendDat));
        		SendDat[0] = 0x01;
        		SendPackageMsg(DOOR_OPEN,SendDat,1,&SendBuf,&lenth);

        		//调试信息，方便查看自己发送数据是否有误
        		printf("uart send ACK lenth=%d,data:\n",lenth);    
        		for(j=0;j<lenth;j++){
        		    printf("0x%02X ",SendBuf[j] );   
        		}   
        		printf("\n");

        		//write(fd,SendBuf,lenth);   //应答数据给STM32,暂时无做应答处理
        		free(SendBuf);
        		break;
    	    case DOOR_CLOSE:    //关闸门
        		printf("地感检测到有JEEP车离开小区\n");
                memset(SendDat,0,sizeof(SendDat));
        		SendDat[0] = 0x00;
        		SendPackageMsg(DOOR_CLOSE,SendDat,1,&SendBuf,&lenth);
        		write(fd,SendBuf,lenth);   //应答数据给STM32,暂时无做应答处理
        		free(SendBuf);
        		break;
    	    default:
        		printf("uart cmd conn't find\n");
        		break;
    	}
    }
    else{
	   printf("uart head or tail error\n");
    }
    return FALSE;
}

/******************************************************************** 
 * 名称：      UART0_Send 
 * 功能：      发送数据 
 * 入口参数：    fd          :文件描述符     
 *              send_buf    :存放串口发送数据 
 *              data_len    :一帧数据的个数 
 * 出口参数：    正确返回为1，错误返回为0 
 *******************************************************************/  
int UART0_Send(int fd, unsigned char *send_buf,unsigned char data_len)  
{  
    int len = 0;  

    len = write(fd,send_buf,data_len);  
    if (len == data_len )  
    {  
    	printf("uart send data is:\n");
    	int i;
    	for(i=0;i<len;i++)
    	    printf("0x%02X ",*(send_buf+i));
    	printf("\n");
    	return len;  
    }
    else     
    {   
	tcflush(fd,TCOFLUSH);  
	return FALSE;  
    }
}

void *uart_recv_msg(void *arg)
{
    fd_set myset;
    int len;
    unsigned char rcv_buf[50];
    int fd,err;
    int i;

    fd_set fs_read;  
    FD_ZERO(&fs_read);  
    FD_SET(fd,&fs_read);   

    fd = UART0_Open(fd,"/dev/ttyUSB0"); //打开串口，返回文件描述符  
    do
    {
    	err = UART0_Init(fd,19200,0,8,1,'N');  
    	printf("Set Port Exactly!\n");  
    	sleep(1);
    }while(FALSE == err || FALSE == uart_fd);

    int max = fd;

    while(1)
    {
        FD_ZERO(&fs_read);  
        FD_SET(fd,&fs_read);  
        select(fd+1,&fs_read,NULL,NULL,NULL);  

        FD_ZERO(&myset);
        FD_SET(fd,&myset);
    	int ret = select(max+1,&myset,NULL,NULL,NULL);//阻塞
        if(ret<=0)
		      printf("time out ret=%d",ret);
    	if(FD_ISSET(fd,&fs_read))
    	{
    	    int len = read(fd,rcv_buf,99);//sizeofrcv_buf());  
    	    if(len > 0)
    	    {
        		//rcv_buf[len] = '\0';
        		// printf("receive len=%d,data is:\n",len);
          //       for(i=0;i<len;i++)
          //           printf("0x%02X \n", rcv_buf[i]);
          //       printf("\n");

        		UART0_RecvHander(fd,rcv_buf,len);
    	    }
    	    else
    	    {
    		      printf("cannot receive data\n");  
    	    }
    	}
        sleep(1);
    }
    UART0_Close(fd);
}

int main(int argc, char **argv)  
{
    /**************************串口发送程序**************************/
    if(0 == strcmp(argv[1],"0"))  //发送数据    
    {
    	int err,fd,i,j;
    	unsigned char *SendBuf=NULL,lenth;
    	unsigned char SendDat[50];
    	fd = UART0_Open(fd,"/dev/ttyUSB0"); //打开串口，返回文件描述符  
    	do
    	{
    	    err = UART0_Init(fd,19200,0,8,1,'N');
    	    printf("Set Port Exactly!\n");  
    	    sleep(1);
    	}while(FALSE == err || FALSE == fd);  


    	SendDat[0] = 0x01;
    	SendPackageMsg(DOOR_OPEN,SendDat,1,&SendBuf,&lenth);
    	
    	for(i=0; i<3; i++)
    	{
    	    int len = UART0_Send(fd,SendBuf,lenth);  
    	    if(len > 0)
    		printf(" %d time send %d data successful\n",i,len);  
    	    else
    		printf("send data failed!\n");  

    	    sleep(2);  
    	}
    	free(SendBuf);
    	SendBuf=NULL;
    	UART0_Close(fd);
    }
    else /************************串口接收程序************************/
    {
    	pthread_t tid;
    	pthread_create(&tid,NULL,uart_recv_msg,NULL);
	   while(1)
       {
            sleep(1);
       }
    }
}


