//
// Created by zsk on 17-10-29.
//
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<dirent.h>

#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

int serverSocket,clientSocket,dataSocket,Res, TYPE;
struct sockaddr_in serverAddr,clientAddr,dataAddr;
size_t serverAdrLen,clientAdrLen,dataAdrlen;

void ntorn_tail(char *info);
char buffer[FILEBUF_SIZE];
char *toClient = "to client =================>";
char *username = "anonymous";
char loginname[FILEBUF_SIZE];
void PORT_handler(char* param,char *reply);
void TYPE_handler(char *t,char *reply);
void RETR_handler(char* param,char *reply);
void PASV_handler(char *reply);
const char *MODE[] = {"BINARY","ASCII"};
void WriteToClient(char *reply,int clientSocket);
void STOR_handler(char *filename, char *reply);
void LIST_handler(char *reply);
void CWD_handler(char *dir,char *reply);
void MKD_handler(char *dir,char *reply);
void RMD_handler(char *dir,char *reply);
void DELE_handler(char *dir,char *reply);
void PWD_handler(char *reply);

int clean_directory_file(char *dir);
void ReadFromCLient();
int default_port = 21;
char default_dir[128] = "/tmp";
void RNFR_handler(char *dir,char *reply);

int main(int argc,char *argv[])
{
    int i;
    for (i = 1; i < argc; i++){
        if(strcmp(argv[i],"-port")==0){
            if(atoi(argv[i + 1]) != 0){default_port = atoi(argv[i + 1]);}
        }
        else if(strcmp(argv[i],"-root")==0){
            char * temp_dir;
            temp_dir = argv[i + 1];
            struct stat dir_stat;
            if (access(temp_dir, F_OK) == 0 && stat(temp_dir, &dir_stat)>=0)
            {
                if(S_ISDIR(dir_stat.st_mode)){strcpy(default_dir,argv[i + 1]);}
            }
        }
    }

    chdir(default_dir);

    printf("Welcome to ftp server\n");

    serverSocket=socket(PF_INET,SOCK_STREAM,0);

    if(serverSocket == -1){printf("Failed creating sockets\n");}
    else printf("Socket successfully created\n");

    memset(&serverAddr, 0 ,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(default_port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(serverAddr.sin_addr.s_addr == INADDR_NONE)printf("Inaddr\n");

    serverAdrLen = sizeof serverAddr;


    Res = bind(serverSocket, (struct sockaddr *)&serverAddr, serverAdrLen);
    if(Res == -1)printf("Bind error\n");
    else printf("Bind Ok!\n");

    Res = listen(serverSocket,5);
    if(Res < 0)printf("listen failed\n");
    else printf("listening\n");

    while(1){
        TYPE = TYPE_I;
        printf("wait for acceptance\n");

        clientAdrLen = sizeof clientAddr;
        clientSocket=accept(serverSocket,(struct sockaddr *)&clientAddr,(socklen_t *)&clientAdrLen);
        if(clientSocket < 0){printf("accept failed\n");return 1;}
        else printf("New Socket Accept %d\n",clientSocket);

        pid_t p = fork();
        if(p < 0){printf("fork failed");}
        else if(p != 0){continue;}

        char reply[200] = "220 FTP server ready\r\n";
        write(clientSocket, reply, (int)strlen(reply));
        printf("%s %s",toClient,reply);

        while(1){
            ReadFromCLient();

            char cmdName[10]={'\0'};
            strncpy(cmdName,buffer,4);

            if(strcmp(cmdName, "USER") == 0)
            {
                strcpy(loginname, &buffer[5]);
                if(strcmp(username,loginname)){
                    strcpy(reply, "530 Error log in(not anonymous),please rewrite the username again\r\n");
                    WriteToClient(reply,clientSocket);
                }
                else {
                    strcpy(reply, "331 Guest login ok, send your complete e-mail address as password.\r\n");
                    WriteToClient(reply,clientSocket);
                    break;
                }
            }
            else {strcpy(reply,"530 Unknown command\r\n");WriteToClient(reply,clientSocket);}
        }

        while(1){
            ReadFromCLient();

            char cmdName[10]={'\0'};
            strncpy(cmdName,buffer,4);

            if(strcmp(cmdName, "PASS") == 0)
            {
                strcpy(reply, "230 User logged in\r\n");
                WriteToClient(reply,clientSocket);
            }
            else if(strcmp(cmdName,"QUIT") == 0)
            {
                strcpy(reply, "221 Goodbye.\r\n");
                WriteToClient(reply,clientSocket);
                return 0;
            }
            else if(strcmp(cmdName,"ABOR") == 0)
            {
                strcpy(reply, "221 Goodbye.\r\n");
                WriteToClient(reply,clientSocket);
                return 0;
            }
            else if(strcmp(cmdName,"SYST") == 0)
            {
                strcpy(reply, "215 UNIX Type: L8\r\n");
                WriteToClient(reply,clientSocket);
            }
            else if(strcmp(cmdName,"TYPE") == 0)
            {
                TYPE_handler(&buffer[5],reply);
            }
            else if(strcmp(cmdName,"PORT") == 0)
            {
                PORT_handler(&buffer[5],reply);
            }
            else if(strcmp(cmdName,"RETR") == 0)
            {
                RETR_handler(&buffer[5],reply);
            }
            else if(strcmp(cmdName,"PASV") == 0)
            {
                PASV_handler(reply);
            }
            else if(strcmp(cmdName,"STOR") == 0)
            {
                STOR_handler(&buffer[5], reply);
            }
            else if(strcmp(cmdName,"LIST") == 0)
            {
                LIST_handler(reply);
            }
            else if(strcmp(cmdName,"CWD ") == 0)
            {
                CWD_handler(&buffer[4],reply);
            }
            else if(strcmp(cmdName,"MKD ") == 0)
            {
                MKD_handler(&buffer[4],reply);
            }
            else if(strcmp(cmdName,"RMD ") == 0)
            {
                RMD_handler(&buffer[4],reply);
            }
            else if(strcmp(cmdName,"DELE") == 0)
            {
                DELE_handler(&buffer[5],reply);
            }
            else if(strcmp(cmdName,"RNFR") == 0)
            {
                RNFR_handler(&buffer[5],reply);
            }
            else if(strncmp(cmdName,"PWD",3) == 0)
            {
                PWD_handler(reply);
            }
        }
    }

}

void WriteToClient(char *reply,int clientSocket)
{
    write(clientSocket, reply, (int)strlen(reply));
    printf("%s %s", toClient, reply);
}

void ReadFromCLient(){
    Res = read(clientSocket,buffer,sizeof(buffer));
    if(Res == 0){
        exit(1);
    }
    buffer[Res - 2] = '\0';
    printf("bytes = %d,buffer is '%s'\n",Res,buffer);
}
void TYPE_handler(char *t,char *reply)
{
    switch (*t)
    {
        case 'A':
            TYPE=TYPE_A;
            strcpy(reply, "200 Type set to A.\r\n");
            break;
        case 'I':
            TYPE=TYPE_I;
            strcpy(reply, "200 Type set to I.\r\n");
            break;
        default:
            TYPE=TYPE_I;
            strcpy(reply, "200 Type set to I.\r\n");
            break;
    }
    WriteToClient(reply,clientSocket);
}

void PASV_handler(char *reply)
{
    struct sockaddr_in LocalDataAddr;
    LocalDataAddr.sin_family=AF_INET;
    LocalDataAddr.sin_addr.s_addr = serverAddr.sin_addr.s_addr;
    LocalDataAddr.sin_port = 0;

    int LocalDataSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(LocalDataSocket<0){
        strcpy(reply,"500 cannot find datasock\r\n");
        WriteToClient(reply,clientSocket);
        return;
    }

    if(bind(LocalDataSocket, (struct sockaddr *)&LocalDataAddr, sizeof LocalDataAddr)==-1){
        strcpy(reply,"500 cannot bind\r\n");
        WriteToClient(reply,clientSocket);
        return;
    }

    if(listen(LocalDataSocket,1)<0){
        strcpy(reply,"500 cannot listen\r\n");
        WriteToClient(reply,clientSocket);
        return;
    }

    struct sockaddr_in temp;
    int len = (int)(sizeof temp);
    getsockname(LocalDataSocket,(struct sockaddr *)&temp, (socklen_t *)&len);
    unsigned long port_num = ntohs(temp.sin_port);

    unsigned long a0,a1,a2,a3,p0,p1;
    sscanf(inet_ntoa(LocalDataAddr.sin_addr),"%lu.%lu.%lu.%lu",&a0,&a1,&a2,&a3);
    p0 = port_num>>8;
    p1 = port_num%256;
    sprintf(reply,"227 Entering Passive Mode(%lu,%lu,%lu,%lu,%lu,%lu)\r\n",a0,a1,a2,a3,p0,p1);
    WriteToClient(reply,clientSocket);

    dataSocket=accept(LocalDataSocket,(struct sockaddr *)&dataAddr,(socklen_t *)&dataAdrlen);
    if(dataSocket < 0){printf("Data accept failed\n");return;}
    else printf("Data Socket Accept %d\n",clientSocket);

    close(LocalDataSocket);
}

void PORT_handler(char *params,char *reply)
{
    unsigned long a0,a1,a2,a3,p0,p1,addr;
    sscanf(params,"%lu,%lu,%lu,%lu,%lu,%lu",&a0,&a1,&a2,&a3,&p0,&p1);
    addr = htonl((a0 << 24) + (a1 << 16) + (a2 << 8) + a3);

    memset(&dataAddr, 0, sizeof(dataAddr));
    dataAddr.sin_family = AF_INET;
    dataAddr.sin_addr.s_addr = addr;
    dataAddr.sin_port = htons((p0 << 8) + p1);

    dataSocket = socket(PF_INET,SOCK_STREAM,0);
    if(dataSocket == -1) {
        printf("failed find datasock\n");
        strcpy(reply,"500 cannot find datasock\r\n");
        WriteToClient(reply,clientSocket);
        return;
    }

    if(connect(dataSocket, (struct sockaddr *)&dataAddr, sizeof(dataAddr))<0){
        printf("failed connect\n");
        strcpy(reply,"500 cannot connect the data sock\r\n");
        WriteToClient(reply,clientSocket);
        return;
    }

    strcpy(reply, "200 PORT command successful\r\n");
    WriteToClient(reply,clientSocket);
}

void STOR_handler(char *filename, char *reply)
{
    FILE *filehandle;
    unsigned char databuf[FILEBUF_SIZE];
    int bytes,bytessum = 0;

    //no connection
    if(dataSocket == -1){
        strcpy(reply,"425 no dataSocket connected\r\n");
        WriteToClient(reply,clientSocket);
        close(dataSocket);
        return;
    }

    //record handle method
    struct stat sbuf;
    char method[10]="rewrite";
    if(stat(filename, &sbuf) == -1)strcpy(method,"create");

    //file handle error
    filehandle = fopen(filename, "wb");
    if(filehandle == 0)
    {
        strcpy(reply, "551 Cannot create the file\r\n");
        WriteToClient(reply,clientSocket);
        close(dataSocket);
        return;
    }

    //ready
    stpcpy(reply, "150 Handle creating or finding over\r\n");
    WriteToClient(reply,clientSocket);

    while((bytes = read(dataSocket, databuf, FILEBUF_SIZE)) > 0)
    {
        //disconenction
        if(dataSocket == -1){
            strcpy(reply,"426 dataSocket disconnected\r\n");
            WriteToClient(reply,clientSocket);
            close(dataSocket);
            return;
        }
        bytessum+=bytes;
        write(fileno(filehandle), databuf, bytes);
    }

    fclose(filehandle);
    close(dataSocket);

    sprintf(reply, "226-You have %s the file %s\n226-totally %d bytes get \n226 Transfer complete\r\n",method,filename,bytessum);
    WriteToClient(reply,clientSocket);
}


void RETR_handler(char *param,char *reply){

    FILE *filehandle;
    unsigned char databuf[FILEBUF_SIZE] = "";
    int bytes;

    filehandle = fopen(param,"rb");

    //no connection
    if(dataSocket == -1){
        strcpy(reply,"425 no dataSocket connected\r\n");
        WriteToClient(reply,clientSocket);
        close(dataSocket);
        return;
    }

    //openfile problem
    if(filehandle == 0)
    {
        strcpy(reply,"551 no such file or directory\r\n");
        WriteToClient(reply,clientSocket);
        close(dataSocket);
        return;
    }

    struct stat statbuf;
    stat(param,&statbuf);
    int size = statbuf.st_size;

    sprintf(reply, "150 Opening %s mode data connection for %s (%d bytes)\r\n", MODE[TYPE], &buffer[5],size);
    WriteToClient(reply,clientSocket);

    while((bytes = read(fileno(filehandle), databuf, FILEBUF_SIZE)) > 0)
    {
        //disconenction
        if(dataSocket == -1){
            strcpy(reply,"426 dataSocket disconnected\r\n");
            WriteToClient(reply,clientSocket);
            close(dataSocket);
            return;
        }
        if(TYPE == TYPE_I)write(dataSocket, (const char *)databuf, bytes);
        //clear the databuf
        memset(&databuf, 0, FILEBUF_SIZE);
    }
    memset(&databuf, 0, FILEBUF_SIZE);

    fclose(filehandle);
    close(dataSocket);
    strcpy(reply, "226 Transfer complete\r\n");
    WriteToClient(reply,clientSocket);
}

void ntorn_tail(char *info) {
    int n = strlen(info);
    int i = 0;
    for (i = 0; i < n; i++) {
        if (info[i] == '\n' && info[i-1] && info[i-1]!='\r') {
            int j;
            for (j = n-1 ; j>=i ;j--)
            {
                info[j+1] =  info[j];
            }
            info[i]='\r';
            n++;
        }
    }

}

void LIST_handler(char *reply) {
    FILE *dir;
    int dlen = 2*PIPE_BUF;
    char databuf[dlen];
    memset(databuf, 0, dlen - 1);

    dir = popen("ls -l", "rb");
    if (dir == 0) {
        stpcpy(reply, "500 Transfer error\r\n");
        WriteToClient(reply, clientSocket);
        return;
    }

    stpcpy(reply, "150 Transfer ready\r\n");
    WriteToClient(reply, clientSocket);

    int flag = 0;
    while (read(fileno(dir), databuf, PIPE_BUF) > 0) {
        //clear 总用量
        if(flag == 0){
            char *temp = strchr(databuf,'\n')+1;
            ntorn_tail(temp);
            printf("%s",temp);
            write(dataSocket, temp, strlen(temp));

            memset(databuf, 0, dlen - 1);
            flag++;
            continue;
        }
        //other situations
        ntorn_tail(databuf);
        write(dataSocket, databuf, strlen(databuf));
        memset(databuf, 0, dlen - 1);
    }
    memset(databuf, 0, dlen - 1);

    pclose(dir);
    close(dataSocket);
    stpcpy(reply, "226 Transfer complete\r\n");
    WriteToClient(reply, clientSocket);
}

void CWD_handler(char *dir,char *reply){
    if(chdir(dir) == -1 ){
        sprintf(reply, "550 no such directory %s\r\n",dir);
    }
    else sprintf(reply, "250 dir has changed into %s\r\n",dir);
    WriteToClient(reply, clientSocket);
}

void MKD_handler(char *dir,char *reply){
    if(mkdir(dir,0777) == -1 ){
        sprintf(reply, "550  created %s failed\r\n",dir);
    }
    else sprintf(reply, "250 dir has been created %s\r\n",dir);
    WriteToClient(reply, clientSocket);
}

int clean_directory_file(char *dir){
    char dirname[128];
    struct dirent *dp;
    struct stat dir_stat;
    DIR *dirp;

    if (access(dir, F_OK) != 0)return -1;
    if (stat(dir, &dir_stat)<0)return -1;

    if ( S_ISREG(dir_stat.st_mode) ) {
        remove(dir);
    } else if ( S_ISDIR(dir_stat.st_mode) ) {
        dirp = opendir(dir);
        while ( (dp=readdir(dirp)) != NULL ) {
            if ((strcmp(".", dp->d_name) == 0) || (strcmp("..", dp->d_name) == 0)) {
                continue;
            }
            sprintf(dirname,"%s/%s", dir, dp->d_name);
            if(clean_directory_file(dirname)==-1)return -1;
        }
        closedir(dirp);
        if(rmdir(dir) == -1)return -1;
    }

    return 0;
}

void RMD_handler(char *dir,char *reply){
    if(rmdir(dir) == -1){
        sprintf(reply, "550 remove %s failed\r\n",dir);
    }
    else sprintf(reply, "250 dir has been removed %s\r\n",dir);
    WriteToClient(reply, clientSocket);
}

void DELE_handler(char *dir,char *reply){
    if(clean_directory_file(dir) == -1){
        sprintf(reply, "550 remove %s failed\r\n",dir);
    }
    else sprintf(reply, "250 dir has been removed %s\r\n",dir);
    WriteToClient(reply, clientSocket);
}

void RNFR_handler(char *dir,char *reply){
    char *first,*second;
    sscanf(dir,"%s %s",first,second);
    if(rename(first,second)<0){
        sprintf(reply,"550 rename %s %s failed\r\n",first,second);
    }
    else strcpy(reply,"250 rename success\r\n");
    WriteToClient(reply,clientSocket);
}

void PWD_handler(char *reply){
    char pwd[512];
    getcwd(pwd,sizeof(pwd)-1);
    sprintf(reply,"%s\r\n",pwd);
    WriteToClient(reply,clientSocket);
    return;
}
