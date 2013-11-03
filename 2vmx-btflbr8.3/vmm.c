#include <linux/io.h>
#include <linux/sched.h>
#include "vmx.h"
#include "vmm.h"
#include "x86.h"
#include "m_buffer.h"
#include "video.h"


LARGE_INTEGER SpecCode;
unsigned int VmmCount;
unsigned int OutPutCount=0; /*elwin*/
unsigned int MissCount=0; /*elwin*/


//unwelcome vmexit :(
VOID HandleUnimplemented(PGUEST_CPU pCpu, ULONG ExitCode)
{
    ULONG InstructionLength;
    UCHAR str[128];
    ULONG IInfo,AState;

    IInfo = _ReadVMCS(GUEST_INTERRUPTIBILITY_INFO);
    AState = _ReadVMCS(GUEST_ACTIVITY_STATE);
    snprintf(str,128,"What the fuck?! VMExit Reason %d\n",ExitCode & 0xFF);
   //elwin *  ConsolePrintStr(str,0xC,1);
    snprintf(str,128,"EIP: %08X   ESP: %08X\n",_ReadVMCS(GUEST_RIP),_ReadVMCS(GUEST_RSP));
    //elwin * ConsolePrintStr(str,0xE,0);
    snprintf(str,128,"CR0: %08X   CR3:%08X   CR4:%08X\n",_ReadVMCS(GUEST_CR0),_ReadVMCS(GUEST_CR3),_ReadVMCS(GUEST_CR4));
    //elwin * ConsolePrintStr(str,0xE,0);
    snprintf(str,128,"GUEST_INTERRUPTIBILITY_INFO = %08X   GUEST_ACTIVITY_STATE = %08X\n",IInfo,AState);
   //elwin *  ConsolePrintStr(str,0xE,0);
    //elwin * ConsolePrintCurrentPage();

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleCpuid(PGUEST_CPU pCpu)
{
    ULONG Function;
    ULONG InstructionLength;

    Function = pCpu->pGuestRegs->eax;
    _CpuId(&pCpu->pGuestRegs->eax, &pCpu->pGuestRegs->ebx, &pCpu->pGuestRegs->ecx, &pCpu->pGuestRegs->edx);
    if(Function == 1)
    {
        pCpu->pGuestRegs->ecx &= ~0x20;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleInvd(PGUEST_CPU pCpu)
{
    ULONG InstructionLength;

    _Invd();
    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}



VOID HandleVmCall(PGUEST_CPU pCpu)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    ULONG InstructionLength, Eip, Esp;

    Eip = _ReadVMCS(GUEST_RIP);

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);

    if ((pCpu->pGuestRegs->eax == SpecCode.LowPart) && (pCpu->pGuestRegs->edx == SpecCode.HighPart))
    {
        //elwin * CmdClearBreakpoint(pCpu,"bc *");
         //elwin * ClearStepBps();
        Eip = (ULONG)_GuestExit;
        Esp = pCpu->pGuestRegs->esp;
        _VmxOff(Esp,Eip);
    }
    else if(pCpu->pGuestRegs->eax == 0xdeadc0de)
    {
       //elwin *  FillScreen();
        InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
        _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
    }
    else
    {
        InjectEvent = 0;
        pInjectEvent->Vector = INVALID_OPCODE_EXCEPTION;
        pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
        pInjectEvent->DeliverErrorCode = 0;
        pInjectEvent->Valid = 1;
        _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
    }
}

VOID HandleVmInstruction(PGUEST_CPU pCpu)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

    InjectEvent = 0;
    pInjectEvent->Vector = INVALID_OPCODE_EXCEPTION;
    pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
    pInjectEvent->DeliverErrorCode = 0;
    pInjectEvent->Valid = 1;
    _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
}

VOID HandleMsrRead(PGUEST_CPU pCpu)
{
    LARGE_INTEGER Msr;
    ULONG ecx;
    ULONG InstructionLength;


    ecx = pCpu->pGuestRegs->ecx;
    Msr.QuadPart = _ReadMSR(ecx);
    pCpu->pGuestRegs->eax = Msr.LowPart;
    pCpu->pGuestRegs->edx = Msr.HighPart;

    if(ecx == MSR_IA32_FEATURE_CONTROL)
    {
        pCpu->pGuestRegs->eax &= ~4;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleMsrWrite(PGUEST_CPU pCpu)
{
    ULONG InstructionLength;

    _WriteMSR(pCpu->pGuestRegs->ecx,pCpu->pGuestRegs->eax,pCpu->pGuestRegs->edx);
    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

ULONG AttachProcess(ULONG TargetCR3)
{
    PULONG vTargetCR3 = phys_to_virt(TargetCR3);
    PULONG vHostCR3 = phys_to_virt(_CR3());
    ULONG i;
    UCHAR str[128];

    if(TargetCR3 > 892*1024*1024)
        return 0;

    memset(vHostCR3,0,0xC00);
    for(i = 0; i < 1024; i++)
    {
        if(!*(vHostCR3+i))
        {
            *(vHostCR3+i) = *(vTargetCR3+i);
        }
        else
        {
            if(*(vTargetCR3+i) && *(vHostCR3+i) != *(vTargetCR3+i))
            {
                sprintf(str,"TargetCR3:%08x  Index:%d  Host:[%08x]  Guest:[%08x]\n",TargetCR3,i,*(vHostCR3+i),*(vTargetCR3+i));
                PrintStr(0,0,7,1,FALSE,str,TRUE);
            }
        }
    }
    _FlushTLB();
    return 1;
}

VOID HandleCrAccess(PGUEST_CPU pCpu)
{
    PMOV_CR_QUALIFICATION pExitQualification;
    ULONG Exit;
    ULONG Cr = 0;
    ULONG Reg = 0;
    ULONG InstructionLength;
	UCHAR str[128]; /*elwin*/

    Exit =_ReadVMCS(EXIT_QUALIFICATION);
    pExitQualification = (PMOV_CR_QUALIFICATION)&Exit;

    switch (pExitQualification->ControlRegister)
    {
    case 0:
        Cr = _ReadVMCS(GUEST_CR0);
        break;

    case 3:
        Cr = _ReadVMCS(GUEST_CR3);
        break;

    case 4:
        Cr = _ReadVMCS(GUEST_CR4);
        break;

    default:
        //what the fuck?
        break;
    }

    switch (pExitQualification->Register)
    {
    case 0:
        Reg = pCpu->pGuestRegs->eax;
        break;

    case 1:
        Reg = pCpu->pGuestRegs->ecx;
        break;

    case 2:
        Reg = pCpu->pGuestRegs->edx;
        break;

    case 3:
        Reg = pCpu->pGuestRegs->ebx;
        break;

    case 4:
        Reg = pCpu->pGuestRegs->esp;
        break;

    case 5:
        Reg = pCpu->pGuestRegs->ebp;
        break;

    case 6:
        Reg = pCpu->pGuestRegs->esi;
        break;

    case 7:
        Reg = pCpu->pGuestRegs->edi;
        break;

    default:
        //what the fuck?
        break;
    }

    switch (pExitQualification->AccessType)
    {
    case 0://MOV_TO_CR
        switch (pExitQualification->ControlRegister)
        {
        case 0:
            _WriteVMCS(GUEST_CR0, Reg);
            break;

        case 3:
            _WriteVMCS(GUEST_CR3, Reg);

            //AttachGuestProcess();
 /*elwin*/ 

		    snprintf(str,128,"it is from cr3 %d\n",VmmCount);/*elwin*/
			PrintStr(0,18,7,1,FALSE,str,TRUE);/*elwin*/

			
			/*
            if(bDebuggerBreak)
            {
                ClearCurrentDisasm();
                RefreshOldRegister(pCpu);
                EnterHyperDebugger(pCpu);
            }
                  */
            break;
 /*elwin*/

        case 4:
            _WriteVMCS(GUEST_CR4, Reg);
            break;

        default:
            break;
        }
        break;

    case 1://MOV_FROM_CR
        switch (pExitQualification->Register)
        {
        case 0:
            pCpu->pGuestRegs->eax = Cr;
            break;


        case 1:
            pCpu->pGuestRegs->ecx = Cr;
            break;

        case 2:
            pCpu->pGuestRegs->edx = Cr;
            break;

        case 3:
            pCpu->pGuestRegs->ebx = Cr;
            break;

        case 4:
            pCpu->pGuestRegs->esp = Cr;
            break;

        case 5:
            pCpu->pGuestRegs->ebp = Cr;
            break;

        case 6:
            pCpu->pGuestRegs->esi = Cr;
            break;

        case 7:
            pCpu->pGuestRegs->edi = Cr;
            break;

        default:
            break;
        }

        break;

    default:
        break;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID InjectPageFault(PGUEST_CPU pCpu,ULONG MemoryAddress)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

    _SetCR2(MemoryAddress);
    _WriteVMCS(VM_ENTRY_EXCEPTION_ERROR_CODE, 0);
    InjectEvent = 0;
    pInjectEvent->Vector = PAGE_FAULT_EXCEPTION;
    pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
    pInjectEvent->DeliverErrorCode = 1;
    pInjectEvent->Valid = 1;
    _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP));
}



VOID HandleDebugException(PGUEST_CPU pCpu)
{
    UCHAR  str[128]; /*elwin*/
    ULONG64 ExitQualification;
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    ULONG GuestEip = _ReadVMCS(GUEST_RIP);
    ULONG Eflags;
	ULONG InstructionLength;


/*elwin*/

   snprintf(str,128,"chuli zhong duan %d",VmmCount); 
	 PrintStr(0,32,7,1,FALSE,str,TRUE); 
      		 snprintf(str,128,"3misscount: %d",MissCount);
		 PrintStr(0,10,7,1,FALSE,str,TRUE);	

 
/*elwin*/



/*elwin*/
ULONG64 ttime;
ttime=_TSC();


ULONG cr3_pid = _ReadVMCS(GUEST_CR3);
ULONG gs_ss=  _ReadVMCS(GUEST_SS_SELECTOR);
ULONG gs_sp= _ReadVMCS(GUEST_RSP);




ULONG64 tos;
ULONG64 fromip;
ULONG64 toip;
tos=_ReadMSR(MSR_LASTBRANCH_TOS);
ULONG utos=tos;
ULONG ufromip;
ULONG utoip;
fromip=_ReadMSR(utos + MSR_LASTBRANCH_0_FROM_IP);
toip=_ReadMSR(utos + MSR_LASTBRANCH_0_TO_IP);

ufromip=fromip;
utoip=toip;

struct output_entry  m_output;



if(!m_buffer1_state)
{

    //Ð´Èëm_buffer1
	if(*(PUCHAR)ufromip==0xE8 || *(PUCHAR)ufromip==0xFF || *(PUCHAR)ufromip==0x9a)
	{
		 snprintf(str,128,"CALL: %d",OutPutCount);
		 PrintStr(0,1,7,1,FALSE,str,TRUE);
	
	
		 m_output.m_ring.time =ttime;
		 
		 m_output.m_vmtrace.ent.type=0;
		 m_output.m_vmtrace.ent.flags=0; 
		 m_output.m_vmtrace.ent.preempt_count=0;
	
		 m_output.m_vmtrace.ent.pid=cr3_pid;
		 m_output.m_vmtrace.ss=(USHORT)gs_ss;
		 m_output.m_vmtrace.esp=gs_sp;
		 m_output.m_vmtrace.call_ret_flag=1;
		 m_output.m_vmtrace.ip = utoip;
		 m_output.m_vmtrace.parent_ip = ufromip;
	
	
		memcpy(m_buffer1+m_buffer1_pos,&m_output,sizeof(m_output));
	    m_buffer1_pos+=sizeof(m_output);
		if(m_buffer1_pos==4000*36)
			{
			  m_buffer1_pos%=4000*36;
			  m_buffer1_state =1;
			}
 		OutPutCount++;

	
	
	}
	
	if(*(PUCHAR)ufromip==0xc3 || *(PUCHAR)ufromip==0xc2 || *(PUCHAR)ufromip==0xcb || *(PUCHAR)ufromip==0xca)
	{
		snprintf(str,128,"RET: %d",OutPutCount);
		PrintStr(0,5,7,1,FALSE,str,TRUE);
		 m_output.m_ring.time =ttime;
		 
		 m_output.m_vmtrace.ent.type=0;
		 m_output.m_vmtrace.ent.flags=0; 
		 m_output.m_vmtrace.ent.preempt_count=0;
	
		 m_output.m_vmtrace.ent.pid=cr3_pid;
		 m_output.m_vmtrace.ss=(USHORT)gs_ss;
		 m_output.m_vmtrace.esp=gs_sp;
		 m_output.m_vmtrace.call_ret_flag=0;
		 m_output.m_vmtrace.ip = utoip;
		 m_output.m_vmtrace.parent_ip = ufromip;
	
	


		memcpy(m_buffer1+m_buffer1_pos,&m_output,sizeof(m_output));

	    m_buffer1_pos+=sizeof(m_output);
		if(m_buffer1_pos==4000*36)
			{
			  m_buffer1_pos%=4000*36;
			  m_buffer1_state =1;
			}
 		OutPutCount++;


		 
	
	}
	/*elwin*/
    
   
}
else if(!m_buffer2_state)
{
    //Ð´Èëm_buffer2
	if(*(PUCHAR)ufromip==0xE8 || *(PUCHAR)ufromip==0xFF || *(PUCHAR)ufromip==0x9a)
	{
		 snprintf(str,128,"CALL:%d",OutPutCount);
		 PrintStr(0,1,7,1,FALSE,str,TRUE);
	
	
		 m_output.m_ring.time =ttime;
		 
		 m_output.m_vmtrace.ent.type=0;
		 m_output.m_vmtrace.ent.flags=0; 
		 m_output.m_vmtrace.ent.preempt_count=0;
	
		 m_output.m_vmtrace.ent.pid=cr3_pid;
		 m_output.m_vmtrace.ss=(USHORT)gs_ss;
		 m_output.m_vmtrace.esp=gs_sp;
		 m_output.m_vmtrace.call_ret_flag=1;
		 m_output.m_vmtrace.ip = utoip;
		 m_output.m_vmtrace.parent_ip = ufromip;
	
	


		 memcpy(m_buffer2+m_buffer2_pos,&m_output,sizeof(m_output));
	    m_buffer2_pos+=sizeof(m_output);
		if(m_buffer2_pos==4000*36)
			{
			  m_buffer2_pos%=4000*36;
			  m_buffer2_state =1;
			}
 		OutPutCount++;

	
	
	}
	
	if(*(PUCHAR)ufromip==0xc3 || *(PUCHAR)ufromip==0xc2 || *(PUCHAR)ufromip==0xcb || *(PUCHAR)ufromip==0xca)
	{
		snprintf(str,128,"RET: %d",OutPutCount);
		PrintStr(0,5,7,1,FALSE,str,TRUE);
		 m_output.m_ring.time =ttime;
		 
		 m_output.m_vmtrace.ent.type=0;
		 m_output.m_vmtrace.ent.flags=0; 
		 m_output.m_vmtrace.ent.preempt_count=0;
	
		 m_output.m_vmtrace.ent.pid=cr3_pid;
		 m_output.m_vmtrace.ss=(USHORT)gs_ss;
		 m_output.m_vmtrace.esp=gs_sp;
		 m_output.m_vmtrace.call_ret_flag=0;
		 m_output.m_vmtrace.ip = utoip;
		 m_output.m_vmtrace.parent_ip = ufromip;
	

		memcpy(m_buffer2+m_buffer2_pos,&m_output,sizeof(m_output));
	    m_buffer2_pos+=sizeof(m_output);
		if(m_buffer2_pos==4000*36)
			{
			  m_buffer2_pos%=4000*36;
			  m_buffer2_state =1;
			}
 		OutPutCount++;

	}
	/*elwin*/
        
}

else
	{
		 snprintf(str,128,"2misscount: %d",MissCount);
		 PrintStr(0,9,7,1,FALSE,str,TRUE); 
		if(*(PUCHAR)ufromip==0xE8 || *(PUCHAR)ufromip==0xFF || *(PUCHAR)ufromip==0x9a||*(PUCHAR)ufromip==0xc3 || *(PUCHAR)ufromip==0xc2 || *(PUCHAR)ufromip==0xcb || *(PUCHAR)ufromip==0xca)
		 {
		 MissCount++;
		 snprintf(str,128,"misscount: %d",MissCount);
		 PrintStr(0,8,7,1,FALSE,str,TRUE);	
		 }
	}
	

 
}

VOID HandleException(PGUEST_CPU pCpu)
{
	  UCHAR str[128]; /*elwin*/
    ULONG Event, InjectEvent;
    ULONG64 ErrorCode, ExitQualification;
    PINTERRUPT_INFO_FIELD pEvent;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent;

    Event = (ULONG)_ReadVMCS(VM_EXIT_INTR_INFO);
    pEvent = (PINTERRUPT_INFO_FIELD)&Event;

    InjectEvent = 0;
    pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

    switch (pEvent->InterruptionType)
    {
    case NMI_INTERRUPT:
        InjectEvent = 0;
        pInjectEvent->Vector = NMI_INTERRUPT;
        pInjectEvent->InterruptionType = NMI_INTERRUPT;
        pInjectEvent->DeliverErrorCode = 0;
        pInjectEvent->Valid = 1;
        _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
        break;

    case EXTERNAL_INTERRUPT:
        break;

    case HARDWARE_EXCEPTION:
        switch (pEvent->Vector)
        {
        case DEBUG_EXCEPTION:
        	  snprintf(str,128,"%d chan shen debug type %d..\n",VmmCount, (pEvent->Vector));/*elwin*/
           PrintStr(0,22,7,1,FALSE,str,TRUE);/*elwin*/
            HandleDebugException(pCpu);
            break;

        case PAGE_FAULT_EXCEPTION:
            ErrorCode = _ReadVMCS(VM_EXIT_INTR_ERROR_CODE);
            ExitQualification = _ReadVMCS(EXIT_QUALIFICATION);
            _SetCR2(ExitQualification);
            _WriteVMCS(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
            InjectEvent = 0;
            pInjectEvent->Vector = PAGE_FAULT_EXCEPTION;
            pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
            pInjectEvent->DeliverErrorCode = 1;
            pInjectEvent->Valid = 1;
            _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
            _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP));
            break;

        default:
            break;
        }

        break;

    case SOFTWARE_EXCEPTION:
        /* #BP (int3) and #OF (into) */

        switch (pEvent->Vector)
        {
        case BREAKPOINT_EXCEPTION:
   /*elwin*/
  			 snprintf(str,128,"%d chan shen breakpoint type %d..\n",VmmCount, (pEvent->Vector));/*elwin*/
			 PrintStr(0,23,7,1,FALSE,str,TRUE);/*elwin*/

   /*elwin*/
			
           // HandleBreakpointException(pCpu);
            break;

        case OVERFLOW_EXCEPTION:
        default:
            break;
        }
        break;

    default:
        break;
    }
}

VOID asmlinkage VmExitHandler(PGUEST_CPU pCpu)
{
    ULONG ExitCode;
	/*elwin*/
    UCHAR str[128];
    ULONG mHostip;
	ULONG mGestip;
	ULONG Eflags;
	ULONG64 msr;
    /*elwin*/
	
    _RegSetIdtr(_ReadVMCS(HOST_IDTR_BASE),0x7FF);
    _RegSetGdtr(_ReadVMCS(HOST_GDTR_BASE),0x3FF);
    pCpu->pGuestRegs->esp = _ReadVMCS(GUEST_RSP);
    ExitCode = _ReadVMCS(VM_EXIT_REASON);

/*elwin*/
//    snprintf(str,128,"[%d]VmExitHandler ExitCode %d..\n",VmmCount,ExitCode);
//    PrintStr(0,ExitCode,7,1,FALSE,str,TRUE);
    mHostip=_ReadVMCS(HOST_RIP);
    snprintf(str,128,"%d hostip is %x:\n",VmmCount, mHostip);
    PrintStr(0,28,7,1,FALSE,str,TRUE);
   // if(VmmCount== 200)
    	{
     //    mGostip=_ReadVMCS(GUEST_RIP);
	   //  *(PUCHAR)mGestip=0xCC;
    	}
   if(VmmCount==20)
   {
       Eflags = _ReadVMCS(GUEST_RFLAGS);
	   Eflags |= FLAGS_TF_MASK;
	  _WriteVMCS(GUEST_RFLAGS,Eflags);

	   msr = _ReadMSR(MSR_IA32_DEBUGCTL);
       _WriteVMCS(GUEST_IA32_DEBUGCTL, (ULONG)((msr & 0xFFFFFFFF)|0x00000003)); /*elwin*/
   
   }


	
	
/*elwin*/
    AttachProcess(_ReadVMCS(GUEST_CR3));

    switch (ExitCode)
    {
    case EXIT_REASON_EXCEPTION_NMI:
        HandleException(pCpu);
        break;

    case EXIT_REASON_EXTERNAL_INTERRUPT:
    case EXIT_REASON_TRIPLE_FAULT:
    case EXIT_REASON_INIT:
    case EXIT_REASON_SIPI:
    case EXIT_REASON_IO_SMI:
    case EXIT_REASON_OTHER_SMI:
    case EXIT_REASON_PENDING_INTERRUPT:
    case EXIT_REASON_TASK_SWITCH:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_CPUID:
        HandleCpuid(pCpu);
        break;

    case EXIT_REASON_HLT:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_INVD:
        HandleInvd(pCpu);
        break;

    case EXIT_REASON_INVLPG:
    case EXIT_REASON_RDPMC:
    case EXIT_REASON_RDTSC:
    case EXIT_REASON_RSM:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_VMCALL:
        HandleVmCall(pCpu);
        break;

    case EXIT_REASON_VMCLEAR:
    case EXIT_REASON_VMLAUNCH:
    case EXIT_REASON_VMPTRLD:
    case EXIT_REASON_VMPTRST:
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMRESUME:
    case EXIT_REASON_VMWRITE:
    case EXIT_REASON_VMXOFF:
    case EXIT_REASON_VMXON:
        HandleVmInstruction(pCpu);
        break;

    case EXIT_REASON_CR_ACCESS:
        HandleCrAccess(pCpu);
        break;

    case EXIT_REASON_DR_ACCESS:
        HandleUnimplemented(pCpu, ExitCode);
        break;
    case EXIT_REASON_IO_INSTRUCTION:
      //  HandleIoAccess(pCpu);  //elwin
        break;

    case EXIT_REASON_MSR_READ:
        HandleMsrRead(pCpu);
        break;

    case EXIT_REASON_MSR_WRITE:
        HandleMsrWrite(pCpu);
        break;

    case EXIT_REASON_INVALID_GUEST_STATE:
    case EXIT_REASON_MSR_LOADING:
    case EXIT_REASON_MWAIT_INSTRUCTION:
    case EXIT_REASON_MONITOR_INSTRUCTION:
    case EXIT_REASON_PAUSE_INSTRUCTION:
    case EXIT_REASON_MACHINE_CHECK:
    case EXIT_REASON_TPR_BELOW_THRESHOLD:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    default:
        HandleUnimplemented(pCpu, ExitCode);
        break;
    }
    _WriteVMCS(GUEST_RSP, pCpu->pGuestRegs->esp);

//    snprintf(str,128,"[%d]VmExitHandler ExitCode %d..ok\n",VmmCount,ExitCode);
//    PrintStr(0,ExitCode,7,1,FALSE,str,TRUE);

	/*elwin*/
	//	  snprintf(str,128,"[%d]VmExitHandler ExitCode %d..\n",VmmCount,ExitCode);
	//	  PrintStr(0,ExitCode,7,1,FALSE,str,TRUE);
		mHostip=_ReadVMCS(HOST_RIP);
		snprintf(str,128,"%d wanchen is %x:\n",VmmCount, mHostip);
		PrintStr(0,30,7,1,FALSE,str,TRUE);
	/*elwin*/

    VmmCount++;
}
