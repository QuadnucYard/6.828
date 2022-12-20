#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/string.h>

#define HISTLEN 32
#define BUFLEN 256

struct buf_hist {
	char buf[HISTLEN][BUFLEN];
	int head, tail, cur;
};
static struct buf_hist hist;

char*
readline(const char *prompt)
{
	static char buf[BUFLEN];
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
creadline(const char* prompt, Completer completer) {
	// If not console, fall bakc on normal mode.
	if (!iscons(0))
		return readline(prompt);

	int i = 0, l = 0;
	int esc = 0, ins = 1, tab = 0;

	// tail is the one beyond last one. cur is the current one.
	hist.cur = hist.tail;
	char* buf = hist.buf[hist.cur];

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif
	cprintf("%s", buf);

	while (1) {
		int c = getchar();
		// cprintf("get %d\n", (int)c);
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		}
		if (c == '\t')
			++tab;
		else
			tab = 0;
		if ((c == '\b' || c == '\x7f') && i > 0) {
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
				case 'A': // Up
					if (hist.cur != hist.head) {
						hist.cur = (hist.cur - 1 + HISTLEN) % HISTLEN;
						buf = hist.buf[hist.cur];
						l = strlen(buf);
						cprintf("\r\033[K%s%s", prompt, buf);
					}
					break;
				case 'B': // Down
					if (hist.cur != hist.tail) {
						hist.cur = (hist.cur + 1) % HISTLEN;
						buf = hist.buf[hist.cur];
						l = strlen(buf);
						cprintf("\r\033[K%s%s", prompt, buf);
					}
					break;
				case 'D': // Left
					if (i > 0) {
						--i;
						cprintf("\033[D");
					}
					break;
				case 'C': // Right
					if (i < l) {
						++i;
						cprintf("\033[C");
					}
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
				l++;
			} else if (l == i) {
				l++;
			}
			buf[i++] = c;
		} else if (c == '\n' || c == '\r') {
			cputchar('\n');
			buf[l] = 0;
			if (hist.tail == hist.cur) {
				hist.tail = (hist.tail + 1) % HISTLEN;
				if (hist.tail == hist.head) {
					memset(hist.buf[hist.head], 0, BUFLEN);
					++hist.head;
				}
			}
			return buf;
		}
		if (tab > 0 && completer) {
			char* comp = completer(buf, i);
			if (comp) {
				char tmp[BUFLEN];
				strcpy(tmp, comp);
				int cnt = 1;
				for (char* comp2; (comp2 = completer(NULL, i)); ++cnt) {
					if (tab > 1) {
						if (cnt == 1) // End the line to be completed
							cprintf("\033[%dC\r\n%-20s", l - i, tmp);
						cprintf("%-20s%s", comp2, cnt % 6 == 5 ? "\r\n" : "");
					}
				}
				if (cnt == 1 && tab == 1) {
					int s = i;
					while (s > 0 && buf[s - 1] != ' ')
						--s;
					cprintf("%s", tmp + i - s);
					strcpy(buf + i, tmp + i - s);
					l = i = i + strlen(tmp);
				} else if (cnt > 1 && tab > 1) {
					cprintf("%s%s%s", cnt % 6 == 0 ? "" : "\r\n", prompt, buf);
				}
			}
		}
	}
}
