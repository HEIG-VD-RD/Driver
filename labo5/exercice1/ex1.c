/**
 * @file ex1.c
 * @Author Rafael Dousse
 
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/timer.h>

#include "utils.h"

// We can only write 64 number in the FIFO
#define FIFO_SIZE  64
#define MAX_VALUE  999999
#define SHOW_VALUE 0
#define MAX_TIME   15000

static struct task_struct *thread;

struct priv {
	// MISC device file
	struct miscdevice miscdev;
	// FIFO
	struct kfifo fifo;
	// Registre for the memory mapping
	void *mem_ptr;
	// Adresse of switch
	void *switches;
	// Adresse of leds
	void *leds;
	// Adresse of the HEX0-HEX3
	void *hex3_hex0;
	// Adresse of the HEX4-HEX5
	void *hex5_hex4;
	// Adresse of the edge
	void *edge;
	// IRQ number
	int irqNum;
	// temp value
	uint32_t tmpValue;
	// There is a 0 in the FIFO
	int active;
	// Completion for the thread
	struct completion comp;
	// Timer for the display
	struct timer_list my_timer;
	// Number of elements displayed
	int numElDis;
	// Timer value
	uint16_t timerValue;
};

/**
 * Display a number on the 7 segments
*/
void display_number_on_7seg(struct priv *priv, uint32_t number)
{
	uint32_t valHex3_0;
	uint32_t valHex5_4;

	// Used to store the codes of the segments
	uint16_t segment_values[6] = { 0 };
	int i = 5;

	// In case the number is 0, I just turn off the 7 segments
	if (number == SHOW_VALUE) {
		iowrite32(number, priv->hex3_hex0);
		iowrite32(number, priv->hex5_hex4);
		return;
	}
	// Extract the digits of the number
	while (number > 0 && i >= 0) {
		int digit = number % 10;

		segment_values[i] = numbers[digit];
		number /= 10;
		i--;
	}

	// Combine the values of the segments
	valHex3_0 = segment_values[5] | (segment_values[4] << 8) |
		    (segment_values[3] << 16) | (segment_values[2] << 24);
	valHex5_4 = segment_values[1] | (segment_values[0] << 8);

	// Display the number on the 7 segments
	iowrite32(valHex3_0, priv->hex3_hex0);
	iowrite32(valHex5_4, priv->hex5_hex4);
	priv->numElDis++;
}

// --------------------- Thread and timer ---------------------

/**
 * Timer handler that call the thread
 * 
 * If there is a value in the FIFO (so if the length of the FIFO is not 0) and the display is active, this handler is running and call the thread
 * If that's not the case, the timer is disabled
*/
static void timer_handler(struct timer_list *t)
{
	struct priv *priv = from_timer(priv, t, my_timer);

	if (kfifo_len(&priv->fifo) && priv->active) {
		complete(&priv->comp);
		mod_timer(&priv->my_timer,
			  jiffies + msecs_to_jiffies(priv->timerValue));
	}
}

/**
 * Thread function that display the value of the FIFO
*/
static int thread_function(void *data)
{
	struct priv *priv = (struct priv *)data;
	uint32_t value;

	// Read the value of the edge
	uint32_t edge = ioread32(priv->edge);

	iowrite32(SET_VALUE, priv->edge);

	while (!kthread_should_stop()) {
		wait_for_completion_interruptible(&priv->comp);

		edge = ioread32(priv->edge);

		if (edge & HEX_2) {
			iowrite32(SET_VALUE, priv->edge);
			display_number_on_7seg(priv, OFF_VALUE);
			priv->active = 0;
			continue;
		}

		if (kfifo_out(&priv->fifo, (unsigned char *)&value,
			      sizeof(uint32_t)) < sizeof(uint32_t)) {
		}

		display_number_on_7seg(priv, value);
		// Reset the completion to wait for the next value
		reinit_completion(&priv->comp);
	}

	return 0;
}

// --------------------- SYS FILES OPERATIONS ---------------------

/**
 * Display the number of elements in the FIFO in the sys file
*/
static ssize_t fifo_count_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);
	unsigned int size = kfifo_len(&priv->fifo) / sizeof(uint32_t);

	return sysfs_emit(buf, "%u\n", size);
}

static DEVICE_ATTR_RO(fifo_count);

/**
 * Display the number of elements displayed in the sys file
*/
static ssize_t number_displayed_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%d\n", priv->numElDis);
}

static DEVICE_ATTR_RO(number_displayed);

/**
 * Display the values in the FIFO
 * 
 * If the FIFO is empty, it will return "FIFO is empty"
 * If the FIFO is not empty, it will display the values in the FIFO
 * 
*/
static ssize_t fifo_displayed_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);
	unsigned int num_elements = kfifo_len(&priv->fifo) / sizeof(uint32_t);
	uint32_t value;
	ssize_t len = 0;
	int ret;

	if (!num_elements) {
		// I know it's not one element for the sysfile but...
		return sysfs_emit(buf, "FIFO empty\n");
	}

	uint32_t *temp_buffer =
		kmalloc_array(num_elements, sizeof(uint32_t), GFP_KERNEL);
	if (!temp_buffer) {
		return -ENOMEM;
	}

	kfifo_out_peek(&priv->fifo, temp_buffer, kfifo_len(&priv->fifo));

	for (unsigned int i = 0; i < num_elements; i++) {
		if (i == num_elements - 1 || !((i + 1) % 10)) {
			ret = scnprintf(buf + len, PAGE_SIZE - len, "%u\n",
					temp_buffer[i]);
		} else {
			ret = scnprintf(buf + len, PAGE_SIZE - len, "%u ",
					temp_buffer[i]);
		}

		if (ret < 0) {
			kfree(temp_buffer);
			return -EFAULT;
		}

		len += ret;
	}

	kfree(temp_buffer);

	return len;
}

static DEVICE_ATTR_RO(fifo_displayed);

/**
 * Show the current value of the timer
*/
static ssize_t change_time_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct priv *priv = dev_get_drvdata(dev);

	// Doesnt respect 1 sys file 1 value
	// return sysfs_emit(buf, "Current value for timer %u\n", priv->timerValue);

	return sysfs_emit(buf, "%u\n", priv->timerValue);
}

/* 
 * Change the time of the timer. Max value is 15000 ms so 15 seconds
 * 
 * If the value is not in the range of 1 to 15000, it will return an error
 * If the value is in the range, it will change the value of the timer
 */
static ssize_t change_time_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	int err;
	int length;
	struct priv *priv = dev_get_drvdata(dev);
	uint16_t tmpTime;

	err = kstrtou16(buf, 10, &tmpTime);
	if (err) {
		pr_info("Error in kstrtoint\n");
		return err;
	}

	if (tmpTime > MAX_TIME) {
		pr_info("Value out of range\n");
		return -EINVAL;
	}

	pr_info("New value for timer %d\n", tmpTime);
	priv->timerValue = tmpTime;

	return count;
}

static DEVICE_ATTR(change_time, S_IWUSR | S_IRUGO, change_time_show,
		   change_time_store);

static struct attribute *show_number_attrs[] = {
	&dev_attr_fifo_count.attr, &dev_attr_number_displayed.attr,
	&dev_attr_fifo_displayed.attr, &dev_attr_change_time.attr, NULL
};

static const struct attribute_group show_number_attr_group = {
	.attrs = show_number_attrs,
	.name = "show_number"
};

// --------------------- Module OPERATION ---------------------

/**
 * Write function that write a value in the FIFO
*/
static ssize_t show_number_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	uint32_t rc;

	struct priv *priv =
		container_of(file->private_data, struct priv, miscdev);

	if (count != sizeof(uint32_t)) {
		pr_info("Incorrect data size\n");
		return -EINVAL; // Ensure that the count is correct
	}

	if (copy_from_user(&priv->tmpValue, buf, sizeof(uint32_t)) != 0) {
		pr_info("Not copied\n");
		return -EFAULT;
	}

	int errUnder = 0;
	int errOver = 0;

	// Check if the value is in the range of 1 to 999999.
	// Since the value is unsigned, it can't be negative
	// and the first check is useless but we never know
	if ((priv->tmpValue < SHOW_VALUE) ||
	    (priv->tmpValue > (uint32_t)MAX_VALUE)) {
		errUnder = (&priv->tmpValue < SHOW_VALUE);
		errOver = (&priv->tmpValue > (uint32_t)MAX_VALUE);
		pr_info("Value out of range errUnder: %d, errOver: %d\n",
			errUnder, errOver);

		return 0;
	}

	// We put the value in the FIFO
	if (kfifo_in(&priv->fifo, (unsigned char *)&priv->tmpValue,
		     sizeof(uint32_t)) != sizeof(uint32_t)) {
		pr_info("Unable to put data in fifo\n");

		return -EFAULT;
	}

	// If the value is 0, we activate the timer which will activate the tread and display the value
	if (priv->tmpValue == SHOW_VALUE) {
		priv->active = 1;
		mod_timer(&priv->my_timer, jiffies);
	}

	*ppos += sizeof(uint32_t);
	return count;
}

/**
 * Read function that does nothing. Can be used to read the value of the FIFO but not asked in the lab instructions
*/
static ssize_t show_number_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	printk("Read do nothing\n");
	return 0;
}

static const struct file_operations show_number_fops = {
	.owner = THIS_MODULE,
	.write = show_number_write,
};

static struct miscdevice show_number_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "show_number",
	.fops = &show_number_fops,
};

static const struct of_device_id show_number_driver_id[] = {
	{ .compatible = "drv2024" },
	{ /* END */ },
};

static int show_number_probe(struct platform_device *pdev)
{
	// Structure that represents the private data
	struct priv *priv;
	// Struc that represents a physical ressource such as memory, I/O port, IRQ, etc.
	struct resource *res;

	int err;

	// Allocate memory for the private data
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		pr_err("Cannot allocate memory\n");
		return -ENOMEM;
	}

	//  Retrieve the address of the register's region from the DT.
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res) {
		pr_err("Cannot get the address of the register's region\n");
		err = -EINVAL;
		goto error;
	}

	priv->miscdev.name = "show_number";
	priv->miscdev.minor = MISC_DYNAMIC_MINOR;
	priv->miscdev.fops = &show_number_fops;

	// Mapping of the physical address to a virtual address
	priv->mem_ptr = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(priv->mem_ptr)) {
		err = PTR_ERR(priv->mem_ptr);
		goto error;
	}

	// Initialization of the pointers
	priv->switches = priv->mem_ptr + SWITCH_BASE;
	priv->leds = priv->mem_ptr + LEDR_BASE;
	priv->edge = priv->mem_ptr + EDGE_MASK;
	priv->hex3_hex0 = priv->mem_ptr + HEX3_HEX0_BASE;
	priv->hex5_hex4 = priv->mem_ptr + HEX5_HEX4_BASE;
	priv->tmpValue = NULL;
	priv->active = 0;
	priv->numElDis = 0;
	priv->timerValue = 1000;

	//  Retrieve the IRQ number from the DT.
	priv->irqNum = platform_get_irq(pdev, 0);
	if (priv->irqNum < 0) {
		err = priv->irqNum;
		goto error;
	}

	// Allocating the kfifo structure
	err = kfifo_alloc(&priv->fifo, FIFO_SIZE * sizeof(uint32_t),
			  GFP_KERNEL);
	if (err) {
		pr_err("Failed to allocate the KFIFO!\n");
		err = -ENOMEM;
		goto error;
	}

	// Store the private data in the platform device
	platform_set_drvdata(pdev, priv);

	err = misc_register(&priv->miscdev);
	if (err != 0) {
		pr_err("Cannot register\n");
		goto error_kfifo;
	}

	// Create the thread
	thread = kthread_run(thread_function, priv, "thread");
	if (IS_ERR(thread)) {
		pr_err("Unable to create thread 1\n");
		err = PTR_ERR(thread);
		goto error_register;
	}

	// Create sys files
	err = sysfs_create_group(&pdev->dev.kobj, &show_number_attr_group);
	if (err) {
		pr_err("Cannot create sysfs file\n");
		goto error_thread;
	}

	// Initialize the completion
	init_completion(&priv->comp);

	//Initialize the timer
	timer_setup(&priv->my_timer, timer_handler, 0);

	// Enable the interrupts on the edge
	iowrite32(SET_VALUE, priv->edge);

	// Turn on the leds to know the module is loaded
	iowrite32(LEDS_ON, priv->leds);

	pr_info("Show_number module probed\n");
	return 0;

error_sysfs:
	sysfs_remove_group(&pdev->dev.kobj, &show_number_attr_group);

error_thread:
	kthread_stop(thread);

error_register:
	misc_deregister(&priv->miscdev);

error_kfifo:
	kfifo_free(&priv->fifo);

error:
	return err;
}

static int show_number_remove(struct platform_device *pdev)
{
	struct priv *priv = platform_get_drvdata(pdev);
	misc_deregister(&priv->miscdev);

	// Stop the thread
	kthread_stop(thread);

	// Turn off the LEDs
	iowrite32(OFF_VALUE, priv->leds);
	iowrite32(OFF_VALUE, priv->hex3_hex0);
	iowrite32(OFF_VALUE, priv->hex5_hex4);
	// Free the kfifo
	kfifo_free(&priv->fifo);

	// Delete the timer
	del_timer_sync(&priv->my_timer);

	// Remove sys files
	sysfs_remove_group(&pdev->dev.kobj, &show_number_attr_group);

	pr_info("Show_number module removed\n");
	return 0;
}

MODULE_DEVICE_TABLE(of, show_number_driver_id);

static struct platform_driver show_number_driver = {
	.driver = { 
		.name = "drv-lab4",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(show_number_driver_id),
	},
	.probe = show_number_probe,
	.remove = show_number_remove,
};

module_platform_driver(show_number_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rafael Dousse");
MODULE_DESCRIPTION("show_number of labo 5 in Driver Development course");