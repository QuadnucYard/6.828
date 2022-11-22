// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "traceback", "traceback info", mon_backtrace },
	{ "showmappings", "Show physical page mappings", mon_showmappings },
	{ "memdump", "Dump memory contents", mon_memdump },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	struct Eipdebuginfo info;
	cprintf("Stack backtrace:\n");
	for (uint32_t ebp = read_ebp(); ebp != 0; ebp = *(uint32_t*)ebp) {
		uint32_t* p = (uint32_t*)ebp;
		uint32_t eip = p[1];
		debuginfo_eip(eip, &info);
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, eip, p[2], p[3], p[4], p[5], p[6]);
		cprintf("         %s:%d: %.*s+%d\n", info.eip_file, info.eip_line,
			info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
	}
	return 0;
}

int
mon_showmappings(int argc, char** argv, struct Trapframe* tf) {
	uint32_t begin = strtol(argv[1], NULL, 16);
	uint32_t end = strtol(argv[2], NULL, 16);
	begin = ROUNDDOWN(begin, PGSIZE);
	end = ROUNDDOWN(end, PGSIZE);
	cprintf("Show memory mappings: [%08p - %08p]\n", begin, end);
	for (uint32_t i = begin; i <= end; i += PGSIZE) {
		pte_t* pte;
		struct PageInfo* pp = page_lookup(kern_pgdir, (void*)i, &pte);
		if (!pp) {
			cprintf("  [%05p] ->\n", PGNUM(i));
			continue;
		}
		// virtual page, physical page, perm, refcnt
		cprintf("  [%05p] -> [%05p] %c%c%c%c%c%c%c%c%c  *%d\n",
			PGNUM(i),
			PGNUM(page2pa(pp)),
			*pte & PTE_G ? 'G' : '-',
			*pte & PTE_PS ? 'S' : '-',
			*pte & PTE_D ? 'D' : '-',
			*pte & PTE_A ? 'A' : '-',
			*pte & PTE_PCD ? 'C' : '-',
			*pte & PTE_PWT ? 'T' : '-',
			*pte & PTE_U ? 'U' : '-',
			*pte & PTE_W ? 'W' : '-',
			*pte & PTE_P ? 'P' : '-',
			pp->pp_ref
		);
	}
	/*
	The first five non-empty page mappings:
	ef000000 ef000 0xf03fd000 0xf011b8d8
	ef001000 ef001 0xf03fd004 0xf011b8e0
	ef002000 ef002 0xf03fd008 0xf011b8e8
	ef003000 ef003 0xf03fd00c 0xf011b8f0
	ef004000 ef004 0xf03fd010 0xf011b8f8

	Show memory mappings: [0xef000000 - 0xef004000]
	  [0xef000] -> [0x0011b] ------U-P  *1
	  [0xef001] -> [0x0011c] ------U-P  *1
	  [0xef002] -> [0x0011d] ------U-P  *1
	  [0xef003] -> [0x0011e] ------U-P  *1
	  [0xef004] -> [0x0011f] ------U-P  *1
	*/
	return 0;
}


int
mon_memdump(int argc, char** argv, struct Trapframe* tf) {
	uint32_t begin = strtol(argv[1], NULL, 16);
	uint32_t end = strtol(argv[2], NULL, 16);
	uint32_t cur_page = -1;
	struct PageInfo* pp = NULL;
	cprintf("Dump memory: [%08p - %08p)\n", begin, end);
	for (uint32_t i = begin; i < end; i++) {
		if (PGNUM(i) != cur_page) {
			cur_page = PGNUM(i);
			pp = page_lookup(kern_pgdir, (void*)i, NULL);
		}
		if ((i & 0xf) == 0 && i != begin) cprintf("\n");
		cprintf("%02x ", ((unsigned char*)page2kva(pp))[PGOFF(i)]);
	}
	cprintf("\n");
	/*
	K> memdump 0xef000000 0xef000100
	00 00 00 00 01 00 00 00 f8 af 15 f0 00 00 00 00
	08 b0 11 f0 00 00 00 00 10 b0 11 f0 00 00 00 00
	18 b0 11 f0 00 00 00 00 20 b0 11 f0 00 00 00 00
	28 b0 11 f0 00 00 00 00 30 b0 11 f0 00 00 00 00
	38 b0 11 f0 00 00 00 00 40 b0 11 f0 00 00 00 00
	48 b0 11 f0 00 00 00 00 50 b0 11 f0 00 00 00 00
	58 b0 11 f0 00 00 00 00 60 b0 11 f0 00 00 00 00
	68 b0 11 f0 00 00 00 00 70 b0 11 f0 00 00 00 00
	78 b0 11 f0 00 00 00 00 80 b0 11 f0 00 00 00 00
	88 b0 11 f0 00 00 00 00 90 b0 11 f0 00 00 00 00
	98 b0 11 f0 00 00 00 00 a0 b0 11 f0 00 00 00 00
	a8 b0 11 f0 00 00 00 00 b0 b0 11 f0 00 00 00 00
	b8 b0 11 f0 00 00 00 00 c0 b0 11 f0 00 00 00 00
	c8 b0 11 f0 00 00 00 00 d0 b0 11 f0 00 00 00 00
	d8 b0 11 f0 00 00 00 00 e0 b0 11 f0 00 00 00 00
	e8 b0 11 f0 00 00 00 00 f0 b0 11 f0 00 00 00 00
	*/
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
