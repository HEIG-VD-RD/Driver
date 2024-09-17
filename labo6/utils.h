/**
 * @brief : Headers for the chronometer driver 
*  @file : utils.h
* @Author : Rafael Dousse
*/

#ifndef UTILS_H
#define UTILS_H

#include "myTime.h"

#define LEDR_BASE      0x00000000
#define HEX3_HEX0_BASE 0x00000020
#define HEX5_HEX4_BASE 0x00000030
#define SWITCH_BASE    0x00000040
#define KEY_BASE       0x00000050
#define INTERRUPT_MASK 0x00000058
#define EDGE_MASK      0x0000005C
#define SET_VALUE      0xf
#define OFF_VALUE      0x0
#define LEDS_ON	       0x3ff
#define HEX_1	       0x1
#define HEX_2	       0x2
#define HEX_3	       0x4
#define HEX_4	       0x8
#define LED0	       (u32)0x1
#define LED1	       0x2
#define LED2	       0x4

//Numbers in hexa
u32 numbers[] = {
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

/**
 * @brief : Struct that represents a lap time with a list
 */
struct lap_time {
	u32 minutes;
	u32 seconds;
	u32 centiseconds;
	struct list_head list;
};

/**
 * @brief : Private structure for the chrono driver
 */
struct priv {
	// Current time of the chrono
	struct myTime timeChrono;
	// Device structure
	struct cdev cdev;
	// Class structure
	struct class *cl;
	// Device number
	dev_t dev_num;
	// Device structure
	struct device *device;

	// Memory pointer
	void *mem_ptr;
	// Switches pointer
	void *switches;
	// Leds pointer
	void *leds;
	// 7 segments pointers 0-3
	void *hex3_hex0;
	// 7 segments pointers 4-5
	void *hex5_hex4;
	// Edge pointer
	void *edge;
	// Interrupt mask pointer
	void *interruptMask;
	// Irq number
	int irqNum;

	// Workqueue for chrono
	struct workqueue_struct *wq_chrono;
	struct work_struct work_chrono;
	// Delayed work for chrono
	struct delayed_work work_delay;

	// Boolean to know if the chrono is running
	bool is_running;
	// Boolean to know if the list is beeing displayed
	bool is_showing_list;
	// Flag to know if the reset button has been pressed and the list is beeing displayed
	bool askedForRemove;

	// Time when the chrono has been started
	ktime_t start_time;
	// Time elapsed
	ktime_t elapsed_time;
	// Time when the chrono has been paused
	ktime_t paused_time;

	// Lap list (head)
	struct list_head lap_list;
	// Current lap iterator
	struct lap_time *current_lap;
	// Previous iterator
	struct lap_time *previous_lap;
	// Read lap iterator
	struct lap_time *read_lap_ptr;
	// Timer for leds
	struct timer_list led_timer;

	// Blink count
	uint8_t led_blink_count;
	// Laps display mode
	uint8_t mode;

	// Spinlock
	spinlock_t lock;
};

/**
 * @brief : Enum for the led state
 */
enum led_state {
	LED_OFF,
	LED_ON,
};

// Functions
static void display_time(struct priv *priv, struct myTime);
static void update_chrono(struct work_struct *work);
static void start_chrono(struct priv *priv);
static void stop_chrono(struct priv *priv);
static void reset_chrono(struct priv *priv);
static void ktime_to_myTime(u64 time_ns, struct myTime *to_transform);
static void add_lap(struct priv *priv, struct myTime time);
static void remove_laps(struct priv *priv);
static void display_list_work(struct work_struct *work);
static void ledsOperation(struct priv *priv, enum led_state value, u32 led);

#endif