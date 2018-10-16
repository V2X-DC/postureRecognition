#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <termios.h>  
#include <stdio.h>  
#include <strings.h> 
#include <stdlib.h>
#include <unistd.h> 
#define BAUDRATE        B9600 
#define UART_DEVICE     "/dev/ttyUSB1"  
#define MAX_PACKET_SIZE 1024 

#define FALSE  -1  
#define TRUE   0  
////////////////////////////////////////////////////////////////////////////////  
/** 
*@brief  设置串口通信速率 
*@param  fd     类型 int  打开串口的文件句柄 
*@param  speed  类型 int  串口速度 
*@return  void 
*/  
int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,  
                   B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, };  
int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200,  300,   
                  115200, 38400, 19200, 9600, 4800, 2400, 1200,  300, };  
void set_speed(int fd, int speed){  
  int   i;   
  int   status;   
  struct termios   Opt;  
  tcgetattr(fd, &Opt);   
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {   
    if  (speed == name_arr[i]) {       
      tcflush(fd, TCIOFLUSH);       
      cfsetispeed(&Opt, speed_arr[i]);    
      cfsetospeed(&Opt, speed_arr[i]);     
      status = tcsetattr(fd, TCSANOW, &Opt);    
      if  (status != 0) {          
        perror("tcsetattr fd1");    
        return;       
      }      
      tcflush(fd,TCIOFLUSH);     
    }    
  }  
}  
////////////////////////////////////////////////////////////////////////////////  
/** 
*@brief   设置串口数据位，停止位和效验位 
*@param  fd     类型  int  打开的串口文件句柄 
*@param  databits 类型  int 数据位   取值 为 7 或者8 
*@param  stopbits 类型  int 停止位   取值为 1 或者2 
*@param  parity  类型  int  效验类型 取值为N,E,O,,S 
*/  
int set_Parity(int fd,int databits,int stopbits,int parity)  
{   
    struct termios options;   
    if  ( tcgetattr( fd,&options)  !=  0) {   
        perror("SetupSerial 1");       
        return(FALSE);    
    }  
    options.c_cflag &= ~CSIZE;   
    switch (databits) /*设置数据位数*/  
    {     
    case 7:       
        options.c_cflag |= CS7;   
        break;  
    case 8:       
        options.c_cflag |= CS8;  
        break;     
    default:      
        fprintf(stderr,"Unsupported data size\n"); return (FALSE);    
    }  
    switch (parity)   
    {     
        case 'n':  
        case 'N':      
            options.c_cflag &= ~PARENB;   /* Clear parity enable */  
            options.c_iflag &= ~INPCK;     /* Enable parity checking */   
            break;    
        case 'o':     
        case 'O':       
            options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/    
            options.c_iflag |= INPCK;             /* Disnable parity checking */   
            break;    
        case 'e':    
        case 'E':     
            options.c_cflag |= PARENB;     /* Enable parity */      
            options.c_cflag &= ~PARODD;   /* 转换为偶效验*/       
            options.c_iflag |= INPCK;       /* Disnable parity checking */  
            break;  
        case 'S':   
        case 's':  /*as no parity*/     
            options.c_cflag &= ~PARENB;  
            options.c_cflag &= ~CSTOPB;break;    
        default:     
            fprintf(stderr,"Unsupported parity\n");      
            return (FALSE);    
        }    
    /* 设置停止位*/    
    switch (stopbits)  
    {     
        case 1:      
            options.c_cflag &= ~CSTOPB;    
            break;    
        case 2:      
            options.c_cflag |= CSTOPB;    
           break;  
        default:      
             fprintf(stderr,"Unsupported stop bits\n");    
             return (FALSE);   
    }   
    /* Set input parity option */   
    if (parity != 'n')     
        options.c_iflag |= INPCK;   
    tcflush(fd,TCIFLUSH);  
    options.c_cc[VTIME] = 150; /* 设置超时15 seconds*/     
    options.c_cc[VMIN] = 0; /* Update the options and do it NOW */  
    if (tcsetattr(fd,TCSANOW,&options) != 0)     
    {   
        perror("SetupSerial 3");     
        return (FALSE);    
    }   
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/  
    options.c_oflag  &= ~OPOST;   /*Output*/  
    return (TRUE);    
}  
////////////////////////////////////////////////////////////////////////////////  
void parseData(char *buf) 
{ 	
    int nQ, nN, nB, nC; 	
    char cX, cY, cM1, cM2; 
    float fTime, fX, fY, fP, fH, fB, fD;  
    if (buf == NULL) 	
    { 		
        printf("error: Can't get buf!n"); 	
        return; 	
    } 	
    sscanf(buf,"$GNGGA,%f,%f,%c,%f,%c,%d,%02d,%f,%f,%c,%f,%c,%f,%04d%02x",&fTime,&fX,&cX,&fY,&cY,&nQ,&nN,&fP,&fH,&cM1,&fB,&cM2, &fD, &nB, &nC);  	
    printf("x: %c %f, y: %c %f, h %f, satellite: %dn",cX, fX, cY, fY, fH, nN);
}

/*cX：N or S;fX：纬度；cY：E or W；fY：经度；fH：height；nN：卫星个数*/  
int main(int argc, char *argv[]) 
{ 	int nb,command; 	
    int um220_fd = -1; 	
    char newbuf[MAX_PACKET_SIZE]; 	
    char msg[20],*ret=NULL; 
    struct termios oldtio, newtio;  	/*Open Um220 Module*/ 	
    if ((um220_fd = open(UART_DEVICE, O_RDWR)) < 0) 
    { 	
        printf("error: Can't open serial port %s!n", UART_DEVICE);              
        return -1;         
    }  	
    /*Init Uart for Um220*/ 	
    printf("Open...\n");  
    set_speed(um220_fd,9600);  
    if (set_Parity(um220_fd,8,1,'N') == FALSE)  {  
        printf("Set Parity Error\n");  
        exit (0);  
    }  
  
    printf("Reading...\n");  	
    /*Set Um220 options*/ 	
    printf("Please select modules of um220n"); 	
    printf("1.BD modulen"); 
    printf("2.GPS modulen"); 
    printf("3.BD+GPS modulen"); 
    if(scanf("%d",&command) != 1) 
    { 	
        printf("error:input is wrong!n");
    } 	
    switch(command) 
    { 	
        case 1: 		
            memset(msg, 0, sizeof(msg)); 	
            strcpy(msg,"$cfgsys,h01"); 		
            if(write(um220_fd,msg,sizeof(msg)) < 0) 
                printf("Failed to set BD modules!n"); 	
            break; 		
        case 2: 		
            memset(msg, 0, sizeof(msg));
            strcpy(msg,"$cfgsys,h10"); 	
            if(write(um220_fd,msg,sizeof(msg)) < 0) 	
                printf("Failed to set GPS modules!n"); 	
            break; 	
        case 3: 		
            memset(msg, 0, sizeof(msg)); 	
            strcpy(msg,"$cfgsys,h11"); 	
            if(write(um220_fd,msg,sizeof(msg)) < 0) 
                printf("Failed to set BD+GPS modules!n"); 	
            break; 	
        default: 		
            printf("Can't identify command,set BD+GPS modules!n"); 	
            memset(msg, 0, sizeof(msg)); 	
            strcpy(msg,"$cfgsys,h11"); 		
            if(write(um220_fd,msg,sizeof(msg)) < 0) 
                printf("Failed to set BD+GPS modules!n");
    }  
    for(;;) 
    { 
        /*Read Data from Um220*/ 
        memset(newbuf, 0, 1024); 
        nb = read(um220_fd, newbuf, MAX_PACKET_SIZE);     
        if (nb == -1) 	
        {              
            perror("read uart error"); 	
            return -1; 	
        } 
        if ((ret=strstr(newbuf, "$GNGGA")) != NULL) 
        { 
            /*Analysis Data*/ 	
            parseData(ret); 	
        } 
        sleep(1); 
    } 	
    /*Close Um220_fd*/ 
    close(um220_fd);
    return 0;
}

