#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int perm;
	envid_t eid;
	while (true) {
		while (ipc_recv(&eid, &nsipcbuf, &perm) != NSREQ_OUTPUT)
			sys_yield();
		while (sys_net_try_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0)
			sys_yield();
	}
}
