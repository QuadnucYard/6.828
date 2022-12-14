#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

volatile uint32_t* e1000;

uint32_t mac[2] = { 0x12005452, 0x5634 };

struct e1000_tx_desc tx_ring[TXRING_LEN];
char tx_buf[TXRING_LEN][MAX_PKT_SIZE];

struct e1000_rx_desc rx_ring[RXRING_LEN];
char rx_buf[RXRING_LEN][MAX_BUF_SIZE];

static void
init_desc() {
	memset(rx_ring, 0, sizeof(rx_ring));
	for (int i = 0; i < RXRING_LEN; i++) {
		rx_ring[i].addr = PADDR(rx_buf + i);
	}
}

int
pci_e1000_attach(struct pci_func* pcif) {
	pci_func_enable(pcif);
	cprintf("attach e1000: %x %x\n", pcif->reg_base[0], pcif->reg_size[0]);

	init_desc();

	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("Register Set: %p, %p, %p, ...\n", e1000[0], e1000[1], e1000[2]);

	/*
	Allocate a region of memory for the transmit descriptor list. Software should insure this memory is
	aligned on a paragraph (16-byte) boundary. Program the Transmit Descriptor Base Address
	(TDBAL/TDBAH) register(s) with the address of the region. TDBAL is used for 32-bit addresses
	and both TDBAL and TDBAH are used for 64-bit addresses.
	Set the Transmit Descriptor Length (TDLEN) register to the size (in bytes) of the descriptor ring.
	This register must be 128-byte aligned.
	The Transmit Descriptor Head and Tail (TDH/TDT) registers are initialized (by hardware) to 0b
	after a power-on or a software initiated Ethernet controller reset. Software should write 0b to both
	these registers to ensure this.
	Initialize the Transmit Control Register (TCTL) for desired operation to include the following:
	• Set the Enable (TCTL.EN) bit to 1b for normal operation.
	• Set the Pad Short Packets (TCTL.PSP) bit to 1b.
	• Configure the Collision Threshold (TCTL.CT) to the desired value. Ethernet standard is 10h.
	This setting only has meaning in half duplex mode.
	• Configure the Collision Distance (TCTL.COLD) to its expected value. For full duplex
	operation, this value should be set to 40h. For gigabit half duplex, this value should be set to
	200h. For 10/100 half duplex, this value should be set to 40h.
	Program the Transmit IPG (TIPG) register with the following decimal values to get the minimum
	legal Inter Packet Gap
	*/
	e1000[E1000_TDBAL / 4] = PADDR(tx_ring);
	// e1000[E1000_TDBAH / 4] = 0;
	e1000[E1000_TDLEN / 4] = sizeof(tx_ring);
	e1000[E1000_TDH / 4] = 0;
	e1000[E1000_TDT / 4] = 0;
	e1000[E1000_TCTL / 4] = E1000_TCTL_EN | E1000_TCTL_PSP | (0x10 << 4) | (0x40 << 12);
	e1000[E1000_TIPG / 4] = 10 | 4 << 10 | 6 << 20;

	/*
	Program the Receive Address Register(s) (RAL/RAH) with the desired Ethernet addresses.
	RAL[0]/RAH[0] should always be used to store the Individual Ethernet MAC address of the
	Ethernet controller
	Initialize the MTA (Multicast Table Array) to 0b. Per software, entries can be added to this table as
	desired.
	Program the Interrupt Mask Set/Read (IMS) register to enable any interrupt the software driver
	wants to be notified of when the event occurs. Suggested bits include RXT, RXO, RXDMT,
	RXSEQ, and LSC. There is no immediate reason to enable the transmit interrupts.
	If software uses the Receive Descriptor Minimum Threshold Interrupt, the Receive Delay Timer
	(RDTR) register should be initialized with the desired delay time.
	Allocate a region of memory for the receive descriptor list. Software should insure this memory is
	aligned on a paragraph (16-byte) boundary. Program the Receive Descriptor Base Address
	(RDBAL/RDBAH) register(s) with the address of the region. RDBAL is used for 32-bit addresses
	and both RDBAL and RDBAH are used for 64-bit addresses.
	Set the Receive Descriptor Length (RDLEN) register to the size (in bytes) of the descriptor ring.
	This register must be 128-byte aligned.
	The Receive Descriptor Head and Tail registers are initialized (by hardware) to 0b after a power-on
	or a software-initiated Ethernet controller reset. Receive buffers of appropriate size should be
	allocated and pointers to these buffers should be stored in the receive descriptor ring. Software
	initializes the Receive Descriptor Head (RDH) register and Receive Descriptor Tail (RDT) with the
	appropriate head and tail addresses.
	*/
	e1000[E1000_RA / 4] = mac[0];
	e1000[E1000_RA / 4 + 1] = mac[1] | E1000_RAH_AV;
	cprintf("e1000: mac address %x:%x\n", mac[1], mac[0]);
	memset((void*)e1000 + E1000_MTA / 4, 0, 128 * 4);
	e1000[E1000_IMS / 4] = E1000_IMS_RXT0 | E1000_IMS_RXO | E1000_IMS_RXDMT0 | E1000_IMS_RXSEQ | E1000_IMS_LSC;
	e1000[E1000_RDBAL / 4] = PADDR(rx_ring);
	e1000[E1000_RDLEN / 4] = sizeof(rx_ring);
	e1000[E1000_RDH / 4] = 0;
	e1000[E1000_RDT / 4] = RXRING_LEN - 1;
	/*
	Program the Receive Control (RCTL) register with appropriate values for desired operation to
	include the following:
	• Set the receiver Enable (RCTL.EN) bit to 1b for normal operation. However, it is best to leave
	the Ethernet controller receive logic disabled (RCTL.EN = 0b) until after the receive
	descriptor ring has been initialized and software is ready to process received packets.
	• Set the Long Packet Enable (RCTL.LPE) bit to 1b when processing packets greater than the
	standard Ethernet packet size. For example, this bit would be set to 1b when processing Jumbo
	Frames.
	• Loopback Mode (RCTL.LBM) should be set to 00b for normal operation.
	• Configure the Receive Descriptor Minimum Threshold Size (RCTL.RDMTS) bits to the
	desired value.
	• Configure the Multicast Offset (RCTL.MO) bits to the desired value.
	• Set the Broadcast Accept Mode (RCTL.BAM) bit to 1b allowing the hardware to accept
	broadcast packets.
	• Configure the Receive Buffer Size (RCTL.BSIZE) bits to reflect the size of the receive buffers
	software provides to hardware. Also configure the Buffer Extension Size (RCTL.BSEX) bits if
	receive buffer needs to be larger than 2048 bytes.
	• Set the Strip Ethernet CRC (RCTL.SECRC) bit if the desire is for hardware to strip the CRC
	prior to DMA-ing the receive packet to host memory.
	• For the 82541xx and 82547GI/EI, program the Interrupt Mask Set/Read (IMS) register to
	enable any interrupt the driver wants to be notified of when the even occurs. Suggested bits
	include RXT, RXO, RXDMT, RXSEQ, and LSC. There is no immediate reason to enable the
	transmit interrupts. Plan to optimize interrupts later, including programming the interrupt
	moderation registers TIDV, TADV, RADV and IDTR.
	• For the 82541xx and 82547GI/EI, if software uses the Receive Descriptor Minimum
	Threshold Interrupt, the Receive Delay Timer (RDTR) register should be initialized with the
	desired delay time.
	*/
	e1000[E1000_RCTL / 4] = E1000_RCTL_EN | E1000_RCTL_LPE | E1000_RCTL_LBM_NO | E1000_RCTL_BAM | E1000_RCTL_SZ_2048 | E1000_RCTL_SECRC;
	return 1;
}

int
e1000_transmit(void* addr, size_t len) {
	// Note that TDT is an index into the transmit descriptor array, 
	// not a byte offset;
	uint32_t tdt = e1000[E1000_TDT / 4];

	while (len > 0) {
		// Checking whether the next descriptor is free
		if ((tx_ring[tdt].cmd & E1000_TXD_CMD_RS) &&
			!(tx_ring[tdt].status & E1000_TXD_STAT_DD)) {
			return -1;
		}

		// copying the packet data into the next descriptor
		uint16_t len1 = MIN(MAX_PKT_SIZE, len);
		memmove(tx_buf[tdt], addr, len1);

		// Updating TDT
		tx_ring[tdt].addr = PADDR(tx_buf[tdt]);
		tx_ring[tdt].length = len1;
		tx_ring[tdt].cmd = E1000_TXD_CMD_RS;
		if (len == len1)
			tx_ring[tdt].cmd |= E1000_TXD_CMD_EOP;
		tx_ring[tdt].status = 0;
		tx_ring[tdt].cso = 0;
		tx_ring[tdt].css = 0;
		addr += len1;
		len -= len1;
		tdt = (tdt + 1) % TXRING_LEN;
	}

	e1000[E1000_TDT / 4] = tdt;

	return 0;
}

int e1000_receive(void* addr, size_t len) {
	uint32_t rdt = e1000[E1000_RDT / 4];
	struct e1000_rx_desc* nxt;
	void* base_addr = addr;

	do {
		rdt = (rdt + 1) % RXRING_LEN;
		nxt = rx_ring + rdt;
		if (!(nxt->status & E1000_RXD_STAT_DD))
			return -1;
		uint16_t len1 = MIN(nxt->length, len);
		memmove(addr, rx_buf[rdt], len1);
		nxt->status &= ~E1000_RXD_STAT_DD;
		addr += len1;
		len -= len1;
	} while (!(nxt->status & E1000_RXD_STAT_EOP));

	e1000[E1000_RDT / 4] = rdt;

	return addr - base_addr;
}