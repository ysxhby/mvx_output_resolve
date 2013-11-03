#include "define.h"


extern unsigned char  *m_buffer1;  //m_buffer1 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  ������1�Ĵ�СΪ4000����Ϣ
extern unsigned char  *m_buffer2;  //m_buffer2 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  ������2�Ĵ�СΪ4000����Ϣ
extern bool   m_buffer1_state;     //m_buffer1_state=0   ��ʾ������1Ϊ��, m_buffer1_state=1  ��ʾ������1Ϊ��
extern bool   m_buffer2_state;     //m_buffer2_state=0  ��ʾ������2Ϊ��, m_buffer1_state=1  ��ʾ������2Ϊ��
extern LONG   m_buffer1_pos;
extern LONG   m_buffer2_pos;



/*  ԭ����׷����Ϣ��ʽ

struct trace_entry {
	unsigned short type; //trace type    Ϲ��
	unsigned char flags; //trace flags   Ϲ��
	unsigned char preempt_count;      //   Ϲ��
	int pid;                            // CR3��ֵ32λ
};


struct ring_buffer_event {
	long long time;       //  ʱ��64λ
};


struct vmtrace_entry {
	struct trace_entry ent;     
	short ss;  //16λ
	int  esp;  //32λ
	unsigned char call_ret_flag; // 1��ʾCALL 0 ��ʾRET
	unsigned long ip;            //CALL �� RET �����IP��ַ32λ  
	unsigned long parent_ip;     //CALL RET ���ڵ�IP��ַ32λ   
};


struct output_entry{
	 struct ring_buffer_event  m_ring ;
     struct vmtrace_entry   m_vmtrace ;
};

*ԭ����׷����Ϣ��ʽ
*/


/*�µĶ����׷����Ϣ��ʽ
*ɾ����ԭ��û���õ�����Ϣ��ԭ����4���ṹ��ĳ�2���ṹ��
*/

struct entry_info
{
	long long time; 	  //  ʱ��64λ
	int pid;			  // CR3��ֵ32λ
	short ss;  //16λ
	int  esp;  //32λ
	unsigned char call_ret_flag; // 1��ʾCALL 0 ��ʾRET
	unsigned long ip;			 //CALL �� RET �����IP��ַ32λ   
	unsigned long parent_ip;	 //CALL RET ���ڵ�IP��ַ32λ   
	unsigned long serial_no;	 //��ʾ����׷����Ϣ�����кŴ�0��ʼ���� 
};

struct output_entry
{
  struct entry_info  m_entry_info;
  int odd_parity;  //m_entry_info��1�ĸ���Ϊż����odd_parity=0,m_entry_info��1�ĸ���Ϊ������odd_parity=1
};
















VOID m_bufferInit(void);
VOID m_bufferRelease(void);
