#include <stdlib.h>
#include <stdio.h>

#include <filesystem>

#include "image.hh"

void print_usage(void) {
	fprintf(stderr,
R"(grid: usage:
	grid ./.gridfile/
)"); return;
}

void print_not_implemented(void) {
	fprintf(stderr, "grid: mode is not implemented\n"); return;
}

int main(int argc, char **argv) {
	// { "grid", ".gridfile" }
	if(argc <= 1 || argc >= 3) {
		print_usage();
		return -1;
	}

	std::filesystem::path gridfile = argv[1];

	if(!std::filesystem::is_regular_file(gridfile)) {
		fprintf(stderr, "grid: invalid file: %s\n", argv[1]);
		return -1;
	}

	if(!image(gridfile)) {
		fprintf(stderr, "grid: imaging failed\n");
		return -1;
	}

	return 0;
}

