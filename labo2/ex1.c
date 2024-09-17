/**
* @file ex1.c
* @brief This programm scrolls from 0 to 15 on the 7 segment display. It uses the keys to change the number. Key0 increases the number and key1 decreases it.
* @author Rafael Dousse
*/

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>
#define MEM_DEV_PATH	  "/dev/mem"
#define LW_BRIDGE_BASE	  0xFF200000
#define LW_BRIDGE_SPAN	  0x00005000
#define LEDR_BASE	  0x00000000
#define HEX3_HEX0_BASE	  0x00000020
#define HEX5_HEX4_BASE	  0x00000030
#define KEY_BASE	  0x00000050
#define LEDR_INTERVAL_SEC 1
#define VERSION		  0

static int end = 0;

/**
 * @brief Signal handler for Ctrl+C. Used to stop the program and turn off the LEDs
 * @param signum
 */
void stopHandler(int signum)
{
	printf("\nSignal caught\n");
	end = 1;
	return;
}

/**
 * @brief Initialize the memory mapping
 * @return Address of the memory mapping
 */
void *initMemoryMapping()
{
	int mem_fd = open(MEM_DEV_PATH, O_RDWR | O_SYNC);
	if (mem_fd == -1) {
		perror("ERROR: could not open \"/dev/mem\"");
		exit(EXIT_FAILURE);
	}

	void *virtual_base = mmap(NULL, LW_BRIDGE_SPAN,
				  (PROT_READ | PROT_WRITE), MAP_SHARED, mem_fd,
				  LW_BRIDGE_BASE);
	if (virtual_base == MAP_FAILED) {
		perror("ERROR: mmap() failed");
		close(mem_fd);
		exit(EXIT_FAILURE);
	}

	close(mem_fd);
	return virtual_base;
}

/**
 * @brief Unmap the memory mapping
 *
 * @param virtual_base Address of the memory mapping
 */
void cleanupMemoryMapping(void *virtual_base)
{
	if (munmap(virtual_base, LW_BRIDGE_SPAN) != 0) {
		perror("ERROR: munmap() failed");
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Used to turn off the lights.
 * 
 * @param count Number of variables given
 * @param args The variables to "turn off"
*/
void ledsOff(int count, ...)
{
	va_list args;
	va_start(args, count);

	for (int i = 0; i < count; i++) {
		volatile int *led_ptr = va_arg(args, volatile int *);
		*led_ptr = 0x0;
	}

	va_end(args);
}

int main()
{
	//Used to handle the Ctrl+C signal if the user wants to stop the program
	signal(SIGINT, stopHandler);

	//Numbers
	uint16_t numbers[] = {
		0x3f, // 0
		0x06, // 1
		0x5b, // 2
		0x4f, // 3
		0x66, // 4
		0x6d, // 5
		0x7d, // 6
		0x07, // 7
		0x7f, // 8
		0x6f, // 9
		0x063f, // 10
		0x0606, // 11
		0x065b, // 12
		0x064f, // 13
		0x0666, // 14
		0x066d, // 15
	};

	void *virtual_base = initMemoryMapping();


	//volatile int *LEDR_ptr = (int *)(virtual_base + LEDR_BASE);
	volatile int *HEX3_HEX0_ptr = (int *)(virtual_base + HEX3_HEX0_BASE);
	//volatile int *HEX5_HEX4_ptr = (int *)(virtual_base + HEX5_HEX4_BASE);
	volatile int *KEY_ptr = (int *)(virtual_base + KEY_BASE); // Correction : volatile int *KEY_ptr = (int *)((int8_t*)virtual_base + KEY_BASE);

	uint8_t i = 0;
	uint8_t previousKeyState = 0;

	while (!end) {
		//Used to detect the rising edge of the keys
		uint8_t currentKeyState = *KEY_ptr & 0x3;

		if ((currentKeyState & 0x1) && !(previousKeyState & 0x1)) {
			i = (i + 1) % 16;
		} else if ((currentKeyState & 0x2) &&
			   !(previousKeyState & 0x2)) {
			//The cast in unsigned is necessary because it signes the number when we go from 0 to 255 with the negation
			i = (unsigned)(i - 1) % 16;
		}

		//Used to detect the rising edge of the keys
		previousKeyState = currentKeyState;

		*HEX3_HEX0_ptr = numbers[i];
	}

	ledsOff(1, HEX3_HEX0_ptr);

	cleanupMemoryMapping(virtual_base);
	return 0;
}
