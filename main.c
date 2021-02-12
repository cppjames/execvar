#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <string.h>
#include <stdbool.h>

char count[] = {0xAA, 0xBB, 0xCC, 0xDD, 0x00};
const unsigned id[] = {0xFFFFFFAA, 0xFFFFFFBB, 0xFFFFFFCC, 0xFFFFFFDD};
const char *read_error = "Could not open executable file for reading.";
const char *write_error = "Could not open executable file for writing.";

static void read_all(char *path, char *buffer);
static size_t file_size(char *path);
static void write_file(char *path, char *buffer, size_t size);

static bool is_id_at(char *buffer, size_t index);

static void error(const char *message) __attribute__((noreturn));

int main(int argc, char **argv) {
	unsigned char count = 0;
	char *file_path = argv[0];

	size_t size = file_size(file_path);
	char *buffer = (char*)calloc(size + 4, sizeof(char));
	read_all(file_path, buffer);

	size_t index = 0;
	for (size_t i = 0; i < size; i++) {
		if (is_id_at(buffer, i)) {
			index = i + sizeof(id) / sizeof(unsigned);
			break;
		}
	}

	count = buffer[index];
	count++;
	buffer[index] = count;

	fprintf(stdout, "%u\n", count);

	unlink(file_path);
	write_file(file_path, buffer, size);
	chmod(file_path, 0777);

	free(buffer);
	
	return EXIT_SUCCESS;
}

static void read_all(char *path, char *buffer) {	
	size_t size = file_size(path);
	FILE *file = fopen(path, "rb");
	if (!file)
		error(read_error);
	fread(buffer, 1, size, file);
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
	fwrite(buffer, sizeof(char), size, file);
	fclose(file);
}

static bool is_id_at(char *buffer, size_t index) {	
	for (size_t cnt = 0; cnt < 4; cnt++)
		if (buffer[index + cnt] != id[cnt])
			return false;
	return true;
}

static void error(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(EXIT_FAILURE);
}
