/*
 * stage_put DIR...: the regular files of each DIR packed into one blob and handed to the kernel for the host
 * (sysctl kern.hakios.stage: the loader stages it as the run's recording; "fastboot oem recording" and
 * "fastboot get_staged FILE" bring it to the computer). Nothing is written to the phone's storage.
 *
 * The blob: "HKSTAGE1", a u32 count, then for each file a u32 name length, the name (no directory), a u64 size and
 * the bytes; little-endian. tools/metal-cache-pull.sh reads it.
 */
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <unistd.h>

static unsigned char *blob;
static size_t used, room;

/* room for N more bytes at blob + used */
static int
reserve(size_t n)
{
	size_t want = room ? room : (size_t)1 << 20;
	unsigned char *bigger;

	if (used + n <= room) {
		return 0;
	}
	while (want < used + n) {
		want *= 2;
	}
	bigger = realloc(blob, want);
	if (bigger == NULL) {
		return -1;
	}
	blob = bigger;
	room = want;
	return 0;
}

static int
put(const void *p, size_t n)
{
	if (reserve(n) != 0) {
		return -1;
	}
	memcpy(blob + used, p, n);
	used += n;
	return 0;
}

int
main(int argc, char **argv)
{
	uint32_t count = 0;
	uint64_t total = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: stage_put DIR...\n");
		return 2;
	}
	if (put("HKSTAGE1", 8) != 0 || put(&count, 4) != 0) {
		return 1;
	}
	for (int a = 1; a < argc; a++) {
		DIR *d = opendir(argv[a]);
		struct dirent *e;

		if (d == NULL) {
			printf("stage_put: %s: not there\n", argv[a]);
			continue;
		}
		while ((e = readdir(d)) != NULL) {
			char path[1024];
			struct stat st;
			uint32_t nameLength = (uint32_t)strlen(e->d_name);
			uint64_t size;
			size_t mark = used;
			int fd;

			snprintf(path, sizeof(path), "%s/%s", argv[a], e->d_name);
			if (e->d_name[0] == '.' || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
				continue;
			}
			fd = open(path, O_RDONLY);
			if (fd < 0) {
				continue;
			}
			size = (uint64_t)st.st_size;
			if (put(&nameLength, 4) != 0 || put(e->d_name, nameLength) != 0 || put(&size, 8) != 0 || reserve((size_t)size) != 0) {
				close(fd);
				return 1;
			}
			/* (a file that changed under us is left out whole) */
			if (read(fd, blob + used, (size_t)size) != (ssize_t)size) {
				used = mark;
				close(fd);
				continue;
			}
			used += (size_t)size;
			close(fd);
			count++;
			total += size;
		}
		closedir(d);
	}
	memcpy(blob + 8, &count, 4);
	if (sysctlbyname("kern.hakios.stage", NULL, NULL, blob, used) != 0) {
		perror("stage_put: kern.hakios.stage");
		return 1;
	}
	printf("stage_put: %u files, %llu bytes, %zu bytes staged for the host\n", count, (unsigned long long)total, used);
	return 0;
}
