// inject conntrack into payload
// userland program read first 8 bytes of 
// origin_ip(4 bytes), origin_port(2bytes) and payloads length (2bytes)
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_nat_helper.h>
#include <net/netfilter/nf_conntrack_seqadj.h>
#include <net/tcp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yingjiu.zou");
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("kernel pkt inject conntrack in front of payload");

#define NET_TO_IP(IP) \
	((unsigned char *)&IP)[0], \
	((unsigned char *)&IP)[1], \
	((unsigned char *)&IP)[2], \
	((unsigned char *)&IP)[3]

void dump(unsigned char *buf, int size) {
	int i = 0;
	for (i = 0; i < size; i++) {
		printk("%02x ", buf[i]);
		if ((i+1)%16 == 0) {
			printk("\n");
		}
	}
	printk("\n");
}

static char *encode16u(const char *p, unsigned short w) {
	*(unsigned char *)(p + 0) = (w & 255);
	*(unsigned char *)(p + 1) = (w >> 8);
	p += 2;
	return (char*)p;
}

static char *encode32u(const char *p, unsigned int l) {
	*(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
	p += 4;
	return (char *)p;
}

