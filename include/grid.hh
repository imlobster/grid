/*
	MIT License

	Copyright (c) 2026 imlobster

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
*/

#pragma once

#include <cstdint>

#include <fstream>
#include <filesystem>

#include <vector>
#include <string>

#include <unordered_map>

namespace {
	constexpr std::uint8_t SIZE_SIZE = sizeof(std::size_t);
}

namespace grid {

// Analogue of std::filesystem::path for Grid
struct Path {
	static constexpr char SEPARATOR = '/';

public:
	std::vector<std::string> path;

public:
	// Is path empty?
	bool empty(void) const noexcept { return path.empty(); }
	// Clear path
	void clear(void) noexcept { path.clear(); return; }

	bool has_parent_path(void) const { return path.size() >= 2; }
	bool has_extension(void) const {
		if(path.size() <= 0)
			return false;
		return path.back().find_first_of('.') != std::string::npos;
	}

	auto begin(void) const { return path.begin(); }
	auto end(void) const { return path.end(); }

	std::string filename(void) const {
		if(path.size() <= 0)
			return "";
		return path.back();
	}

	// Filename without extension
	std::string stem(void) const {
		if(path.size() <= 0)
			return "";
		std::size_t dot = path.back().find_last_of('.');
		if(dot == std::string::npos)
			return path.back();
		return path.back().substr(0, dot);
	}

	std::string extension(void) const {
		if(path.size() <= 0)
			return "";
		std::size_t dot = path.back().find_last_of('.');
		if(dot == std::string::npos)
			return "";
		return path.back().substr(dot);
	}

	std::string string(bool relative = false) const {
		std::string ostr = "";

		if(!relative) {
			for(std::size_t i = 0; i < path.size(); ++i)
				ostr += SEPARATOR + path[i];
		} else {
			for(std::size_t i = 0; i < path.size(); ++i) {
				if(i + 1 < path.size())
					ostr += path[i] + SEPARATOR;
				else
					ostr += path[i];
			}
		}

		return ostr;
	}

	Path parent_path(void) const {
		if(!has_parent_path()) { return *this; }
		Path new_path = *this;
		new_path.path.erase(new_path.path.end() - 1);
		return new_path;
	}

	void push(const std::string &istr) {
		path.push_back(istr);
		return;
	}

	std::string pop(void) {
		std::string back = std::move(path.back());
		path.pop_back();
		return back;
	}

private:
	void build_path_(const std::string &istr, std::vector<std::string> &opath) const {
		std::size_t start = 0;

		while(start < istr.size()) {
			while(start < istr.size() && istr[start] == SEPARATOR)
				++start;

			if(start >= istr.size())
				break;

			std::size_t end = start;

			while(end < istr.size() && istr[end] != SEPARATOR)
				++end;

			opath.emplace_back(istr.substr(start, end - start));
			start = end;
		}

		return;
	}

public:
	bool operator==(const Path &ipath) {
		if(ipath.path.size() != path.size())
			return false;

		for(std::size_t i = 0; i < path.size(); ++i) {
			if(ipath.path[i] != path[i])
				return false;
		}

		return true;
	}


	Path operator+(const std::string &istr) const {
		Path new_path = *this;
		new_path.path.back() += istr;
		return new_path;
	}

	Path& operator+=(const std::string &istr) {
		path.back() += istr;
		return *this;
	}

	Path operator/(const Path &ipath) const {
		Path new_path = *this;
		new_path.path.insert(new_path.path.end(), ipath.path.begin(), ipath.path.end());
		return new_path;
	}

	Path& operator/=(const Path &ipath) {
		path.insert(path.end(), ipath.path.begin(), ipath.path.end());
		return *this;
	}

	Path operator/(const std::string &istr) const {
		Path new_path = *this;
		std::vector<std::string> rest;
		build_path_(istr, rest);
		new_path.path.insert(new_path.path.end(), rest.begin(), rest.end());
		return new_path;
	}

	Path& operator/=(const std::string &istr) {
		std::vector<std::string> rest;
		build_path_(istr, rest);
		path.insert(path.end(), rest.begin(), rest.end());
		return *this;
	}

	Path(const Path &ipath) {
		path = ipath.path;
	}

	Path(const std::string &istr) {
		build_path_(istr, path);
	}

	Path(const char *istr) {
		build_path_(istr, path);
	}

	Path& operator=(const Path &ipath) {
		path = ipath.path;
		return *this;
	}

	Path& operator=(const std::string &istr) {
		build_path_(istr, path);
		return *this;
	}

	Path& operator=(const char *istr) {
		build_path_(istr, path);
		return *this;
	}

	~Path() = default;
};

struct Grid {
	using Entry = std::size_t;

	struct Table {
		std::unordered_map<std::string, Table> nested;
		std::unordered_map<std::string, Entry> contained;

		~Table() = default;
	};

private:
	std::ifstream stream_;
	Table table_;
	std::size_t bunch_offset;

public:

	// Is directory
	bool is_directory(const Path &ipath) {
		if(ipath.empty())
			return false;

		// Searching for the directory by path

		const Table *actual = &table_;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->nested.contains(ipath.path.back());
	}

	bool is_directory(const Path &ipath, const Table &itable) {
		if(ipath.empty())
			return false;

		// Searching for the directory by path

		const Table *actual = &itable;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->nested.contains(ipath.path.back());
	}

	// Is regular file
	bool is_regular_file(const Path &ipath) {
		if(ipath.empty())
			return 0;

		// Searching for the entry by path

		const Table *actual = &table_;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->contained.contains(ipath.path.back());;
	}

	bool is_regular_file(const Path &ipath, const Table &itable) {
		if(ipath.empty())
			return false;

		// Searching for the entry by path

		const Table *actual = &itable;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->contained.contains(ipath.path.back());;
	}

	// Exists
	bool exists(const Path &ipath) {
		if(ipath.empty())
			return false;

		// Searching for the entry by path

		const Table *actual = &table_;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->nested.contains(ipath.path.back()) || actual->contained.contains(ipath.path.back());
	}

	bool exists(const Path &ipath, const Table &itable) {
		if(ipath.empty())
			return false;

		// Searching for the entry by path

		const Table *actual = &itable;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		return actual->nested.contains(ipath.path.back()) || actual->contained.contains(ipath.path.back());
	}

	// Find directory
	bool find_directory(const Path &ipath, Table &otable) {
		if(ipath.empty())
			return false;

		// Searching for the directory by path

		const Table *actual = &table_;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		// Take a table from the directory

		{
			auto found = actual->nested.find(ipath.path.back());

			if(found == actual->nested.end())
				return false;

			otable = found->second;
		}

		return true;
	}

	// Find directory in directory
	bool find_directory(const Path &ipath, Table &otable, const Table &itable) {
		if(ipath.empty())
			return false;

		// Searching for the directory by path

		const Table *actual = &itable;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return false;

			actual = &(it->second);
		}

		// Take a table from the directory

		{
			auto found = actual->nested.find(ipath.path.back());

			if(found == actual->nested.end())
				return false;

			otable = found->second;
		}

		return true;
	}

	// Find file
	std::size_t find_file(const Path &ipath) {
		if(ipath.empty())
			return 0;

		std::size_t offset = 0;

		// Searching for the entry by path

		const Table *actual = &table_;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return 0;

			actual = &(it->second);
		}

		// Take an offset from the entry

		{
			auto found = actual->contained.find(ipath.path.back());

			if(found == actual->contained.end())
				return 0;

			offset = found->second;
		}

		return offset;
	}

	// Find file in directory
	std::size_t find_file(const Path &ipath, const Table &itable) {
		if(ipath.empty())
			return 0;

		std::size_t offset = 0;

		// Searching for the entry by path

		const Table *actual = &itable;

		for(std::size_t i = 0; i < ipath.path.size() - 1; ++i) {
			auto it = actual->nested.find(ipath.path[i]);

			if(it == actual->nested.end())
				return 0;

			actual = &(it->second);
		}

		// Take an offset from the entry

		{
			auto found = actual->contained.find(ipath.path.back());

			if(found == actual->contained.end())
				return 0;

			offset = found->second;
		}

		return offset;
	}

	// Get file content
	bool get_file_content(std::size_t ioffset, std::vector<char> &odata) {
		std::size_t entry_size = 0;

		stream_.seekg(ioffset, std::ios::beg);

		// Read entry size

		{
			std::uint8_t entry_size_bytes[SIZE_SIZE];
			stream_.read((char*)entry_size_bytes, SIZE_SIZE);

			if(stream_.gcount() != SIZE_SIZE)
				return false;

			for(std::size_t i = 0; i < SIZE_SIZE; ++i) {
				entry_size |= ((size_t)entry_size_bytes[i]) << (8 * i);
			}
		}

		// Write out entry content by chunks

		{
			constexpr std::size_t BUFFSZ = 4096;
			std::size_t rest = entry_size;

			odata.resize(entry_size);

			std::size_t offset = 0;

			while(rest >= 1) {
				std::size_t to_read = rest < BUFFSZ ? rest : BUFFSZ;

				stream_.read((char*)odata.data() + offset, to_read);

				if(stream_.gcount() != static_cast<std::streamsize>(to_read))
					return false;

				offset += to_read;
				rest   -= to_read;
			}
		}

		return true;
	}

	bool read(const Path &ipath, std::vector<char> &odata) {
		if(ipath.empty())
			return false;

		std::size_t offset = find_file(ipath);
		if(!offset)
			return false;

		return get_file_content(offset, odata);
	}

	bool read(const Path &ipath, std::vector<char> &odata, const Table &itable) {
		if(ipath.empty())
			return false;

		std::size_t offset = find_file(ipath, itable);
		if(!offset)
			return false;

		return get_file_content(offset, odata);
	}

private:

	bool read_in_table_(Table& otable) {
		std::size_t table_size = 0;

		// Read table size

		{
			std::uint8_t table_size_bytes[SIZE_SIZE];
			stream_.read((char*)table_size_bytes, SIZE_SIZE);

			if(stream_.gcount() != SIZE_SIZE)
				return false;

			for(std::size_t i = 0; i < SIZE_SIZE; ++i) {
				table_size |= ((size_t)table_size_bytes[i]) << (8 * i);
			}
		}

		for(std::size_t i = 0; i < table_size; ++i) {
			bool is_directory;

			// Read node type

			{
				std::uint8_t c;

				stream_.read((char*)&c, 1);
				if(stream_.gcount() != 1)
					return false;

				is_directory = (char)c == 'd';
			}

			std::string name = "";

			// Read node name

			{
				std::uint8_t read[64];
				bool end = false;

				while(!end) {
					stream_.read((char*)read, sizeof(read));
					std::streamsize rest = stream_.gcount();

					for(std::streamsize i = 0; i < rest; ++i) {
						if((char)read[i] != '\0')
							continue;

						name.append((char*)read, i);
						stream_.seekg(i - rest + 1, std::ios::cur);
						end = true;
						break;
					}

					if(!end)
						name.append((char*)read, rest);
				}
			}

			std::size_t pointing = 0;

			// Read node pointer

			{
				std::uint8_t offset_bytes[SIZE_SIZE];
				stream_.read((char*)offset_bytes, SIZE_SIZE);

				if(stream_.gcount() != SIZE_SIZE)
					return false;

				for(std::size_t i = 0; i < SIZE_SIZE; ++i) {
					pointing |= ((size_t)offset_bytes[i]) << (8 * i);
				}
			}

			// Add a node to the table

			if(is_directory) {
				// If this node is a table too, call this method
				Table nested_table;

				std::size_t was_here = stream_.tellg();

				stream_.seekg(pointing, std::ios::beg);
				read_in_table_(nested_table);

				otable.nested.emplace(name, nested_table);
				stream_.seekg(was_here, std::ios::beg);
			} else {
				otable.contained.emplace(name, pointing);
			}
		}

		return true;
	}

public:
	Grid(const std::filesystem::path &ipath) {
		stream_.open(ipath, std::ios::binary);

		if(!stream_.is_open())
			throw std::ios_base::failure("Unable to open file for reading");

		{
			std::size_t table_size = 0;

			// Read all tables size

			{
				stream_.seekg(0, std::ios::end);
				std::size_t file_size = stream_.tellg();
				stream_.seekg(0, std::ios::beg);

				if(file_size < SIZE_SIZE)
					throw std::runtime_error("File corrupted");

				std::uint8_t table_size_bytes[SIZE_SIZE];
				stream_.read((char*)table_size_bytes, SIZE_SIZE);

				if(stream_.gcount() != SIZE_SIZE)
					throw std::ios_base::failure("Unable to read file");

				for(std::uint8_t i = 0; i < SIZE_SIZE; ++i) {
					table_size |= ((size_t)table_size_bytes[i]) << (8 * i);
				}

				if(file_size < table_size)
					throw std::runtime_error("File corrupted");
			}

			bunch_offset = SIZE_SIZE + table_size;

			if(!read_in_table_(table_))
				throw std::runtime_error("Grid corrupted");
		}
	}

	~Grid() = default;
};

}

