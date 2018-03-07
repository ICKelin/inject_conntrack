#include "inject_conntrack.h"

static struct nf_conn *get_conntrack(struct sk_buff *skb, enum ip_conntrack_info *conntrack_info) {
	struct nf_conn *ct = NULL;
	ct = nf_ct_get(skb, conntrack_info);
	if (ct == NULL) {
		return NULL;
	}

	if (nf_ct_is_untracked(ct)) {
		return NULL;
	}

	if (!(ct->status & IPS_DST_NAT)) {
		return NULL;
	}

	return ct;
}

// not engouth headroom/tailroom
// expand skb tailroom
static int expand_skb(struct sk_buff *skb, int headroom, int tailroom) {
	pskb_expand_head(skb, headroom, tailroom, GFP_ATOMIC);
	if (skb == NULL) {
		printk("expand head fail, drop pkt");
		return -1;
	}

	if (skb_tailroom(skb) < 8) {
		printk("expand head too short, drop pkt");
		return -1;
	}

	return 0;
}

// tcp
static int handle_tcp(struct sk_buff *skb){
	struct iphdr *ip = ip_hdr(skb);
	struct tcphdr *tcp = tcp_hdr(skb);
	
	// conntrack
	enum ip_conntrack_info conntrack_info;	
	struct nf_conntrack_tuple *origin_tuple = NULL;
	struct nf_conn *ct = NULL;

	// inject buffer
	char insert_data[8];
	char *ptr = &insert_data[0];

	// origin payload
	char *origin_data = skb_network_header(skb) + ip->ihl * 4 + tcp->doff * 4;
	int origin_len = skb->tail - skb->network_header - ip->ihl * 4 - tcp->doff * 4;

	ct = get_conntrack(skb, &conntrack_info);
	if (ct == NULL) {
		return NF_ACCEPT;
	}

	origin_tuple = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;

	// empty payload
	if (origin_len == 0) {
		return NF_ACCEPT;
	}

	// modify tcp payload
	ptr = encode32u(ptr, origin_tuple->dst.u3.ip);
	ptr = encode16u(ptr, ntohs(origin_tuple->dst.u.all));
	ptr = encode16u(ptr, origin_len);


	// expand skb tailroom
	if (skb_tailroom(skb) < 8) {
		if (expand_skb(skb, skb_headroom(skb), skb_tailroom(skb) + 40) != 0) {
			return NF_DROP;	
		}
	}

	// copy conntrack data into skb payload 
	memmove(origin_data + 8, origin_data, origin_len);
	memcpy(origin_data, &insert_data[0], 8);

	nfct_seqadj_ext_add(ct);
	__nf_nat_mangle_tcp_packet(skb, ct, conntrack_info, 
			ip->ihl * 4, 0, origin_len, 
			origin_data, origin_len + 8, true);

	return NF_ACCEPT;
}

// udp
static int handle_udp(struct sk_buff *skb){
	struct iphdr *ip = ip_hdr(skb);
	struct udphdr *udp = udp_hdr(skb);

	enum ip_conntrack_info conntrack_info;	
	struct nf_conntrack_tuple *origin_tuple = NULL;
	struct nf_conn *ct = NULL;

	// inject buffer
	char insert_data[6];
	char *ptr = &insert_data[0];
	
	// origin payload
	char *origin_data = skb_network_header(skb) + ip->ihl * 4 + sizeof(*udp);
	int origin_len = skb->tail - skb->network_header - ip->ihl * 4 - sizeof(*udp);

	ct = get_conntrack(skb, &conntrack_info);	
	if (!ct) {
		return NF_ACCEPT;
	}

	origin_tuple = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;

	if (origin_len == 0) {
		return NF_ACCEPT;
	}
	// modify udp data
	ptr = encode32u(ptr, origin_tuple->dst.u3.ip);
	ptr = encode16u(ptr, ntohs(origin_tuple->dst.u.all));
	//ptr = encode16u(ptr, origin_len);

	if (skb_tailroom(skb) < 8) {
		if (expand_skb(skb, skb_headroom(skb), skb_tailroom(skb) + 40) != 0) {
			return NF_DROP;	
		}
	}

	memmove(origin_data + 6, origin_data, origin_len);
	memcpy(origin_data, &insert_data[0], 6);

	if (ct)	{
		nf_nat_mangle_udp_packet(skb, ct, conntrack_info, ip->ihl * 4, 0, origin_len, origin_data, origin_len + 6); 
	}
	return NF_ACCEPT;
}

// icmp
static int handle_icmp(struct sk_buff *skb){
	return NF_ACCEPT;
}

static int handle_entry(struct sk_buff *skb){
	struct iphdr *ip = ip_hdr(skb);
	switch(ip->protocol){
		case IPPROTO_TCP:
			return handle_tcp(skb);
			break;
		case IPPROTO_UDP:
			return handle_udp(skb);
			break;
		case IPPROTO_ICMP:
			return handle_icmp(skb);
			break;
		default:
			return NF_ACCEPT;
	}
}

static unsigned int func_call_back(unsigned int hooknum,
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff*)){

	return handle_entry(skb);
}

struct nf_hook_ops input_hook_ops = {
	.hooknum = NF_INET_LOCAL_IN,
	.hook = func_call_back,
	.pf = PF_INET,
	.priority = NF_IP_PRI_FILTER + 1};


	int __init inject_conntrack_init(void){
	printk("inject conntrack module init\n");

	//注册钩子
	nf_register_hook(&input_hook_ops);

	return 0;
}

void __exit inject_conntrack_exit(void){
	printk("inject conntrack module release\n");

	//注销钩子
	nf_unregister_hook(&input_hook_ops);
}

module_init(inject_conntrack_init);
module_exit(inject_conntrack_exit);
