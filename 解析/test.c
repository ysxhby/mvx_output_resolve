#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct trace_entry {
unsigned short type; //trace type 瞎填
unsigned char flags; //trace flags 瞎填
unsigned char preempt_count; // 瞎填
int pid; // CR3的值32位
};

struct ring_buffer_event {
long long time; // 时间64位
};

struct vmtrace_entry {
struct trace_entry ent;
short ss; //16位
int esp; //32位
unsigned char call_ret_flag; // 1表示CALL 0 表示RET
unsigned long ip; //CALL 和 RET 跳向的IP地址32位 ？？？？
unsigned long parent_ip; //CALL RET 所在的IP地址32位 ？？？
};

typedef struct output_entry{
struct ring_buffer_event m_ring ;
struct vmtrace_entry m_vmtrace ;
}OE;


void replace(char* str) {
    if (NULL == str) {
        return;
    }
     
    while (*str != '\0') {
        if (*str == '\n') {
            *str = ' ';
        }
        *str++;
    }
    return;
}

void get_funcname(int ip,char* result)
{
    FILE *fstream=NULL;  
    char buff[40]="";      
    char cmd[50]="addr2line -e /home/cai/vmlinux -f 0x";
    char str[20];
    
    memset(result,0,sizeof(result)); 
    sprintf(str,"%x",ip);    
    strcat(cmd,str);

    if(NULL==(fstream=popen(cmd,"r")))       
    {      
        fprintf(stderr,"execute command failed: ");       
        return;       
    }      
    while(NULL!=fgets(buff, sizeof(buff), fstream))      
    {   
        strcat(result,buff);       
    }
    replace(result);
    //printf("%s\n",result);
    pclose(fstream);   
}

int main(int argc, char *argv[])
{
  OE out_ent;
  FILE *fp = NULL;
  //FILE *f2 = NULL;
  FILE *fllll = NULL;
  char* result_ip;
  char* result_pip;
  result_ip = (char*)malloc(sizeof(char)*100);
  result_pip = (char*)malloc(sizeof(char)*100);
  if(argc==3)
  	{
  	fp = fopen(argv[1], "rb");
  	fllll = fopen(argv[2],"wt");
        //f2 = fopen(argv[3],"wt");
  	}
  else
  	{
  	printf("wrong1\n");
  	return;
  	}

  fflush(fllll);
  while(!feof(fp))
  {
    fread(&out_ent, sizeof(out_ent), 1, fp);
    get_funcname(out_ent.m_vmtrace.ip,result_ip);
    get_funcname(out_ent.m_vmtrace.parent_ip,result_pip);

    
    fprintf(stdout,"TIME:0x%llx,", out_ent.m_ring.time);
    //printf("1\n");
    fprintf(stdout,"PID:0x%x,",out_ent.m_vmtrace.ent.pid);
    fprintf(stdout,"Flag:%d,",out_ent.m_vmtrace.call_ret_flag);
    fprintf(stdout,"CALL:%s,",result_pip);
    fprintf(stdout,"RETURN:%s\n",result_ip);

    fprintf(fllll,"TIME:0x%llx,", out_ent.m_ring.time);
    //printf("1\n");
    fprintf(fllll,"PID:0x%x,",out_ent.m_vmtrace.ent.pid);
    fprintf(fllll,"Flag:%d,",out_ent.m_vmtrace.call_ret_flag);
    fprintf(fllll,"CALL:%s,",result_pip);
    fprintf(fllll,"RETURN:%s\n",result_ip);
/*
    fprintf(f2,"TIME:0x%llx,", out_ent.m_ring.time);
    //printf("1\n");
    fprintf(f2,"PID:0x%x,",out_ent.m_vmtrace.ent.pid);
    fprintf(f2,"Flag:%d,",out_ent.m_vmtrace.call_ret_flag);
    fprintf(f2,"CALL:%s,",result_pip);
    fprintf(f2,"RETURN:%s\n",result_ip);
*/
/*
    printf("TIME:0x%llx,", out_ent.m_ring.time);
    //printf("1\n");
    printf("PID:0x%x,",out_ent.m_vmtrace.ent.pid);
    printf("Flag:%d,",out_ent.m_vmtrace.call_ret_flag);
    printf("CALL:%s,",result_pip);
    printf("RETURN:%s\n,",result_ip);
*/
  }
    
  free(result_pip);
  free(result_ip);
 // fclose(f2);
  fclose(fp);
  fclose(fllll);
 
  return 0;
}
