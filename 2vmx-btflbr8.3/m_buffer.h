#include "define.h"


extern unsigned char  *m_buffer1;  //m_buffer1 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  缓冲区1的大小为4000条信息
extern unsigned char  *m_buffer2;  //m_buffer2 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  缓冲区2的大小为4000条信息
extern bool   m_buffer1_state;     //m_buffer1_state=0   表示缓冲区1为空, m_buffer1_state=1  表示缓冲区1为满
extern bool   m_buffer2_state;     //m_buffer2_state=0  表示缓冲区2为空, m_buffer1_state=1  表示缓冲区2为满
extern LONG   m_buffer1_pos;
extern LONG   m_buffer2_pos;



/*  原来的追踪信息格式

struct trace_entry {
	unsigned short type; //trace type    瞎填
	unsigned char flags; //trace flags   瞎填
	unsigned char preempt_count;      //   瞎填
	int pid;                            // CR3的值32位
};


struct ring_buffer_event {
	long long time;       //  时间64位
};


struct vmtrace_entry {
	struct trace_entry ent;     
	short ss;  //16位
	int  esp;  //32位
	unsigned char call_ret_flag; // 1表示CALL 0 表示RET
	unsigned long ip;            //CALL 和 RET 跳向的IP地址32位  
	unsigned long parent_ip;     //CALL RET 所在的IP地址32位   
};


struct output_entry{
	 struct ring_buffer_event  m_ring ;
     struct vmtrace_entry   m_vmtrace ;
};

*原来的追踪信息格式
*/


/*新的定义的追踪信息格式
*删除了原来没有用到的信息把原来的4个结构体改成2个结构体
*/

struct entry_info
{
	long long time; 	  //  时间64位
	int pid;			  // CR3的值32位
	short ss;  //16位
	int  esp;  //32位
	unsigned char call_ret_flag; // 1表示CALL 0 表示RET
	unsigned long ip;			 //CALL 和 RET 跳向的IP地址32位   
	unsigned long parent_ip;	 //CALL RET 所在的IP地址32位   
	unsigned long serial_no;	 //表示此条追踪信息的序列号从0开始递增 
};

struct output_entry
{
  struct entry_info  m_entry_info;
  int odd_parity;  //m_entry_info中1的个数为偶数则odd_parity=0,m_entry_info中1的个数为奇数则odd_parity=1
};
















VOID m_bufferInit(void);
VOID m_bufferRelease(void);
