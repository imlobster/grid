#include "image.hh"

#define GRID_VERBOSE

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <limits.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#include <iostream>

struct File {
	std::string name;
	std::filesystem::path path;
	size_t size;
};

struct Directory {
	std::string name;
	std::vector<Directory> directories;
	std::vector<File> files;
};

void gather_directory(const std::filesystem::path &ipath, Directory &odir) {
	for(const auto& entry : std::filesystem::directory_iterator(ipath)) {
		if(std::filesystem::is_directory(entry)) {
			Directory nested = { entry.path().filename(), {}, {} };
			gather_directory(entry.path(), nested);
			odir.directories.emplace_back(std::move(nested));
			continue;
		}

		File file = { entry.path().filename(), entry.path(), entry.file_size() };
		odir.files.emplace_back(std::move(file));
	}
}

size_t calculate_table_size(const Directory &idir, bool recursive = false) {
	size_t result = sizeof(size_t);

	for(const Directory &dir : idir.directories) {
		result += 1 + dir.name.size() + 1 + sizeof(size_t);

		if(recursive) {
			result += calculate_table_size(dir, recursive);
		}
	}

	for(const File &file : idir.files) {
		result += 1 + file.name.size() + 1 + sizeof(size_t);
	}

	return result;
}

size_t calculate_data_bunch(const Directory &idir) {
	size_t result = 0;

	for(const Directory &dir : idir.directories) {
		result += calculate_data_bunch(dir);
	}

	for(const File &file : idir.files) {
		result += sizeof(size_t) + file.size;
	}

	return result;
}

bool image_directory(const Directory &idir, size_t &utableoff, size_t &ubunchoff, std::ofstream &oimg) {
	size_t this_off = utableoff;
	utableoff += calculate_table_size(idir, false);
	oimg.seekp(this_off, std::ios::beg);

	// write this table meta
	{
		size_t total_entries_in_table = idir.files.size() + idir.directories.size();

		uint8_t meta_out[sizeof(size_t)];
		for (size_t i = 0; i < sizeof(size_t); ++i) {
			meta_out[i] = (total_entries_in_table >> (8 * i)) & 0xFF;
		}

		oimg.write((char*)meta_out, sizeof(size_t));
	}

	for(const Directory &dir : idir.directories) {
		// write nested directory meta
		{
			size_t nested_table_meta_size = 1 + dir.name.size() + 1 + sizeof(size_t);
			std::vector<uint8_t> nested_table_meta(nested_table_meta_size, 0);

			nested_table_meta[0] = 'd';
			memcpy(&nested_table_meta[1], dir.name.c_str(), dir.name.size());

			for (size_t i = 0; i < sizeof(size_t); ++i) {
				nested_table_meta[1 + dir.name.size() + 1 + i] = (utableoff >> (8 * i)) & 0xFF;
			}

			oimg.write((char*)nested_table_meta.data(), nested_table_meta_size);
		}

		size_t was_here = oimg.tellp();
		image_directory(dir, utableoff, ubunchoff, oimg);
		oimg.seekp(was_here, std::ios::beg);
	}

	for(const File &file : idir.files) {
		// write file meta
		{
			size_t file_meta_size = 1 + file.name.size() + 1 + sizeof(size_t);
			std::vector<uint8_t> file_meta(file_meta_size, 0);

			file_meta[0] = 'f';
			memcpy(&file_meta[1], file.name.c_str(), file.name.size());

			for (size_t i = 0; i < sizeof(size_t); ++i) {
				file_meta[1 + file.name.size() + 1 + i] = (ubunchoff >> (8 * i)) & 0xFF;
			}

			oimg.write((char*)file_meta.data(), file_meta_size);
		}

		// write file
		size_t was_here = oimg.tellp();
		oimg.seekp(ubunchoff, std::ios::beg);

		{
			// write size
			{
				uint8_t size_out[sizeof(size_t)];
				for (size_t i = 0; i < sizeof(size_t); ++i) {
					size_out[i] = (file.size >> (8 * i)) & 0xFF;
				}

				oimg.write((char*)size_out, sizeof(size_t));
			}

			// copy file
			{
				std::ifstream this_file(file.path);
				if(!this_file.is_open()) {
					fprintf(stderr, "grid: unable to open file: %s\n", file.path.c_str());
					return false;
				}

				constexpr size_t BUFFSZ = 4096;

				uint8_t read_buffer[BUFFSZ];
				size_t rest = file.size;

				while(rest >= 1) {
					size_t to_read = rest < BUFFSZ ? rest : BUFFSZ;
					this_file.read((char*)read_buffer, to_read);
					oimg.write((char*)read_buffer, to_read);
					rest -= to_read;
				}
			}
		}

		ubunchoff += sizeof(size_t) + file.size;
		oimg.seekp(was_here, std::ios::beg);
	}

	return true;
}

bool read_gridfile(const std::filesystem::path &ipath, std::filesystem::path &oroot, std::filesystem::path &oimg)
{
	std::ifstream gridfile(ipath);
	if(!gridfile) {
		fprintf(stderr, "grid: unable to open gridfile: %s\n", ipath.c_str());
		return false;
	}

	std::string ln[2];

	{
		std::string tmpln;
		uint8_t lnp = 0;

		while(std::getline(gridfile, tmpln)) {
			if(lnp >= 2) {
				fprintf(stderr, "grid: gridfile is invalid: %s: too many lines\n", ipath.c_str());
				return false;
			}

			ln[lnp++] = tmpln;
		}

		if(lnp <= 1) {
			fprintf(stderr, "grid: gridfile is invalid: %s: too few lines\n", ipath.c_str());
			return false;
		}
	}

	oroot = ln[0];
	oimg = ln[1];

	return true;
}

bool image(const std::filesystem::path &igridfile) {
#ifdef GRID_VERBOSE
	fprintf(stdout,
R"(Context says:
	PATH  %s
)",
		igridfile.c_str()
	);
#endif

	std::filesystem::path root_path, img_path;

	// read gridfile
	if(!read_gridfile(igridfile, root_path, img_path)) {
		fprintf(stderr, "grid: failed on reading gridfile: %s\n", igridfile.c_str());
		return false;
	}

	// validate gridfile
	if(!std::filesystem::is_directory(root_path)) {
		fprintf(stderr, "grid: provided root is invalid: %s\n", root_path.c_str());
		return false;
	}

#ifdef GRID_VERBOSE
	fprintf(stdout,
R"(Gridfile says:
	ROOT  %s
	IMAGE %s
)",
		root_path.c_str(), img_path.c_str()
	);
#endif

	// collecting files
	Directory root = { "", {}, {} };
	gather_directory(root_path, root);

	size_t table_size = calculate_table_size(root, true);
	size_t bunch_size = calculate_data_bunch(root);

#ifdef GRID_VERBOSE
	fprintf(stdout,
R"(Map says:
	TABLES %lu
	BUNCH  %lu
)",
		table_size, bunch_size
	);
#endif

	// writing

	if(std::filesystem::exists(img_path)) {
		if(std::filesystem::is_regular_file(img_path)) {
			if(!std::filesystem::remove(img_path)) {
				fprintf(stderr, "grid: unable to delete file: %s\n", img_path.c_str());
				return false;
			}
		} else {
			fprintf(stderr, "grid: weird out path: %s\n", img_path.c_str());
			return false;
		}
	}

	std::ofstream out_img(img_path, std::ios::binary);
	if(!out_img.is_open()) {
		fprintf(stderr, "grid: unable to open file for writing: %s\n", img_path.c_str());
		return false;
	}

	// write header size
	{
		uint8_t size_out[sizeof(size_t)];

		for (size_t i = 0; i < sizeof(size_t); ++i) {
			size_out[i] = (table_size >> (8 * i)) & 0xFF;
		}

		out_img.write((char*)size_out, sizeof(size_t));
	}

	// image
	{
		size_t table_offset = sizeof(size_t);
		size_t file_offset = sizeof(size_t) + table_size;

		if(!image_directory(root, table_offset, file_offset, out_img)) { return false; }
	}

	return true;
}
