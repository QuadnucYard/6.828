#include <inc/lib.h>

void
mkdir(const char* path) {
	int r = open(path, O_MKDIR | O_CREAT | O_EXCL);
	if (r < 0)
		printf("mkdir error: %e\n", r);
	else
		close(r);
}

void
umain(int argc, char** argv) {
	binaryname = "mkdir";
	if (argc == 1) {
		printf("mkdir: missing operand\n");
		return;
	}
	for (int i = 1; i < argc; i++)
		mkdir(argv[i]);
}
