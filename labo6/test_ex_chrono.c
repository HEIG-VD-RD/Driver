/**
* @brief Test program for the chrono driver
* @file test_ex_chrono.c
* @author Rafael Dousse
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "myTime.h"

#define DEVICE_PATH "/dev/chrono"

int main()
{
	int fd = open(DEVICE_PATH, O_RDONLY);
	int err;
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	struct userReadTime time;

	err = read(fd, &time, sizeof(struct userReadTime));

	if (err < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	printf("Current time:  %02lu:%02lu.%02lu\n", time.currentTime.minutes,
	       time.currentTime.seconds, time.currentTime.centiseconds);
	printf("Laps time :\n");

	while (!time.isEnd) {
		printf("%02lu:%02lu.%02lu\n", time.lapTime.minutes,
		       time.lapTime.seconds, time.lapTime.centiseconds);
		err = read(fd, &time, sizeof(struct userReadTime));

		if (err < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
	}

	close(fd);

	return 0;
}
