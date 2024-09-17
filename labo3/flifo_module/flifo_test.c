/**
* @file flifo_test.c
* @author Rafael Dousse
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "flifo.h"

#define DEVICE_PATH "/dev/flifo"
#define DEBUG	    0

/**
 * @brief Set the mode of the device.
 * @param fd File descriptor of the device.
 * @param mode Mode to set.
*/
void setMode(int fd, unsigned long mode)
{
	int err = ioctl(fd, FLIFO_CMD_CHANGE_MODE, mode);

	if (err < 0) {
		perror("ioctl set mode");
		exit(EXIT_FAILURE);
	}
#if DEBUG
	printf("Mode set to %d successfully\n", mode);
#endif
}

/**
 * @brief Reset the device.
 * @param fd File descriptor of the device.
 * @param mode Mode to set.
*/
void resetFLifo(int fd)
{
	int err = ioctl(fd, FLIFO_CMD_RESET);
	if (err < 0) {
		perror("ioctl resetFLifo");
		exit(EXIT_FAILURE);
	}
#if DEBUG
	printf("Device resetFLifo successfully\n");
#endif
}

/**
 * @brief Write values to the device.
 * @param fd File descriptor of the device.
 * @param values Array of values to write.
 * @param size Size of the array.
*/
void writeValue(int fd, int *values, int size)
{
	int err;
	for (int i = 0; i < size; i++) {
		err = write(fd, &values[i], sizeof(int));

		if (err < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}

#if DEPUG
		printf("Written %d to device\n", values[i]);
#endif
	}
}

/**
 * @brief Read values from the device.
 * @param fd File descriptor of the device.
 * @param values Array to store the read values.
 * @param size Size of the array.
*/
void readValue(int fd, int *values, int size)
{
	int err;
	for (int i = 0; i < size; i++) {
		err = read(fd, &values[i], sizeof(int));

		if (err < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}

#if DEBUG
		printf("Read %d from device\n", values[i]);
#endif
	}
}

/**
 * @brief Compare the read values with the expected values.
 * @param readValue Array of values to read
 * @param expected_values Array of expected values.
 * @param size Size of the arrays.
*/
void compareValue(int *readValue, int *expected_values, int size)
{
	int errors = 0;
	for (int i = 0; i < size; i++) {
		if (readValue[i] != expected_values[i]) {
			printf("Wrong value at index %d: read %d, expected %d\n",
			       i, readValue[i], expected_values[i]);
			errors++;
		}
	}

	if (errors == 0)
		printf("All values match the expected result.\n");
	else
		printf("There were %d wrong values.\n", errors);
}

/**
 * @brief Concatenate two arrays in a new one. For the test to work, the size must be even.
 * @param dest Destination array.
 * @param srcFIFO Source array for the first half in FIFO mode.
 * @param srcLIFO Source array for the second half in LIFO mode.
 * @param size Size of the arrays.
*/
void concat(int *dest, int *srcFIFO, int *srcLIFO, int size)
{
	// Check for null pointers
	if (!dest || !srcFIFO || !srcLIFO) {
		printf("Error: Null pointer provided as an argument.\n");
		return;
	}

	// Check for even size
	if (!size % 2) {
		printf("Error: Size must be even.\n");
		return;
	}
	int halfSize = size / 2;
	int i;

	// Concatenate the two arrays
	for (i = 0; i < halfSize; i++) {
		dest[i] = srcFIFO[i];
	}
	for (; i < size; i++) {
		dest[i] = srcLIFO[i - (halfSize)];
	}

#if DEBUG
	printf("Concatenation done.\n");
	printf("Result: \n{ ");
	for (i = 0; i < size; i++) {
		printf("%d ", dest[i]);
	}
	printf("}\n");
#endif
}

int main()
{
	int fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// Values to write and read
	static const int writeValues[NB_VALUES] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
	int readValues[NB_VALUES] = { 0 };
	int expectedValue_lifo[NB_VALUES];

	// Expected values in LIFO mode
	for (int i = 0; i < NB_VALUES; i++) {
		expectedValue_lifo[i] = writeValues[NB_VALUES - 1 - i];
	}

	//Test in FIFO mode
	resetFLifo(fd);
	setMode(fd, MODE_FIFO);
	writeValue(fd, writeValues, NB_VALUES);
	readValue(fd, readValues, NB_VALUES);
	compareValue(readValues, writeValues, NB_VALUES);

	// Test in LIFO mode
	resetFLifo(fd);
	setMode(fd, MODE_LIFO);
	writeValue(fd, writeValues, NB_VALUES);
	readValue(fd, readValues, NB_VALUES);
	compareValue(readValues, expectedValue_lifo, NB_VALUES);

	// Test half in FIFO and half in LIFO
	static const int HALFSIZE = NB_VALUES / 2;
	int testFifoLifo[NB_VALUES] = { 0 };
	concat(testFifoLifo, writeValues, expectedValue_lifo, NB_VALUES);

	resetFLifo(fd);
	writeValue(fd, writeValues, NB_VALUES);

	setMode(fd, MODE_FIFO);
	// Read the first half in FIFO mode
	readValue(fd, readValues, HALFSIZE);
	setMode(fd, MODE_LIFO);
	// Read the second half in LIFO mode
	readValue(fd, readValues + HALFSIZE, HALFSIZE);

	compareValue(readValues, testFifoLifo, NB_VALUES);

	close(fd);
	return EXIT_SUCCESS;
}