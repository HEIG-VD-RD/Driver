/**
 * @file show_numbers.c
 * @Author Rafael Dousse
 * @brief Test program to write values to the device module 
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>

#define DEVICE_PATH "/dev/show_number"
#define SIZE	   64

/**
 * @brief Write values to the device.
 * @param fd File descriptor of the device.
 * @param values Array of values to write.
 * @param size Size of the array.
*/
void writeValue(int fd, uint32_t *values, int size)
{
	printf("writeValue\n");
	int err;
	for (int i = 0; i < size; i++) {
		err = write(fd, &values[i], sizeof(uint32_t));

		if (err < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		printf("Written %d to device\n", values[i]);
	}
}

/**
 * @brief Read values from the device.
 * @param fd File descriptor of the device.
 * @param values Array to store the read values.
 * @param size Size of the array.
*/
void readValue(int fd, uint32_t *values, int size)
{
	int err;
	for (int i = 0; i < size; i++) {
		err = read(fd, &values[i], sizeof(uint32_t));

		if (err < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

	}
}

int main(int argc, char *argv[])
{
	
	const int SIZETOSEND = argc - 1;
	// Check if the number of arguments is correct
	if (argc < 2 || SIZETOSEND > SIZE) {
		printf("Usage: %s <value1> <value2> ... <value%d>\n", argv[0], SIZE);
		exit(EXIT_FAILURE);
	}

	uint32_t values[SIZETOSEND];
	// Convert the arguments to integers
	for (int i = 0; i < SIZETOSEND; i++) {
		values[i] = (uint32_t)strtoul(argv[i + 1], NULL, NULL);
	}

	int fd = open(DEVICE_PATH, O_RDWR);
	
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// Write the values to the device
	writeValue(fd, values, SIZETOSEND);

	close(fd);

	return 0;
}