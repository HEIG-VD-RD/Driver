/**
 * @brief Driver for a chronometer
*  @file : exercice_chrono.c
* @Author : Rafael Dousse
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/sysfs.h>

#include "utils.h"

#define CENTI_DIVIDER	(u32)10000000
#define SEC_DIVIDER	(u32)1000000000
#define MIN_DIVIDER	(u64)60000000000

#define MIN_DIVIDER2	(u32)60000000
#define MIN_DIVIDER3	(u32)1000
#define CENTI_MOD	(u64)100
#define SEC_MOD		(u64)60
#define MIN_MOD		(u64)100

#define TIMER_MS	200
#define SECONDS_IN_MS	2000
#define SECONDS_COUNTER (SECONDS_IN_MS / TIMER_MS)
#define UPDATE_INTERVAL 3000

#define DEVICE_NAME	"chrono"

static inline u32 abs_u32(s32(x))
{
	return (u32)(x < 0 ? -x : x);
}

//---------------------------------------------------------------------------------------------------
//IRQ Handler

/**
 * @brief Handler for the IRQ. Call the thread handler
 * @param irq irq number
 * @param dev_id device id
 * @return IRQ_WAKE_THREAD
 */
static irqreturn_t irq_handler(int irq, void *dev_id)
{
	return IRQ_WAKE_THREAD;
}

/**
 * @brief Thread handler for the IRQ. Handle the logic of the keys
 * Hex 1 : Start/Stop the chrono Hex 2 : Add a lap Hex 3 : Display the list of laps Hex 4 : Reset the chrono
 * @param irq irq number
 * @param dev_id device id
 * @return IRQ_HANDLED
 */
static irqreturn_t irq_thread_handler(int irq, void *dev_id)
{
	unsigned long flags;
	struct priv *priv = (struct priv *)dev_id;
	uint32_t edge = ioread32(priv->edge);

	switch (edge) {
	case HEX_1:
		if (priv->is_running) {
			stop_chrono(priv);
		} else {
			start_chrono(priv);
		}

		break;
	case HEX_2:
		if (priv->is_running) {
			add_lap(priv, priv->timeChrono);
		}

		break;
	case HEX_3:

		if (priv->is_showing_list) {
			priv->is_showing_list = false;
		} else {
			priv->is_showing_list = true;

			spin_lock_irqsave(&priv->lock, flags);

			priv->current_lap = list_first_entry(
				&priv->lap_list, typeof(*priv->current_lap),
				list);

			spin_unlock_irqrestore(&priv->lock, flags);

			schedule_delayed_work(&priv->work_delay,
					      msecs_to_jiffies(0));
		}

		break;
	case HEX_4:
		reset_chrono(priv);
		break;
	default:
		pr_info("Nothing to do\n");
		break;
	}

	// Set the edge
	iowrite32(SET_VALUE, priv->edge);
	return IRQ_HANDLED;
}

//---------------------------------------------------------------------------------------------------
//Timer

/**
 * @brief Handler for the timer. Handle the blinking of the LED2
 * @param t timer
 */
static void led_timer_handler(struct timer_list *t)
{
	struct priv *priv = from_timer(priv, t, led_timer);

	// Static enum led_state to keep the state of the LED2
	static enum led_state tmp_led_value;

	if (priv->led_blink_count > 0) {
		// Invert the LED2 value
		tmp_led_value = (tmp_led_value == LED_ON) ? LED_OFF : LED_ON;

		ledsOperation(priv, tmp_led_value, LED2);

		priv->led_blink_count--;

		// Restart the timer
		mod_timer(&priv->led_timer,
			  jiffies + msecs_to_jiffies(TIMER_MS));
	} else {
		// Turn off the LED after the blinking
		ledsOperation(priv, LED_OFF, LED2);
	}
}

/**
 * @brief Start the timer for the blinking of the LED
 * @param priv private structure
 */
static void start_led_blinking(struct priv *priv)
{
	priv->led_blink_count = SECONDS_COUNTER; // 2 secondes / 200 ms = 10
	mod_timer(&priv->led_timer, jiffies + msecs_to_jiffies(TIMER_MS));
}

//---------------------------------------------------------------------------------------------------
// Workqueue operation

/**
 * @brief Display the time on the 7-segments display
 * @param priv private structure
 * @param time time to display
 */
static void display_time(struct priv *priv, struct myTime time)
{
	// Extract the digits
	u32 min_tens = time.minutes / 10;
	u32 min_units = time.minutes % 10;
	u32 sec_tens = time.seconds / 10;
	u32 sec_units = time.seconds % 10;
	u32 cent_tens = time.centiseconds / 10;
	u32 cent_units = time.centiseconds % 10;
	// The array contains the values for the numbers to display on the 7-segments
	u32 hex3_hex0_value = (numbers[sec_tens] << 24) |
			      (numbers[sec_units] << 16) |
			      (numbers[cent_tens] << 8) | numbers[cent_units];
	u32 hex5_hex4_value = (numbers[min_tens] << 8) | numbers[min_units];

	iowrite32(hex3_hex0_value, priv->hex3_hex0);
	iowrite32(hex5_hex4_value, priv->hex5_hex4);
}

/**
 * @brief Convert the time in nanoseconds to a myTime structure
 * @param time_ns time in nanoseconds
 * @param to_transform myTime structure to fill
 */
static void ktime_to_myTime(u64 time_ns, struct myTime *to_transform)
{
	u64 centiseconds;
	u64 seconds;
	u64 minutes;

	// Compute the centiseconds, the seconds and the minutes
	centiseconds = div_u64(time_ns, CENTI_DIVIDER);
	div_u64_rem(centiseconds, CENTI_MOD, &to_transform->centiseconds);

	seconds = div_u64(time_ns, SEC_DIVIDER);
	div_u64_rem(seconds, SEC_MOD, &to_transform->seconds);

	minutes = div_u64(time_ns, MIN_DIVIDER3);
	minutes = div_u64(minutes, MIN_DIVIDER2);
	div_u64_rem(minutes, MIN_MOD, &to_transform->minutes);
}

/**
 * @brief Work function to update the chrono
 * @param work work structure
 */
static void update_chrono(struct work_struct *work)
{
	struct priv *priv = container_of(work, struct priv, work_chrono);
	ktime_t current_time;
	u64 total_ns;
	unsigned long flags;

	while (priv->is_running) {
		spin_lock_irqsave(&priv->lock, flags);
		// Check to stop the chrono at 99:59:99
		if (priv->timeChrono.minutes == 99 &&
		    priv->timeChrono.seconds == 59 &&
		    priv->timeChrono.centiseconds > 90) {
			priv->timeChrono.centiseconds = 99;
			display_time(priv, priv->timeChrono);
			stop_chrono(priv);

			spin_unlock_irqrestore(&priv->lock, flags);
			break;
		}

		current_time = ktime_get_ns();
		// Compute the elapsed time
		priv->elapsed_time =
			ktime_add(priv->paused_time,
				  ktime_sub(current_time, priv->start_time));
		total_ns = (u64)ktime_to_ns(priv->elapsed_time);

		ktime_to_myTime(total_ns, &priv->timeChrono);
		// We display the time if we are not in the "displaying the list" mode
		if (!priv->is_showing_list) {
			display_time(priv, priv->timeChrono);
		}
		spin_unlock_irqrestore(&priv->lock, flags);
		msleep(10);
	}
}

/**
 * @brief Start the chrono with queue_work
 * @param priv private structure 
 */
static void start_chrono(struct priv *priv)
{
	if (!priv->is_running) {
		priv->start_time = ktime_get_ns();
		priv->is_running = true;
		queue_work(priv->wq_chrono, &priv->work_chrono);
		ledsOperation(priv, LED_ON, LED0);
	}
}

/**
 * @brief Stop the chrono
 * @param priv private structure
 */
static void stop_chrono(struct priv *priv)
{
	if (priv->is_running) {
		ktime_t current_time = ktime_get_ns();
		// Compute the paused time
		priv->paused_time =
			ktime_add(priv->paused_time,
				  ktime_sub(current_time, priv->start_time));
		priv->is_running = false;

		ledsOperation(priv, LED_OFF, LED0);
	}
}

/**
 * @brief Reset the chrono
 * @param priv private structure
 */
static void reset_chrono(struct priv *priv)
{
	if (!priv->is_running) {
		stop_chrono(priv);
		unsigned long flags;

		spin_lock_irqsave(&priv->lock, flags);
		struct myTime last_time = priv->timeChrono;
		spin_unlock_irqrestore(&priv->lock, flags);

		priv->elapsed_time = ktime_set(0, 0);
		priv->paused_time = ktime_set(0, 0);
		priv->timeChrono = (struct myTime){ 0, 0, 0 };

		priv->read_lap_ptr = list_first_entry_or_null(
			&priv->lap_list, typeof(*priv->read_lap_ptr), list);

		// If we are not in the "displaying the list" mode, we can remove the laps but if we are, we will add a flag to remove the laps
		// when we will exit the "displaying the list" mode.
		if (!priv->is_showing_list) {
			remove_laps(priv);
			display_time(priv, priv->timeChrono);
		} else {
			add_lap(priv, last_time);
			priv->askedForRemove = true;
		}
	}
}
//---------------------------------------------------------------------------------------------------
// Delayed work operation

/**
 * @brief Display the lap time depending the mode
 * @param priv private structure
 */
static void display_lap(struct priv *priv, struct lap_time *lap,
			struct lap_time *prev_lap)
{
	struct myTime time_to_display;
	u32 lap_minutes = lap->minutes;
	u32 lap_seconds = lap->seconds;
	u32 lap_centiseconds = lap->centiseconds;

	if (priv->mode == 0) {
		time_to_display.minutes = lap_minutes;
		time_to_display.seconds = lap_seconds;
		time_to_display.centiseconds = lap_centiseconds;
	} else {
		if (lap_centiseconds < prev_lap->centiseconds) {
			lap_seconds -= 1;
			lap_centiseconds += 100;
		}
		time_to_display.centiseconds =
			lap_centiseconds - prev_lap->centiseconds;

		if (lap_seconds < prev_lap->seconds) {
			lap_minutes -= 1;
			lap_seconds += 60;
		}
		time_to_display.seconds = lap_seconds - prev_lap->seconds;
		time_to_display.minutes = lap_minutes - prev_lap->minutes;
	}

	display_time(priv, time_to_display);
}

/**
 * @brief Display the list of laps using a delayed work
 * @param work work structure
 */
static void display_list_work(struct work_struct *work)
{
	struct priv *priv =
		container_of(to_delayed_work(work), struct priv, work_delay);
	unsigned long flags;

	if (list_empty(&priv->lap_list)) {
		pr_info("No laps to display\n");
		priv->is_showing_list = false;
		return;
	}

	if (!priv->is_showing_list ||
	    list_entry_is_head(priv->current_lap, &priv->lap_list, list)) {
		priv->is_showing_list = false;

		if (priv->askedForRemove) {
			priv->askedForRemove = false;
			remove_laps(priv);
		}
		priv->previous_lap = NULL;

		spin_lock_irqsave(&priv->lock, flags);
		display_time(priv, priv->timeChrono);
		spin_unlock_irqrestore(&priv->lock, flags);

		ledsOperation(priv, LED_OFF, LED1);

		pr_info("Either asked for the end or end of list");
		return;
	}

	spin_lock_irqsave(&priv->lock, flags);
	if (priv->previous_lap == NULL) {
		display_lap(
			priv, priv->current_lap,
			&(struct lap_time){
				.minutes = 0,
				.seconds = 0,
				.centiseconds = 0,
			}); // For the first lap, use 0:0.0 as time for previous lap since it's suppose to display from the beginning

	} else {
		display_lap(priv, priv->current_lap, priv->previous_lap);
	}
	// Move the pointers
	priv->previous_lap = priv->current_lap;
	priv->current_lap = list_next_entry(priv->current_lap, list);

	spin_unlock_irqrestore(&priv->lock, flags);

	ledsOperation(priv, LED_ON, LED1);

	schedule_delayed_work(&priv->work_delay,
			      msecs_to_jiffies(UPDATE_INTERVAL));
}

//---------------------------------------------------------------------------------------------------
// Lap operation

/**
 * @brief Add a lap to the list
 * @param priv private structure
 * @param time time to add
 */

static void add_lap(struct priv *priv, struct myTime time)
{
	struct lap_time *new_lap;
	unsigned long flags;

	new_lap = kmalloc(sizeof(*new_lap), GFP_KERNEL);
	if (!new_lap) {
		pr_err("Cannot allocate memory\n");
		return;
	}

	new_lap->minutes = time.minutes;
	new_lap->seconds = time.seconds;
	new_lap->centiseconds = time.centiseconds;

	spin_lock_irqsave(&priv->lock, flags);
	list_add_tail(&new_lap->list, &priv->lap_list);

	// Reinitialize read lap pointer if it was pointing to head
	if (list_entry_is_head(priv->read_lap_ptr, &priv->lap_list, list)) {
		priv->read_lap_ptr = list_first_entry(&priv->lap_list,
						      struct lap_time, list);
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	start_led_blinking(priv);
}

/**
 * @brief Delete all the laps from the list
 * @param priv Private structure
 */
static void remove_laps(struct priv *priv)
{
	unsigned long flags;
	struct lap_time *lap, *tmp;

	if (list_empty(&priv->lap_list)) {
		pr_info("No laps to delete\n");
		return;
	}
	spin_lock_irqsave(&priv->lock, flags);
	list_for_each_entry_safe(lap, tmp, &priv->lap_list, list) {
		list_del(&lap->list);
		kfree(lap);
	}
	spin_unlock_irqrestore(&priv->lock, flags);
}

//---------------------------------------------------------------------------------------------------
// Sysfs operation

/**
 * @brief Sysfile to show if the chrono is running
 * @param dev device
 */
static ssize_t is_running_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n", priv->is_running);
}
static DEVICE_ATTR_RO(is_running);

/**
 * @brief Sysfile to show the current time of the chrono
 * @param dev device
 */
static ssize_t current_time_chrono_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);

	unsigned long flags;
	struct myTime time_to_display;

	spin_lock_irqsave(&priv->lock, flags);
	time_to_display = priv->timeChrono;
	spin_unlock_irqrestore(&priv->lock, flags);

	return sysfs_emit(buf, "%02u:%02u.%02u\n", time_to_display.minutes,
			  time_to_display.seconds,
			  time_to_display.centiseconds);
}
static DEVICE_ATTR_RO(current_time_chrono);

/**
 * @brief Sysfile to show the laps
 * @param dev device
 */
static ssize_t laps_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);
	struct lap_time *lap;
	ssize_t count = 0;

	list_for_each_entry(lap, &priv->lap_list, list) {
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "%02u:%02u.%02u\n", lap->minutes,
				   lap->seconds, lap->centiseconds);
	}
	return count;
}
static DEVICE_ATTR_RO(laps);

/**
 * @brief Sysfile to show the number of laps
 * @param dev device
 */
static ssize_t laps_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);
	struct lap_time *lap;
	size_t count = 0;

	list_for_each_entry(lap, &priv->lap_list, list) {
		count++;
	}

	return sysfs_emit(buf, "%zu\n", count);
}
static DEVICE_ATTR_RO(laps_count);

/**
 * @brief Sysfile to show the mode of the laps
 * @param dev device
 */
static ssize_t laps_mode_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);
	return sysfs_emit(buf, "%d\n", priv->mode);
}

/**
 * @brief Sysfile to set the mode of the laps
 * @param dev device
 * @param buf buffer
 * @param count size of the buffer
 */
static ssize_t laps_mode_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	struct priv *priv = dev_get_drvdata(dev);
	int mode;

	if (kstrtoint(buf, 10, &mode) || ((mode != 0) && (mode != 1))) {
		return -EINVAL;
	}

	priv->mode = mode;
	return count;
}

static DEVICE_ATTR_RW(laps_mode);

static struct attribute *sysfs_attrs[] = {
	&dev_attr_is_running.attr, &dev_attr_current_time_chrono.attr,
	&dev_attr_laps.attr,	   &dev_attr_laps_count.attr,
	&dev_attr_laps_mode.attr,  NULL,
};

static const struct attribute_group sysfs_group = { .attrs = sysfs_attrs,
						    .name = DEVICE_NAME };
//---------------------------------------------------------------------------------------------------
// Char device op

/**
 * @brief Read function for the char device
 * @param file file structure
 * @param buf buffer
 * @param count size of the buffer
 * @param ppos position
 */
static ssize_t drv_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	struct priv *priv =
		container_of(file->f_inode->i_cdev, struct priv, cdev);
	unsigned long flags;

	struct userReadTime timeToSend;

	if (count != sizeof(struct userReadTime)) {
		return -EINVAL;
	}

	if (list_empty(&priv->lap_list)) {
		pr_info("No laps to display\n");
		spin_lock_irqsave(&priv->lock, flags);
		timeToSend = (struct userReadTime){
			.currentTime = priv->timeChrono,
			.isEnd = true,
		};
		spin_unlock_irqrestore(&priv->lock, flags);
		if (copy_to_user(buf, &timeToSend,
				 sizeof(struct userReadTime))) {
			return -EFAULT;
		}
		*ppos += sizeof(struct userReadTime);
		return sizeof(struct userReadTime);
	}

	if (list_entry_is_head(priv->read_lap_ptr, &priv->lap_list, list)) {
		spin_lock_irqsave(&priv->lock, flags);
		timeToSend = (struct userReadTime){
			.currentTime = priv->timeChrono,
			.lapTime = { 0, 0, 0 },
			.isEnd = true,
		};
		spin_unlock_irqrestore(&priv->lock, flags);

		if (copy_to_user(buf, &timeToSend,
				 sizeof(struct userReadTime))) {
			return -EFAULT;
		}

		spin_lock_irqsave(&priv->lock, flags);
		priv->read_lap_ptr = list_first_entry(
			&priv->lap_list, typeof(*priv->read_lap_ptr), list);
		spin_unlock_irqrestore(&priv->lock, flags);

		*ppos += sizeof(struct userReadTime);
		return sizeof(struct userReadTime);
	}

	spin_lock_irqsave(&priv->lock, flags);
	if (priv->read_lap_ptr == NULL) {
		priv->read_lap_ptr = list_first_entry(
			&priv->lap_list, typeof(*priv->read_lap_ptr), list);
	}

	timeToSend = (struct userReadTime){

		.currentTime = priv->timeChrono,
		.lapTime = {
			.minutes = priv->read_lap_ptr->minutes,
			.seconds = priv->read_lap_ptr->seconds,
			.centiseconds = priv->read_lap_ptr->centiseconds,
		},
		.isEnd = false,
	};
	spin_unlock_irqrestore(&priv->lock, flags);
	if (copy_to_user(buf, &timeToSend, sizeof(struct userReadTime))) {
		return -EFAULT;
	}
	spin_lock_irqsave(&priv->lock, flags);
	priv->read_lap_ptr = list_next_entry(priv->read_lap_ptr, list);
	spin_unlock_irqrestore(&priv->lock, flags);

	*ppos += sizeof(struct userReadTime);

	return sizeof(struct userReadTime);
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = drv_read,
};
//---------------------------------------------------------------------------------------------------
// Others

/**
 * @brief Function to turn on or off the LEDs
 * @param priv private structure
 * @param value value to set
 * @param led led to set
 */
static void ledsOperation(struct priv *priv, enum led_state value, u32 led)
{
	u32 led_value = ioread32(priv->leds);
	led_value = (value == LED_ON) ? led_value | led : led_value & ~led;
	iowrite32(led_value, priv->leds);
}

/**
 * @brief Probe function
 */
static int chrono_probe(struct platform_device *pdev)
{
	struct priv *priv;
	struct resource *res;
	int err;

	// Memory allocation
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		pr_err("Cannot allocate memory\n");
		return -ENOMEM;
	}

	// Get the address of the register's region
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Cannot get the address of the register's region\n");
		err = -EINVAL;
		return err;
	}

	// Mapping of the physical address to a virtual address
	priv->mem_ptr = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(priv->mem_ptr)) {
		err = PTR_ERR(priv->mem_ptr);
		return err;
	}

	// Initialization of the pointers
	priv->switches = priv->mem_ptr + SWITCH_BASE;
	priv->leds = priv->mem_ptr + LEDR_BASE;
	priv->edge = priv->mem_ptr + EDGE_MASK;
	priv->hex3_hex0 = priv->mem_ptr + HEX3_HEX0_BASE;
	priv->hex5_hex4 = priv->mem_ptr + HEX5_HEX4_BASE;
	priv->interruptMask = priv->mem_ptr + INTERRUPT_MASK;

	//  Retrieve the IRQ number from the DT.
	priv->irqNum = platform_get_irq(pdev, 0);
	if (priv->irqNum < 0) {
		err = priv->irqNum;
		return err;
	}

	// Register the interrupt handler associated with the IRQ
	err = devm_request_threaded_irq(&pdev->dev, priv->irqNum, irq_handler,
					irq_thread_handler, 0, "irq_handler",
					(void *)priv);

	if (err != 0) {
		return err;
	}

	// Allocating device number
	err = alloc_chrdev_region(&priv->dev_num, 0, 1, DEVICE_NAME);
	if (err < 0) {
		pr_err("Erreur lors de l'allocation du numéro de périphérique\n");
		return err;
	}

	// Initialize cdev
	cdev_init(&priv->cdev, &fops);
	priv->cdev.owner = THIS_MODULE;

	err = cdev_add(&priv->cdev, priv->dev_num, 1);
	if (err < 0) {
		pr_err("Erreur lors de l'ajout du cdev\n");
		unregister_chrdev_region(priv->dev_num, 1);
		return err;
	}

	// Create class
	priv->cl = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(priv->cl)) {
		printk(KERN_ALERT
		       "Erreur lors de la création de la classe de périphérique\n");
		cdev_del(&priv->cdev);
		unregister_chrdev_region(priv->dev_num, 1);
		return -1;
	}

	// Create device
	priv->device =
		device_create(priv->cl, NULL, priv->dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(priv->device)) {
		pr_err("Erreur lors de la création du périphérique\n");
		class_destroy(priv->cl);
		cdev_del(&priv->cdev);
		unregister_chrdev_region(priv->dev_num, 1);
		return PTR_ERR(priv->device);
	}

	// Create sysfs group
	if (sysfs_create_group(&pdev->dev.kobj, &sysfs_group)) {
		printk(KERN_ALERT
		       "Erreur lors de l'enregistrement dans le groupe sysfile\n");
		device_destroy(priv->cl, priv->dev_num);
		class_destroy(priv->cl);
		unregister_chrdev_region(priv->dev_num, 1);
		return -1;
	}

	platform_set_drvdata(pdev, priv);

	// Initialize the workqueue
	priv->wq_chrono = create_singlethread_workqueue("chrono_wq");
	if (!priv->wq_chrono) {
		pr_err("Erreur lors de la création de la workqueue chrono\n");
		sysfs_remove_group(&pdev->dev.kobj, &sysfs_group);
		device_destroy(priv->cl, priv->dev_num);
		class_destroy(priv->cl);
		cdev_del(&priv->cdev);
		unregister_chrdev_region(priv->dev_num, 1);
		return -ENOMEM;
	}

	// Initialize the work queue
	INIT_WORK(&priv->work_chrono, update_chrono);
	// Initialize the delayed work
	INIT_DELAYED_WORK(&priv->work_delay, display_list_work);
	// Initialize the list
	INIT_LIST_HEAD(&priv->lap_list);
	// Initialize the timer
	timer_setup(&priv->led_timer, led_timer_handler, 0);
	// Initialize spinlock
	spin_lock_init(&priv->lock);
	// Initialize the read pointer
	priv->read_lap_ptr = list_first_entry_or_null(
		&priv->lap_list, typeof(*priv->read_lap_ptr), list);
	// Enable the interrupts
	iowrite32(SET_VALUE, priv->interruptMask);
	// Enable the interrupts on the edge
	iowrite32(SET_VALUE, priv->edge);
	// Prepare the display
	priv->timeChrono = (struct myTime){ 0, 0, 0 };
	display_time(priv, priv->timeChrono);

	printk(KERN_INFO "Driver probed\n");
	return 0;
}

static int chrono_remove(struct platform_device *pdev)
{
	struct priv *priv = platform_get_drvdata(pdev);
	// Stop the chrono if running
	stop_chrono(priv);

	// Delete the laps
	remove_laps(priv);

	// Delete device
	device_destroy(priv->cl, priv->dev_num);

	// Delete class
	class_destroy(priv->cl);

	// Delete cdev
	cdev_del(&priv->cdev);

	// Unregister device number
	unregister_chrdev_region(priv->dev_num, 1);

	// Delete sysfs group
	sysfs_remove_group(&pdev->dev.kobj, &sysfs_group);

	//Deleting workqueue
	cancel_work_sync(&priv->work_chrono);

	// Destroy the workqueue
	destroy_workqueue(priv->wq_chrono);

	// Delete the timer
	del_timer_sync(&priv->led_timer);

	// Destroy the delayed queue
	cancel_delayed_work_sync(&priv->work_delay);

	// Turn off the LEDs
	iowrite32(OFF_VALUE, priv->leds);
	iowrite32(OFF_VALUE, priv->hex3_hex0);
	iowrite32(OFF_VALUE, priv->hex5_hex4);

	pr_info("Driver removed \n");

	return 0;
}

static const struct of_device_id chrono_driver_id[] = {
	{ .compatible = "drv2024" },
	{ /* END */ },
};

MODULE_DEVICE_TABLE(of, chrono_driver_id);

static struct platform_driver chrono_driver = {
	.driver = { 
		.name = "drv-lab6-chrono",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(chrono_driver_id),
	},
	.probe = chrono_probe,
	.remove = chrono_remove,
};

module_platform_driver(chrono_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael Dousse");
MODULE_DESCRIPTION("Chrono driver");