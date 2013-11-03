#include <linux/slab.h>
#include <linux/module.h>
#include "m_buffer.h"




/*elwin*/


unsigned char  *m_buffer1;  //m_buffer1 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  ������1�Ĵ�СΪ4000����Ϣ
unsigned char  *m_buffer2;  //m_buffer2 = (unsigned char *)kmalloc(4000*36,GFP_ATOMIC);  ������2�Ĵ�СΪ4000����Ϣ
bool   m_buffer1_state;     //m_buffer1_state=0   ��ʾ������1Ϊ��, m_buffer1_state=1  ��ʾ������1Ϊ��
bool   m_buffer2_state;     //m_buffer2_state=0  ��ʾ������2Ϊ��, m_buffer1_state=1  ��ʾ������2Ϊ��
LONG   m_buffer1_pos;
LONG   m_buffer2_pos;










VOID m_bufferInit(void)
{

	/*elwin*/
    m_buffer1 = (PUCHAR)kmalloc(4000*sizeof(struct output_entry),GFP_ATOMIC);
    m_buffer2 = (PUCHAR)kmalloc(4000*sizeof(struct output_entry),GFP_ATOMIC);	
	m_buffer1_pos=0;
	m_buffer2_pos=0;
	m_buffer1_state=0;
	m_buffer2_state=0;
	/*elwin*/
}



VOID m_bufferRelease(void)
{
	/*elwin*/
	if(m_buffer1)
        kfree(m_buffer1);
	if(m_buffer2)
        kfree(m_buffer2);	
}

