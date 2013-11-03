#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/inetdevice.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#include <linux/zlib.h>
#include "video.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ysx");
char *ifname = "eth0";
module_param(ifname, charp, 0644);
MODULE_PARM_DESC(ifname, "Send packets from which net device");

char *buffer = "test from kernel!\n";
module_param(buffer, charp, 0644);
MODULE_PARM_DESC(buffer, "Packet content");

__u32 dstip = 0x0a6c1883;
//__u32 dstip = 0xc0a80056;
module_param(dstip, uint, 0644);

__s16 dstport = 8000;
module_param(dstport, short, 0644);

long timeout = 1;
module_param(timeout, long, 0644);
MODULE_PARM_DESC(timeout, "Interval between send packets, default 1(unit second)");

static struct task_struct *kthreadtask = NULL;

#define DATA_PAGE_SIZE 4000*36
#define OUT_BUF_SIZE 4000*40

void *buf[2];
int *id0;
int *id1;
int out_buf_size;
void *out_buf;



static int bind_to_device(struct socket *sock, char *ifname)//get local ip,bind with socket
{
    struct net *net;
    struct net_device *dev;
    __be32 addr;
    struct sockaddr_in sin;
    int err;
    net = sock_net(sock->sk);
    dev = __dev_get_by_name(net, ifname);

    if (!dev) {
        printk(KERN_ALERT "No such device named %s\n", ifname);
        return -ENODEV;    
    }
    addr = inet_select_addr(dev, 0, RT_SCOPE_UNIVERSE);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = addr;
    sin.sin_port = 0;
    err = sock->ops->bind(sock, (struct sockaddr*)&sin, sizeof(sin));
    if (err < 0) {
        printk(KERN_ALERT "sock bind err, err=%d\n", err);
        return err;
    }
    return 0;
}

static int connect_to_addr(struct socket *sock)//get aim ip and port, connect it with socket
{
    struct sockaddr_in daddr;
    int err;
    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = cpu_to_be32(dstip);
//    daddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
//    printk("0x%lx\n",INADDR_BROADCAST);
    daddr.sin_port = cpu_to_be16(dstport);
    err = sock->ops->connect(sock, (struct sockaddr*)&daddr,
            sizeof(struct sockaddr), 0);
    if (err < 0) {
        printk(KERN_ALERT "sock connect err, err=%d\n", err);
        return err;
    }
    return 0;
}

struct threadinfo{
    struct socket *sock;
    char *buffer;
    loff_t fsize;
};

/**
  * 返回压缩包大小，数据位于全局变量out_buf中
  */
int compress2send(int bufOrder, int dataSize)
{
	z_stream stream;
	unsigned int workspace_size = zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL);
	stream.workspace = kmalloc(workspace_size, GFP_KERNEL);
	zlib_deflateInit(&stream, 1);//Z_BEST_COMPRESSION);

	int flush = Z_FINISH;
	stream.next_in = buf[bufOrder];
	stream.avail_in = dataSize;
	
	stream.avail_out = out_buf_size;
	stream.next_out = out_buf;
	zlib_deflate(&stream, flush);

	int have = out_buf_size - stream.avail_out;

	(void)zlib_deflateEnd(&stream);
	kfree(stream.workspace);
	return have;
}

int kernel_send(struct socket *sock, struct msghdr *msg, int len)
{
	struct kvec iov;
	iov.iov_base = out_buf;
        iov.iov_len = len;
	int ret = kernel_sendmsg(sock, msg, &iov, 1, len);
	if (ret != len)
    	return -1;
	else 
		return ret;
}
static int sendthread(void *data)//build a thread to send message
{
    
    struct threadinfo *tinfo = data;
    struct msghdr msg = {.msg_flags = MSG_DONTWAIT|MSG_NOSIGNAL};
    int len, ret;

	while(1)
	{	
		char str[15];
		snprintf(str, 10, "%d:::%d", *id0,*id1);
		PrintStr(0, 13, 7, 1, FALSE, str, TRUE);
		if (*id0)
		{
			//压缩、UDP发送
			len = compress2send(0, DATA_PAGE_SIZE);
			ret = kernel_send(tinfo->sock, &msg, len);
			if (ret == -1){
				printk(KERN_ALERT "kernel_sendmsg err");
				goto out;
			}
			*id0 = 0;
		}
		if (*id1)
		{
			len = compress2send(1, DATA_PAGE_SIZE);
			ret = kernel_send(tinfo->sock, &msg, len);
			if (ret == -1){
				printk(KERN_ALERT "kernel_sendmsg err");
				goto out;
			}
			*id1 = 0;
                      //  printk("%d\n",ret);
		}
		//延时
		schedule_timeout_interruptible(1);
         //   break;
	}

out:
    kthreadtask = NULL;
    sk_release_kernel(tinfo->sock->sk);
    kfree(tinfo);
    printk("error Finish!\n");
    return 0;
}

int udp_send_init(bool *flag1,bool *flag2,unsigned int buf1,unsigned int buf2)
{
	//标志位
        id0 = flag1;
        id1 = flag2;
	buf[0] = buf1;
	buf[1] = buf2;
	out_buf_size = OUT_BUF_SIZE;
	out_buf = kmalloc(out_buf_size, GFP_KERNEL);
	if (out_buf == NULL)
		goto kmalloc_error;
	
    int err = 0;
    struct socket *sock;
    struct threadinfo *tinfo;

    err = sock_create_kern(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (err < 0) {
        printk(KERN_ALERT "UDP create sock err, err=%d\n", err);
        goto create_error;
    }
    sock->sk->sk_reuse = 1;


    err = bind_to_device(sock, ifname);
    if (err < 0) {
        printk(KERN_ALERT "Bind to %s err, err=%d\n", ifname, err);
        goto bind_error;
    }    
    err = connect_to_addr(sock);
    if (err < 0) {
        printk(KERN_ALERT "sock connect err, err=%d\n", err);
        goto connect_error;
    }
    
    tinfo = kmalloc(sizeof(struct threadinfo), GFP_KERNEL);
    if (!tinfo) {
        printk(KERN_ALERT "kmalloc threadinfo err\n");
        goto kmalloc_error;
    }
    tinfo->sock = sock;
//    tinfo->buffer = file_buf;
//    tinfo->fsize = fsize;
    kthreadtask = kthread_run(sendthread, tinfo, "sendfile");//tinfo include sock and buffer.

    if (IS_ERR(kthreadtask)) {
        printk(KERN_ALERT "create sendmsg thread err, err=%ld\n",
                PTR_ERR(kthreadtask));
        goto thread_error;
    }
    return 0;

thread_error:
    kfree(tinfo);
kmalloc_error:
bind_error:
connect_error:
    sk_release_kernel(sock->sk);
    kthreadtask = NULL;
create_error:
    return -1;

}

void udp_send_exit(void)
{
    
    if (kthreadtask) {
        kthread_stop(kthreadtask);
    }
    printk(KERN_ALERT "UDP send quit\n");

    return;
}

//module_init(udp_send_init);
//module_exit(udp_send_exit); 
