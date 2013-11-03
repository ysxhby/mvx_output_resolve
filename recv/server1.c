#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#define Size 4000*36

int main(int argc, char **argv)
{
    int sockfd;
    FILE *fp;
    char *path = "/home/cai/test/";
    char filename[25];
    long count = 0;
    long len;
    struct sockaddr_in servaddr;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    int n;
    char * recvline = NULL;
    recvline = (char *)malloc(Size * sizeof(char));
    memset(recvline, 0 , Size);
    

    while(1)
    {
        len = recvfrom(sockfd, recvline, Size, 0, NULL, NULL);
       
        sprintf(filename,"%s%ld",path,count);
        printf("%s\n",filename);
        fp=fopen(filename,"w+");
        fwrite(recvline,sizeof( unsigned char ),len,fp);
        fclose(fp);
        count++;
    }
    close(sockfd);
    
}
