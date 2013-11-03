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
//#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/zlib.h>


static int bind_to_device(struct socket *sock, char *ifname);

static int connect_to_addr(struct socket *sock);

int compress2send(int bufOrder, int dataSize);

int kernel_send(struct socket *sock, struct msghdr *msg, int len);

static int sendthread(void *data);
//
int udp_send_init(bool *flag1,bool *flag2,unsigned int buf1,unsigned int buf2);

void udp_send_exit(void);
