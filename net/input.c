#include "ns.h"

#define PKT_COUNT 3

extern union Nsipc nsipcbuf;

static struct jif_pkt* pkt = (struct jif_pkt*)REQVA;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	int r;

	for (int i = 0; i < PKT_COUNT; i++)
		if ((r = sys_page_alloc(0, (void*)pkt + i * PGSIZE, PTE_P | PTE_U | PTE_W)) < 0)
			panic("sys_page_alloc: %e", r);

	struct jif_pkt* cpkt = pkt;
	for (int i = 0; ; i = (i + 1) % PKT_COUNT) {
		cpkt = (struct jif_pkt*)((void*)pkt + i * PGSIZE);
		while ((r = sys_net_recv((void*)cpkt + sizeof(cpkt->jp_len), PGSIZE - sizeof(cpkt->jp_len))) < 0)
			sys_yield();

		cpkt->jp_len = r;
		ipc_send(ns_envid, NSREQ_INPUT, cpkt, PTE_P | PTE_U | PTE_W);
		sys_yield();
	}
}
