/*
 * This is all just a hack. No locking no idea about performance...
 * no nothing.
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdint.h>

const char *trays_log_file = "/dev/stdout";
int trays_fd = -1;

unsigned long bb_depth = 0;

void
trays_init(const char *fp)
{
	if (fp != NULL) {
		trays_log_file = fp;
		trays_fd = open(trays_log_file, O_CREAT|O_EXCL|O_RDWR|O_NONBLOCK,
		  S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	} else {
		trays_fd = dup(1);
	}
	if (trays_fd == -1) {
		perror("trays_init");
		return;
	}
}	

void
trays_log(const char *m, uint64_t ba)
{
// Uhm fix for ULL, but just do this for now.
	char badboi[1024]; // overkill all we need is 11 extra for "%lu,"
	if (trays_fd != -1) {
		// Arbitrary 512.
		if (m && strlen(m) > 0 && strlen(m) < 512) {
			memset(badboi, 0, 1024);
			sprintf(badboi, "%lu %s %lx\n", bb_depth, m, ba);
			write(trays_fd, (void *)badboi, strlen(badboi));
		} else {
			char *toolong = "xxx: entry message too long.\n";	
			write(trays_fd, toolong, strlen(toolong));
		}
	} else {
		printf("xxx: logging to unopened logfile\n");
	}
}

void
trays_log_function_entry(const char *m)
{
	char badboi[1024]; // overkill all we need is 11 extra for "%lu,"
	++bb_depth;
	if (trays_fd != -1) {
		// Arbitrary 512.
		if (m && strlen(m) > 0 && strlen(m) < 512) {
			memset(badboi, 0, 1024);
			sprintf(badboi, "%lu %s %lx\n", bb_depth, m, (long)__builtin_return_address(0));
			write(trays_fd, (void *)badboi, strlen(badboi));
		} else {
			char *toolong = "xxx: entry message too long.\n";	
			write(trays_fd, toolong, strlen(toolong));
		}
	} else {
		printf("xxx: logging to unopened logfile\n");
	}
}

void
trays_log_function_exit(const char *m, uint64_t ba)
{
	char badboi[1024];

	if (trays_fd != -1) {
		// Arbitrary 512.
		if (m && strlen(m) > 0 && strlen(m) < 512) {
			memset(badboi, 0, 1024);
			sprintf(badboi, "%lu %s %lx\n", bb_depth, m, ba);
			write(trays_fd, (void *)badboi, strlen(badboi));
		} else {
			char *toolong = "xxx: exit message too long.\n";	
			write(trays_fd, toolong, strlen(toolong));
		}
	} else {
		printf("xxx: logging to unopened logfile\n");
	}
	--bb_depth;
}

static
long
getpc()
{
	return (long)__builtin_return_address(0);
}

void
trays_log_pc()
{
	char m[128];
	memset(m, 0, 128);
	sprintf(m, "0x%lx\n", (long)__builtin_return_address(0));
	write(trays_fd, (void *)m, strlen(m));
}
