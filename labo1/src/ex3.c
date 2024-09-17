/**
* @file ex3.c
* @author Rafael Dousse
*/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>

#define MEM_DEV_PATH	  "/dev/mem"
#define LW_BRIDGE_BASE	  0xFF200000
#define LW_BRIDGE_SPAN	  0x00005000
#define LEDR_BASE	  0x00000000
#define LEDR_INTERVAL_SEC 1

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
 * @brief Used to turn off the lights. It has to be an int*  
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

	void *virtual_base = initMemoryMapping();

	volatile int *LEDR_ptr = (volatile int *)(virtual_base + LEDR_BASE);

	//Turn on the first LED
	*LEDR_ptr = 0x1;
	sleep(LEDR_INTERVAL_SEC);

	int leftnRight = 1;
	
	while (!end) {
		//Move the LEDS to the left with left shift operator
		if (leftnRight == 1) {
			*LEDR_ptr = *LEDR_ptr << 1;
			if (*LEDR_ptr == 0x200) {
				leftnRight = 0;
			}
			//Move the LEDS to the right with right shift operator
		} else {
			*LEDR_ptr = *LEDR_ptr >> 1;
			if (*LEDR_ptr == 0x1) {
				leftnRight = 1;
			}
		}
		sleep(LEDR_INTERVAL_SEC);
	}
	
	ledsOff(1, LEDR_ptr);

	cleanupMemoryMapping(virtual_base);
	return 0;
}
