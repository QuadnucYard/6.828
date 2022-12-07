#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here

volatile uint32_t* e1000;

int
pci_e1000_attach(struct pci_func* pcif) {
	pci_func_enable(pcif);
	cprintf("attach e1000: %x %x\n", pcif->reg_base[0], pcif->reg_size[0]);
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("Register Set: %p, %p, %p, ...\n", e1000[0], e1000[1], e1000[2]);
	return 1;
}
