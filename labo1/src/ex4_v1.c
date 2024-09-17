/**
* @file ex4_v1.c
* @brief This version only scrolls the message on the 7-segments leds while we push the keys  0 or 1. If they're not pushed the message won't move. Key0 make it scroll
*        the the right and key1 to the left.
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

	//Message to display: "Bienvenue en drv"
	//B = 0x7F I = 0x30 e = 0x7b  n = 0x37  v = u = 0x3e d = 0x5e r = 0x31
	uint8_t message[] = { 0x7F, 0x30, 0x7b, 0x37, 0x3e, 0x7b, 0x37, 0x3e,
			      0x7b, 0x00, 0x7b, 0x37, 0x00, 0x5e, 0x31, 0x3e };

	void *virtual_base = initMemoryMapping();

	volatile int *LEDR_ptr = (int *)(virtual_base + LEDR_BASE);
	volatile int *HEX3_HEX0_ptr = (int *)(virtual_base + HEX3_HEX0_BASE);
	volatile int *HEX5_HEX4_ptr = (int *)(virtual_base + HEX5_HEX4_BASE);
	volatile int *KEY_ptr = (int *)(virtual_base + KEY_BASE);

	int i = -1;

	while (!end) {
		if (*KEY_ptr & 0x1) {
			i--;
		} else if ((*KEY_ptr) & 0x2) {
			i++;
		}

		if (i <= 0) {
			i = 0;

			//Turning LEDS 5 -> 9 on
			*LEDR_ptr = 0x3e0;
		} else if (i >= 10) {
			i = 10;
			//Turning LEDS 0 -> 4 on
			*LEDR_ptr = 0x1f;
		} else {
			ledsOff(1, LEDR_ptr);
		}

		*HEX3_HEX0_ptr = (message[i + 2] << 24) |
				 (message[i + 3] << 16) |
				 (message[i + 4] << 8) | message[i + 5];
		*HEX5_HEX4_ptr = (message[i] << 8) | (message[i + 1]);

		sleep(1);
	}

	ledsOff(3, LEDR_ptr, HEX3_HEX0_ptr, HEX5_HEX4_ptr);

	cleanupMemoryMapping(virtual_base);
	return 0;
}
