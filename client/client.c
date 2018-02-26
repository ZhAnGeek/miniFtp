#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include <sys/ioctl.h>
#include<errno.h>
#include<dirent.h>
#include<net/if.h>

#define MAX_INPUT_SIZE 254
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

int TYPE = TYPE_I;
char buffer[FILEBUF_SIZE];
char UserInput[MAX_INPUT_SIZE+1];
int ServerSocket, data_sock;
int Res = 0;
struct sockaddr_in ServerAddr, data_addr;
void user_login();
void cmd_syst();
void cmd_type(char *Type);
void cmd_cd(char *dir);
int cmd_port();
void cmd_list();
void cmd_get(char *filename, int flag);
void cmd_put(char *filename, int flag);
int cmd_pasv();
void cmd_mkd(char *dir);
void cmd_rmd(char *dir);
void cmd_dele(char *dir);
void cmd_rnfr(char *dir);
void cmd_pwd();
char * toServer = "to server =================>";
void ReadFromServer();
void WriteToServer(char *command,char *params);
char workspace[128];
int GetBufCode();
int getlocalip(char *outip);
int main(int argv, char **argc)
{
    strcpy(workspace,"/");
    char input_Ip[128];
    int input_Port;
    while(1) {

        printf("Please input the server IP\nPress Enter to confirm:\n(For example(127.0.0.1))\n");
        while (1) {
            scanf("%s", input_Ip);
            u_long ip = inet_addr(input_Ip);
            if (ip != INADDR_NONE) {
                break;
            }
            printf("Wrong ip please input again\n");
        }


        printf("Please input the server Port:\n");
        scanf("%d",&input_Port);

        memset(&ServerAddr, 0, sizeof(ServerAddr));
        ServerAddr.sin_family = AF_INET;
        ServerAddr.sin_addr.s_addr = inet_addr(input_Ip);
        ServerAddr.sin_port = htons((uint16_t)input_Port);

        char yn;
        while ((ServerSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            printf("failed created socket\nrecreate?[y/n]:");

            scanf("%c", &yn);
            if (yn == 'n')return 0;
        }
        yn = ' ';
        while (connect(ServerSocket, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0) {
            printf("failed connect\nreconnect?[y/n]:");
            scanf("%c", &yn);
            if (yn == 'n') {
                close(ServerSocket);
                return 0;
            }
            else if (yn == 'y')break;
        }
        if(yn == 'y')continue;
        break;
    }

    char ch;
    while((ch = getchar()) != '\n'){};

    printf("connected to %s:%d\n", input_Ip,input_Port);

    ReadFromServer();

    user_login();

    while(1)
    {
	printf("%s--:",workspace);
	fgets(UserInput, MAX_INPUT_SIZE, stdin);
	UserInput[strlen(UserInput)-1] = '\0';

	if(strncmp("quit", UserInput, 4) == 0)
	{
        WriteToServer("QUIT",NULL);
        ReadFromServer();
	    break;
	}
    else if(strncmp("abor", UserInput, 4) == 0){
        WriteToServer("ABOR",NULL);
        ReadFromServer();
        break;
    }
	else if(strncmp("syst", UserInput, 4) == 0)
	{
        cmd_syst();
	}
	else if(strncmp("type", UserInput, 4) == 0)
	{
        cmd_type(&UserInput[5]);
	}
	else if(strncmp("cd", UserInput, 2) == 0)
	{
        cmd_cd(&UserInput[3]);
	}
	else if(strncmp("port", UserInput, 4) == 0)
	{
        cmd_port();
	}
    else if(strncmp("pasv", UserInput, 4) == 0)
    {
        cmd_pasv();
    }
	else if((strncmp("ls", UserInput, 4) == 0))
	{
        cmd_list();
	}
	else if(strncmp("getp", UserInput, 4) == 0)
	{
        cmd_get(&UserInput[5], 0);
	}
	else if(strncmp("putp", UserInput, 4) == 0)
	{
        cmd_put(&UserInput[5], 0);
	}
	else if(strncmp("geta",UserInput,4) == 0)
    {
        cmd_get(&UserInput[5], 1);
    }
    else if(strncmp("puta", UserInput, 4) == 0)
    {
        cmd_put(&UserInput[5], 1);
    }
    else if(strncmp("mkd",UserInput,3) == 0)
        cmd_mkd(&UserInput[4]);
    else if(strncmp("rmd",UserInput,3) == 0)
        cmd_rmd(&UserInput[4]);
    else if(strncmp("rnfr",UserInput,4) == 0)
        cmd_rnfr(&UserInput[5]);
    else if(strncmp("dele",UserInput,4) == 0)
        cmd_dele(&UserInput[5]);
    else if(strncmp("pwd",UserInput,3) == 0)
        cmd_pwd();
    }

    close(ServerSocket);
    return 0;
}

int GetBufCode(){
    int sum = 0;
    int len = strlen(buffer);
    int j;
    int i;
    for(i = len - 2;i>0;) {
        sum = 0;

        for (j = i; j > 0; --j) {
            if (buffer[j] == '\n' && buffer[j - 1] == '\r'){
                i = j - 2;break;
            }
        }

        if (j == 0)j = -1;

        for (j = j + 1;; j++) {
            if (buffer[j] == '\r' || buffer[j] == ' ')break;
            else if (buffer[j] >= '0' && buffer[j] <= '9') {
                sum = sum * 10 + buffer[j] - '0';
            }
            else {sum = -1;break;}
        }
        if(sum != -1 && sum != 0)break;
    }
    return sum;
}

int getlocalip(char* outip)
{
    int i=0;
    int sockfd;
    struct ifconf ifconf;
    char buf[512];
    struct ifreq *ifreq;
    char* ip;
    //初始化ifconf
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;
    strcpy(outip,"127.0.0.1");
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
    {
        return -1;
    }
    ioctl(sockfd, SIOCGIFCONF, &ifconf);    //获取所有接口信息
    close(sockfd);
    //接下来一个一个的获取IP地址
    ifreq = (struct ifreq*)buf;
    for(i=(ifconf.ifc_len/sizeof(struct ifreq)); i>0; i--)
    {
        ip = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);


        if(strcmp(ip,"127.0.0.1")==0)
        {
            ifreq++;
            continue;
        }

        if(strcmp(ip,"10.0.2.15")==0)
        {
            ifreq++;
            continue;
        }
    }
    strcpy(outip,ip);
    return 0;
}

void cmd_dele(char *dir){
    WriteToServer("DELE",dir);
    ReadFromServer();
}

void cmd_rnfr(char *dir){
    WriteToServer("RNFR",dir);
    ReadFromServer();
}

void cmd_cd(char *dir)
{
    WriteToServer("CWD",dir);
    ReadFromServer();
    cmd_pwd();
}

void cmd_mkd(char *dir){
    WriteToServer("MKD",dir);
    ReadFromServer();
}

void cmd_rmd(char *dir){
    WriteToServer("RMD",dir);
    ReadFromServer();
}

int cmd_pasv(){
    WriteToServer("PASV",0);
    ReadFromServer();

    unsigned long a0,a1,a2,a3,p0,p1;
    sscanf(buffer,"227 Entering Passive Mode (%lu,%lu,%lu,%lu,%lu,%lu).",&a0,&a1,&a2,&a3,&p0,&p1);
    int addr = htonl((a0 << 24) + (a1 << 16) + (a2 << 8) + a3);

    struct sockaddr_in ServerDataSock;
    socklen_t DataSockLen = sizeof ServerDataSock;
    memset(&ServerDataSock, 0, sizeof ServerDataSock);


    ServerDataSock.sin_family = AF_INET;
    ServerDataSock.sin_port=htons((p0<<8)+p1);
    ServerDataSock.sin_addr.s_addr=addr;

    data_sock = socket(PF_INET, SOCK_STREAM, 0);

    if(data_sock == -1)	//create socket failed
    {
        printf("data socket() failed\n");
        return -1;
    }

    if(connect(data_sock, (struct sockaddr *)&ServerDataSock, DataSockLen)<0){
        printf("data connect failed\n");
        return -1;
    }

    return 0;

}

void user_login()
{
    while (1) {
        printf(">input yout userame:");
        fgets(UserInput, MAX_INPUT_SIZE, stdin);
        UserInput[strlen(UserInput) - 1] = '\0';

        WriteToServer("USER",UserInput);
        ReadFromServer();

        if (GetBufCode() == 331) {
            printf(">input your password:\n");
            fgets(UserInput, MAX_INPUT_SIZE, stdin);
            UserInput[strlen(UserInput) - 1] = '\0';

            WriteToServer("PASS", UserInput);
            ReadFromServer();

            cmd_syst();
            break;
        }

    }
    bzero(UserInput,strlen(UserInput));
}

void cmd_syst()
{
    WriteToServer("SYST",NULL);
    ReadFromServer();
}

void cmd_type(char *Type)
{
    switch(Type[0]){
        case 'A':TYPE = TYPE_A;break;
        case 'I':TYPE = TYPE_I;break;
        default:return;
    }
    WriteToServer("TYPE",Type);
    ReadFromServer();
}

int cmd_port()
{
    struct sockaddr_in LocalDataAddr;
    char localip[128];
    if(getlocalip(localip) == -1) {
        printf("error when trying to get the local addr\n");
        return -1;
    }

    LocalDataAddr.sin_family=AF_INET;
    LocalDataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    LocalDataAddr.sin_port = 0;

    int LocalDataSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(LocalDataSocket<0){
        printf("socket create failed");
        return -1;
    }

    if(bind(LocalDataSocket, (struct sockaddr *)&LocalDataAddr, sizeof LocalDataAddr)==-1){
        printf("socket create failed");
        return -1;
    }

    if(listen(LocalDataSocket,1)<0){
        printf("socket create failed");
        return -1;
    }

    struct sockaddr_in temp;
    int len = (int)(sizeof temp);
    getsockname(LocalDataSocket,(struct sockaddr *)&temp, (socklen_t *)&len);
    unsigned long port_num = ntohs(temp.sin_port);

    unsigned long a0,a1,a2,a3,p0,p1;
    sscanf(localip,"%lu.%lu.%lu.%lu",&a0,&a1,&a2,&a3);
    p0 = port_num>>8;
    p1 = port_num%256;

    char params[128];
    sprintf(params,"%lu,%lu,%lu,%lu,%lu,%lu",a0,a1,a2,a3,p0,p1);
    WriteToServer("PORT",params);

    socklen_t data_len;
    data_sock=accept(LocalDataSocket,(struct sockaddr *)&data_addr,&data_len);
    if(data_sock < 0){printf("Data accept failed\n");return -1;}
    else printf("Data Socket Accept\n");

    ReadFromServer();

    close(LocalDataSocket);
}

void cmd_list()
{
    unsigned char databuf[PIPE_BUF];
    int bytes = 0, bytesread = 0;
    
    if(cmd_pasv() == -1) return;

    WriteToServer("LIST",NULL);
    ReadFromServer();

    while ((bytes = read(data_sock, databuf, sizeof(databuf)) > 0)) {
        printf("%s", databuf);
        bytesread += bytes;
        bzero(databuf, sizeof(databuf));
    }
    close(data_sock);
    
    ReadFromServer();
}

void cmd_get(char *filename, int flag)
{
    FILE *outfile;
    short file_open=0;
    unsigned char databuf[FILEBUF_SIZE];
    int bytes = 0, bytesread = 0;

    switch (flag)
    {
        case 0:if(cmd_port() == -1) return;
                break;
        case 1:if(cmd_pasv() == -1) return;
                break;
        default:
            break;
    }

    WriteToServer("RETR", filename);
    ReadFromServer();

    if(GetBufCode()/100 == 5){
        return;
    }

    while ((bytes = read(data_sock, databuf, FILEBUF_SIZE)) > 0) {
	if (file_open == 0) {
	    if ((outfile = fopen(filename, "wb")) == 0) {
		printf("fopen failed to open file");
		close(data_sock);
		return;
	    }
	    file_open = 1;
	}
	write(fileno(outfile), databuf, bytes);
	bytesread += bytes;
    }

    if (file_open != 0) fclose(outfile);

    close(data_sock);
    ReadFromServer();
    printf("%d bytes get.\n", bytesread);
}

void cmd_put(char *filename, int flag)
{
    FILE *infile;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes, bytessend = 0;
    
    infile = fopen(filename,"rb");
    if(infile == 0)
    {
	printf("fopen() failed");
	return;
    }

    switch (flag){
        case 0:if(cmd_port() == -1) return;break;
        case 1:if(cmd_pasv() == -1) return;break;
        default:break;
    }

    WriteToServer("STOR",filename);
    ReadFromServer();
    
    while((bytes = read(fileno(infile), databuf, FILEBUF_SIZE)) > 0)
    {
        bytessend += bytes;
        write(data_sock, (const char *)databuf, bytes);
	    memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);
    
    fclose(infile);
    close(data_sock);
    
    ReadFromServer();
    
    printf("%d bytes send\n", bytessend);
}

void WriteToServer(char *command,char *params){
    char reply[MAX_INPUT_SIZE+1];
    if(params == NULL){
        sprintf(reply, "%s\r\n", command);
    }
    else sprintf(reply, "%s %s\r\n", command, params);
    write(ServerSocket, reply, strlen(reply));
    printf("%s %s",toServer, reply);
}

void ReadFromServer()
{
    Res = read(ServerSocket, buffer, sizeof(buffer));
    buffer[Res-2] = 0;
    printf("%s\n", buffer);
}

void cmd_pwd(){
    WriteToServer("PWD",NULL);
    ReadFromServer();
    strcpy(workspace,buffer);
}




