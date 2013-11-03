#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include "define.h"
#include "vmx.h"
#include "vmm.h"
#include "m_buffer.h"
#include "video.h"
#include "dbgprint.h"
#include "vmxice_extract.h"
extern LARGE_INTEGER SpecCode;

BOOLEAN asmlinkage StartVirtualization(PVOID pGuestEsp)
{
    BOOLEAN bRet;
    PGUEST_CPU pCpu;

    pCpu = (PGUEST_CPU)kmalloc(sizeof(GUEST_CPU), GFP_ATOMIC);
    memset(pCpu,0,sizeof(GUEST_CPU));
    if (!pCpu)
    {
        RED_FONT;
        printf("can't allocate cpu structure!\r\n");
        goto failexit;
    }

    GREEN_FONT;
    printf("SetupVMX...\r\n");
    bRet = SetupVMX(pCpu);
    if (bRet == false)
    {
        RED_FONT;
        printf("SetupVMX Failed!\r\n");
        goto failexit;
    }
    printf("\r\n");

    GREEN_FONT;
    printf("SetupVMCS...\r\n");
    bRet = SetupVMCS(pCpu, pGuestEsp);
    if (bRet == false)
    {
        RED_FONT;
        printf("SetupVMCS Failed!\r\n");
        goto failexit;
    }
    printf("\r\n");

    GREEN_FONT;
    printf("Virtualize...\r\n");
    bRet = Virtualize(pCpu);

failexit:
    DEFAULT_FONT;
    return false;
}

static int vmxice_init(void)
{
    BOOLEAN bRet;

    YELLOW_FONT;
    printf("VMXICE for linux\r\n");
    printf("MengXP Works 2011-2012 @ XUPT\r\n\r\n");
    DEFAULT_FONT;
    printf("%d\r\n",sizeof(struct output_entry));
  
	m_bufferInit();
    VideoInit();
    udp_send_init(&m_buffer1_state,&m_buffer2_state, m_buffer1, m_buffer2);
    SpecCode.QuadPart = _TSC();

    bRet = CheckForVirtualizationSupport();
    if (bRet == false)
    {
        RED_FONT;
        printf("Not support VT-x!\r\n");
        goto init_exit;
    }

    bRet = CheckIfVMXIsEnabled();
    if (bRet == false)
    {
        RED_FONT;
        printf("BIOS Set VMX Disabled!\r\n");
        goto init_exit;
    }

    bRet = _StartVirtualization();
    if(bRet)
    {
        GREEN_FONT;
        printf("Virtualize successfully!\r\n");
        printf("Press F12 to call VMXICE!\r\n");
    }
    else
    {
        RED_FONT;
        printf("Virtualize failed!\r\n");
    }

init_exit:
    DEFAULT_FONT;
    return 0;
}

static void vmxice_exit(void)
{
//    mm_segment_t old_fs;
//    struct file *fp = NULL;
//
//    fp = filp_open("/home/screen.bmp",O_RDWR | O_CREAT, 0644);
//    if(fp)
//    {
//        old_fs = get_fs();
//        set_fs(get_ds());
//		fp->f_op->write(fp,m_buffer1,4000*36,&fp->f_pos);
//		fp->f_op->write(fp,m_buffer2,4000*36,&fp->f_pos);
//		set_fs(old_fs);
//        filp_close(fp,NULL);
//    }
//    else
//    {
//        printf("Can't open ~/screen.bmp\r\n");
//    }
    _StopVirtualization(SpecCode.LowPart,SpecCode.HighPart);
    m_bufferRelease();
	VideoRelease();

}

module_init(vmxice_init);
module_exit(vmxice_exit);
