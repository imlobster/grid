#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <filesystem>
#include <vector>
#include <string>

#include "grid.hh"

#define LOG "grider: "

enum class Command : uint8_t {
	NONE,
	LS,
	CAT,
};

void print_usage(void) {
	fprintf(stderr,
LOG R"(usage:
	grider <grid> ( ls | cat ) <path>
)");
}

int cat(grid::Grid &ifile, grid::Path &ipath) {
	if(ipath.empty()) {
		fprintf(stderr, LOG "no path provided\n");
		return -1;
	}

	std::size_t offset = ifile.find_file(ipath);
	if(!offset) {
		fprintf(stderr, LOG "unable to find file\n");
		return -1;
	}

	std::vector<char> content;
	if(!ifile.get_file_content(offset, content)) {
		fprintf(stderr, LOG "unable to read file content\n");
		return -1;
	}

	for(const char c : content)
		fputc(c, stdout);

	return 0;
}

int ls(grid::Grid &ifile, grid::Path &ipath) {
	auto print_table = [&ipath](grid::Grid::Table& itable) -> void {
		fprintf(stdout, "\t\033[37m'%s':\033[m\n", ipath.string().c_str());

		for(const auto &[key, _] : itable.nested)
			fprintf(stdout, "\033[34m%s\033[m\n", key.c_str());

		for(const auto &[key, _] : itable.contained)
			fprintf(stdout, "\033[32m%s\033[m\n", key.c_str());

		fputc('\n', stdout);
		return;
	};

	if(ipath.empty()) {
		print_table(ifile.table);
		return 0;
	}

	grid::Grid::Table table;

	if(!ifile.find_directory(ipath, table)) {
		fprintf(stderr, LOG "unable to find directory\n");
		return -1;
	}

	print_table(table);

	return 0;
}

int main(int argc, char **argv) {
	if(argc != 4) {
		print_usage(); return 1;
	}

	Command cmd = Command::NONE;

	if     (strcmp("ls", argv[2]) == 0)
		cmd = Command::LS;
	else if(strcmp("cat", argv[2]) == 0)
		cmd = Command::CAT;
	else {
		print_usage(); return 1;
	}

	grid::Grid file(argv[1]);
	grid::Path path(argv[3]);

	switch(cmd) {
		case Command::LS: return(ls(file, path)); break;
		case Command::CAT: return(cat(file, path)); break;
		default: print_usage(); return 1; break;
	}

	return 1;
};
