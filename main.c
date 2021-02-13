#ifdef _WIN32
#error "platform dependent code: only linux is supported."
#endif
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dlfcn.h>
#include <elf.h>
#include <link.h>

extern _start;

// needs at least 1 non-zero member in order to be put in .data
char count_data[5] = {0, 0, 0, 0, 1};
const char *read_error = "Could not open executable file for reading";
const char *write_error = "Could not open executable file for writing";

static void read_all(char *path, char *buffer);
static size_t file_size(char *path);
static void write_file(char *path, char *buffer, size_t size);

static void error(const char *message) __attribute__((noreturn));

int main(int argc, char **argv) {
	// read the entire binary into memory, we have to fully rewrite the binary later
	char *file_path = argv[0];
	size_t size = file_size(file_path);
	char *buffer = (char*)malloc(size);
	read_all(file_path, buffer);

	// map program memory space back to elf memory space
	Dl_info info;
	void *extra = NULL;
	dladdr1(&_start, &info, &extra, RTLD_DL_LINKMAP);
	struct link_map *map = extra;
	uintptr_t base = (uintptr_t)map->l_addr;
	uintptr_t count_addr = (uintptr_t)count_data - base;
	int *count = (int*) count_data;
	printf("count: %d\n", *count);

	// parse elf header
	Elf64_Ehdr header;
	memcpy(&header, buffer, sizeof(header));
	if(memcmp(header.e_ident, ELFMAG, SELFMAG) != 0) {
		error("Executable is not ELF");
	}
	// parse section headers and find the block containing count_data
	Elf64_Off e_shoff = header.e_shoff;
	uint16_t e_shentsize = header.e_shentsize;
	uint16_t e_shnum = header.e_shnum;
	Elf64_Shdr block_header;
	bool did_find_block = false;
	for(int i = 0; i < e_shnum; i++) {
		Elf64_Shdr sheader;
		memcpy(&sheader, buffer + e_shoff + i * e_shentsize, sizeof(sheader));
		if(sheader.sh_type == SHT_PROGBITS || sheader.sh_type == SHT_NOBITS) { // .data or .bss
			// if this section contains the data we're searching for
			if(count_addr > sheader.sh_addr && count_addr - sheader.sh_addr < sheader.sh_size) {
				block_header = sheader;
				did_find_block = true;
				// the object must be in .data; this is a development check
				assert(sheader.sh_type != SHT_NOBITS);
				break;
			}
		}
	}
	if(!did_find_block) {
		error("internal error: unable to find object in program data");
	}

	// sanity check
	int dummy;
	memcpy(&dummy, buffer + block_header.sh_offset + (count_addr - block_header.sh_addr), sizeof(int));
	assert(dummy == *count);
	// update count and write back
	(*count)++;
	memcpy(buffer + block_header.sh_offset + (count_addr - block_header.sh_addr), count, sizeof(int));

	// can't update the executable, but we can unlink and rewrite
	unlink(file_path);
	write_file(file_path, buffer, size);
	chmod(file_path, 0755);
	free(buffer);
	
	return EXIT_SUCCESS;
}

static void read_all(char *path, char *buffer) {
	size_t size = file_size(path);
	FILE *file = fopen(path, "rb");
	if(!file)
		error(read_error);
	if(fread(buffer, 1, size, file) != size)
		error("Error while reading executable");
	fclose(file);
}

static size_t file_size(char *path) {
	FILE *file = fopen(path, "rb");
	if (!file)
		error(read_error);
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);
	fclose(file);
	return size;
}

static void write_file(char *path, char *buffer, size_t size) {
	FILE *file = fopen(path, "wb");
	if (!file)
		error(write_error);
	if(fwrite(buffer, 1, size, file) != size)
		error("Error while writing executable");
	fclose(file);
}

static void error(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(EXIT_FAILURE);
}
