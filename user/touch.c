#include <inc/lib.h>

void
touch(const char* path) {
	int r = open(path, O_CREAT);
	if (r < 0)
		printf("mkdir error: %e\n", r);
	else
		close(r);
}

void
umain(int argc, char** argv) {
	binaryname = "touch";
	if (argc == 1) {
		printf("touch: missing operand\n");
		return;
	}
	for (int i = 1; i < argc; i++)
		touch(argv[i]);
}
