/**
* @file ex4.c
* @brief Exo 4 version avec read()
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
#include <errno.h>
#include <string.h>
#include <time.h>

#define DEV_PATH       "/dev/uio0"
#define LW_BRIDGE_BASE 0xFF200000
#define LEDR_BASE      0x00000000
#define HEX3_HEX0_BASE 0x00000020
#define HEX5_HEX4_BASE 0x00000030
#define KEY_BASE       0x00000050
#define N	       0
#define INTERRUPT_MASK 0x00000058
#define EDGE_MASK      0x0000005C
#define SET_VALUE      0xF
#define OFF_VALUE      0x0

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
	int offset = N * getpagesize();
	int length = getpagesize();

	int mem_fd = open(DEV_PATH, O_RDWR | O_SYNC);
	if (mem_fd == -1) {
		perror("ERROR: could not open \"/dev/uio0\"");
		exit(EXIT_FAILURE);
	}

	void *virtual_base = mmap(NULL, length, (PROT_READ | PROT_WRITE),
				  MAP_SHARED, mem_fd, offset);
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
	size_t length = getpagesize();
	if (munmap(virtual_base, length) != 0) {
		perror("ERROR: munmap() failed");
		exit(EXIT_FAILURE);
	}
}

// Struct for the countries
struct Country {
	char *name;
	char *capital;
	char *otherCities[3];
};

/**
 * @brief Shuffle an array of cities
 * 
 * @param cities Array of cities
 * @param count Number of cities
*/
void shuffleCities(char *cities[], int count)
{
	srand(time(NULL));

	for (int i = count - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		char *temp = cities[i];
		cities[i] = cities[j];
		cities[j] = temp;
	}
}

/**
 * @brief Concatenate the cities and shuffle them
 * 
 * @param country Struct of the country
 * @param result Destination array
*/
void concatAndShuffle(struct Country country, char *result[])
{
	// Copy of the addresses of the cities other than the capital
	for (int i = 0; i < 3; i++) {
		result[i] = country.otherCities[i];
	}

	// Add the capital
	result[3] = country.capital;

	// Shuffle the array
	shuffleCities(result, 4);
}

int main()
{
	//Used to handle the Ctrl+C signal if the user wants to stop the program
	signal(SIGINT, stopHandler);

	//Seed for the random number generator
	srand(time(NULL));

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

	struct Country countries[] = {
		{ "Suisse", "Berne", { "Lausanne", "Zurich", "Geneve" } },
		{ "France", "Paris", { "Marseille", "Lyon", "Toulouse" } },
		{ "Colombie", "Bogota", { "Medellin", "Cali", "Cartagena" } },
		{ "Belgique", "Bruxelle", { "Anvers", "Gand", "Liege" } },
		{ "Suede", "Stockholm", { "Malmo", "Goteborg", "Uppsala" } }
	};

	// Size of the array of cities. It will never change and I added 1 to include the capital
	int sizeCities = sizeof(countries[0].otherCities) /
				 sizeof(countries[0].otherCities[0]) +
			 1;

	void *virtual_base = initMemoryMapping();

	volatile int *HEX3_HEX0_ptr = (int *)(virtual_base + HEX3_HEX0_BASE);
	volatile int *pushbutton_edge = (int *)(virtual_base + EDGE_MASK);
	volatile int *pushbutton_interrupt =
		(int *)(virtual_base + INTERRUPT_MASK);

	int fd = open("/dev/uio0", O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// Setting the interrupt and edge mask for the pushbuttons
	*pushbutton_interrupt = SET_VALUE;
	*pushbutton_edge = SET_VALUE;

	int count = 0;
	// Display the first number
	*HEX3_HEX0_ptr = numbers[count];

	while (!end) {
		// unmask
		uint32_t info = 1;

		int r = rand() % 5;

		char *allCities[sizeCities];

		concatAndShuffle(countries[r], allCities);

		ssize_t nb = write(fd, &info, sizeof(info));

		if (nb != (ssize_t)sizeof(info)) {
			perror("write");
			close(fd);
			exit(EXIT_FAILURE);
		}

		/* Wait for interrupt */
		if ((nb = read(fd, &info, sizeof(info))) < 0) {
			if (errno == EINTR) {
				*pushbutton_edge = 0xF;
				break;
			}
		}
		// First part of the game, wait for the user to press the key0 to display the question
		if (nb == (ssize_t)sizeof(info) && *pushbutton_edge & 0x1) {
			printf(" Quelle est la capitale de la %s ?\n",
			       countries[r].name);
			for (int i = 0; i < sizeCities; i++) {
				printf("\t %d: %s\n", i, allCities[i]);
			}
		} else {
			//If the user presses another key, we come back to wait for him to press key 0
			*pushbutton_edge = SET_VALUE;
			continue;
		}

		// set the edge mask back to 0xF to wait for the next interrupt
		*pushbutton_edge = SET_VALUE;

		// Part 2 answer to the question by the user
		nb = write(fd, &info, sizeof(info));
		if (nb != (ssize_t)sizeof(info)) {
			perror("write");
			close(fd);
			exit(EXIT_FAILURE);
		}
		nb = read(fd, &info, sizeof(info));

		//Check answer for key 0
		if (nb == (ssize_t)sizeof(info) && *pushbutton_edge & 0x1 &&
		    !strcmp(allCities[0], countries[r].capital)) {
			printf("Bravo, bonne réponse !\n");
			count++;
			//Check answer for key 1
		} else if (nb == (ssize_t)sizeof(info) &&
			   *pushbutton_edge & 0x2 &&
			   !strcmp(allCities[1], countries[r].capital)) {
			printf("Bravo, bonne réponse !\n");
			count++;
			//Check answer for key 2
		} else if (nb == (ssize_t)sizeof(info) &&
			   *pushbutton_edge & 0x4 &&
			   !strcmp(allCities[2], countries[r].capital)) {
			printf("Bravo, bonne réponse !\n");
			count++;
			//Check answer for key 3
		} else if (nb == (ssize_t)sizeof(info) &&
			   *pushbutton_edge & 0x8 &&
			   !strcmp(allCities[3], countries[r].capital)) {
			printf("Bravo, bonne réponse !\n");
			count++;

		} else {
			printf("Mauvaise réponse \n");
			count = 0;
		}
		// set the edge mask back to 0xF to wait for the next interrupt
		*pushbutton_edge = SET_VALUE;

		*HEX3_HEX0_ptr = numbers[count];
		// We stop the game if the user has 15 correct answers since there are only 16 numbers to display
		if (count == 15) {
			printf("\nBravo, Vous avez répondu tout juste!\nFin du jeu!\n");
			break;
		}
	}

	// We turn off the LEDs, reset the pushbuttons and free the memory mapping
	*pushbutton_edge = SET_VALUE;
	*HEX3_HEX0_ptr = OFF_VALUE;
	cleanupMemoryMapping(virtual_base);

	return 0;
}