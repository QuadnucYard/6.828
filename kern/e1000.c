#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t* e1000;

struct e1000_tx_desc tx_ring[TXRING_LEN];

int
pci_e1000_attach(struct pci_func* pcif) {
	pci_func_enable(pcif);
	cprintf("attach e1000: %x %x\n", pcif->reg_base[0], pcif->reg_size[0]);
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
	
	return 1;
}
