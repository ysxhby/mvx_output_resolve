obj-m := vmxice.o
vmxice-objs := main.o vmx-x86.o vmx.o vmm.o m_buffer.o video.o dbgprint.o vmxice_extract.o

CURRENT_PATH := $(shell pwd)
LINUX_KERNEL := $(shell uname -r)
LINUX_KERNEL_PATH := /root/new/linux-$(LINUX_KERNEL)


all:
	yasm -felf vmx-x86.asm
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules

clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean

