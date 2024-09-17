/**
 * @file show_numbers_random.c
 * @Author Rafael Dousse
 * @brief Test program to write random values to the device module 
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdint.h>

#define DEVICE_PATH "/dev/show_number"
#define SIZE	    64
#define MAX_VALUE   999999


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
	srand(time(NULL)); 

	// At least one value to write 
	const int RANDOM_SIZE = 1 + (rand() % SIZE); 

	//RANDOM NUMBER GENERATOR
	uint32_t values[RANDOM_SIZE];
	// Write random values to the array
	for (int i = 0; i < (RANDOM_SIZE - 1) ; i++) {
		values[i] = rand() % MAX_VALUE;
	}
	// Just in case there is no 0 in the random values
	values[ RANDOM_SIZE ] = 0;

	int fd = open(DEVICE_PATH, O_RDWR);

	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// Write values to the device
	writeValue(fd, values, RANDOM_SIZE);

	close(fd);

	return 0;
}
