#include <inc/stdio.h>
#include <inc/error.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i, c, echoing;

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = 0;
	echoing = iscons(0);
	while (1) {
		c = getchar();
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			if (echoing)
				cprintf("\b \b");
			i--;
		} else if (c >= ' ' && i < BUFLEN-1) {
			if (echoing)
				cputchar(c);
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			if (echoing)
				cputchar('\n');
			buf[i] = 0;
			return buf;
		} else if (c == 0x1B) {
			if (echoing)
				cputchar(c);
		}
	}
}


char*
creadline(const char* prompt) {
	// If not console, fall bakc on normal mode.
	if (!iscons(0))
		return readline(prompt);

	int i, l, c, echoing;
	int esc = 0, ins = 1;
#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = l = 0;
	while (1) {
		c = getchar();
		// cprintf("get %d\n", (int)c);
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		} else if ((c == '\b' || c == '\x7f') && i > 0) {
			for (int j = i; j <= l; j++)
				buf[j - 1] = buf[j];
			i--;
			l--;
			cprintf("\b\033[s%.*s \033[u", l - i, buf + i);
		} else if (c == 0x1B) { // ESC
			cputchar(c);
			esc = 1;
		} else if (c >= ' ' && i < BUFLEN - 1) { // Printable
			if (esc == 1) {// Matches 0x1B
				esc = c == 0x5B ? 2 : 0;
				continue;
			} else if (esc == 2) { // Matches 0x5B
				switch (c) {
				case 'D':
					cprintf("\033[D");
					--i;
					break;
				case 'C':
					cprintf("\033[C");
					if (buf[i])
						++i;
					break;
				}
				if (c == 0x32) esc = 3;
				else if (c == 0x33) esc = 4;
				else esc = 0;
				continue;
			} else if (esc == 3) {
				esc = 0;
				if (c == 0x7E) {
					ins = !ins;
					continue;
				}
			} else if (esc == 4) {
				esc = 0;
				if (c == 0x7E) {
					// Delete
					if (i < l) {
						for (int j = i + 1; j <= l; j++)
							buf[j - 1] = buf[j];
						l--;
						cprintf("\033[s%.*s \033[u", l - i, buf + i);
					}
					continue;
				}
			}
			cputchar(c | 0x0500);
			if (ins) {
				cprintf("\033[s%.*s\033[u", l - i, buf + i);
				for (int j = l; j > i; j--)
					buf[j] = buf[j - 1];
			}
			buf[i++] = c;
			l++;
		} else if (c == '\n' || c == '\r') {
			cputchar('\n');
			buf[l] = 0;
			return buf;
		}
	}
}
