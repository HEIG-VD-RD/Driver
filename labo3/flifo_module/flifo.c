#include <linux/module.h> /* Needed by all modules */
#include <linux/kernel.h> /* Needed for KERN_INFO */
#include <linux/init.h> /* Needed for the macros */
#include <linux/fs.h> /* Needed for file_operations */
#include <linux/slab.h> /* Needed for kmalloc */
#include <linux/uaccess.h> /* copy_(to|from)_user */
#include <linux/cdev.h> /*Needed for cdev */
#include <linux/device.h> /* Needed for device_create */

#include <linux/string.h>

#include "flifo.h"

#define MAJOR_NUM   97
#define DEVICE_NAME "flifo"

static int values[NB_VALUES];
static size_t next_in;
static size_t nb_values;
static int mode;
static int value_read = 0;

// New way to register a char device
static dev_t dev_num;
static struct cdev flifo_cdev;
static struct class *my_class;

/**
 * @brief Device file read callback to read the value in the list.
 *
 * @param filp  File structure of the char device from which the value is read.
 * @param buf   Userspace buffer to which the value will be copied.
 * @param count Number of available bytes in the userspace buffer.
 * @param ppos  Current cursor position in the file (ignored).
 *
 * @return Number of bytes written in the userspace buffer.
 */
static ssize_t flifo_read(struct file *filp, char __user *buf, size_t count,
			  loff_t *ppos)
{
	int value;

	// The buffer is empty
	if (nb_values == 0) {
		// Might not be the correct code error to return but I don't see which one to use
		return -EAGAIN;
	}

	if (count != sizeof(int)) {
		return -EINVAL;
	}

	switch (mode) {
	case MODE_FIFO:
		value = values[(NB_VALUES + next_in - nb_values) % NB_VALUES];
		break;
	case MODE_LIFO:
		value = values[(next_in - 1 + NB_VALUES) % NB_VALUES];
		break;
	default:
		return 0;
	}

	if (copy_to_user(buf, &value, sizeof(int))) {
		return -EFAULT;
	}

	if (mode == MODE_LIFO) {
		next_in--;
	}
	nb_values--;

	*ppos += sizeof(int); // Update the cursor position

	return sizeof(
		int); // Return the number of bytes written in the userspace buffer
}

/**
 * @brief Device file write callback to add a value to the list.
 *
 * @param filp  File structure of the char device to which the value is written.
 * @param buf   Userspace buffer from which the value will be copied.
 * @param count Number of available bytes in the userspace buffer.
 * @param ppos  Current cursor position in the file.
 *
 * @return Number of bytes read from the userspace buffer.
 */
static ssize_t flifo_write(struct file *filp, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	uint32_t value;

	if (count != sizeof(uint32_t)) {
		return -EINVAL;
	}

	// There is no more space in the buffer
	if (nb_values >= NB_VALUES) {
		pr_info("Buffer full, nb_values = %d, NB_VALUES = %d\n",
			nb_values, NB_VALUES);
		return -ENOSPC;
	}

	if (copy_from_user(&value, buf, sizeof(uint32_t)) != 0) {
		return -EFAULT;
	}

	values[next_in] = value;
	next_in = (next_in + 1) % NB_VALUES;
	nb_values++;

	*ppos += sizeof(uint32_t);

	return sizeof(uint32_t);
}

/**
 * @brief Device file ioctl callback. This permits to modify the behavior of the module.
 *        - If the command is FLIFO_CMD_RESET, then the list is reset.
 *        - If the command is FLIFO_CMD_CHANGE_MODE, then the arguments will determine
 *          the list's mode between FIFO (MODE_FIFO) and LIFO (MODE_LIFO)
 *
 * @param filp File structure of the char device to which ioctl is performed.
 * @param cmd  Command value of the ioctl
 * @param arg  Optionnal argument of the ioctl
 *
 * @return 0 if ioctl succeed, -1 otherwise.
 */
static long flifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case FLIFO_CMD_RESET:
		next_in = 0;
		nb_values = 0;

		break;

	case FLIFO_CMD_CHANGE_MODE:

		if (arg != MODE_FIFO && arg != MODE_LIFO) {
			return -1;
		}
		mode = arg;
		break;

	default:
		break;
	}

	return 0;
}

static int flifo_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	// Set the permissions of the device file
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

const static struct file_operations flifo_fops = {
	.owner = THIS_MODULE,
	.read = flifo_read,
	.write = flifo_write,
	.unlocked_ioctl = flifo_ioctl,
};

static int __init flifo_init(void)
{
	next_in = 0;
	nb_values = 0;
	mode = MODE_FIFO;

	// Old way to register a char device
	//register_chrdev(MAJOR_NUM, DEVICE_NAME, &flifo_fops);

	// New way to register a char device
	dev_num = MKDEV(MAJOR_NUM, 0);

	// Register the char device
	if (register_chrdev_region(dev_num, 1, DEVICE_NAME) < 0) {
		pr_err("Can't register device\n");
		return -1;
	}

	// Create a class for the device
	my_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (my_class == NULL) {
		pr_err("Can't create class\n");
		goto unregister;
	}

	// Set the permissions of the device file
	my_class->dev_uevent = flifo_uevent;

	// Create the device file
	if (device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME) == NULL) {
		pr_err("Can't create device\n");

		goto class_destroy;
	}

	// Initialize the char device
	cdev_init(&flifo_cdev, &flifo_fops);

	// Add the char device
	if (cdev_add(&flifo_cdev, dev_num, 1) < 0) {
		pr_err("Can't add cdev\n");
		goto device_destroy;
	}

	pr_info("FLIFO ready!\n");
	pr_info("ioctl FLIFO_CMD_RESET: %u\n", FLIFO_CMD_RESET);
	pr_info("ioctl FLIFO_CMD_CHANGE_MODE: %lu\n", FLIFO_CMD_CHANGE_MODE);
	pr_info(KERN_INFO
		"FLIFO device initialized with major %d and minor %d\n",
		MAJOR(dev_num), MINOR(dev_num));
	return 0;

device_destroy:
	device_destroy(my_class, dev_num);

class_destroy:
	class_destroy(my_class);

unregister:
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

	return -1;
}

static void __exit flifo_exit(void)
{
	// Old way to unregister a char device
	//unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

	cdev_del(&flifo_cdev);
	device_destroy(my_class, dev_num);
	class_destroy(my_class);
	unregister_chrdev_region(dev_num, 1);

	pr_info("FLIFO done!\n");
}

MODULE_AUTHOR("REDS");
MODULE_LICENSE("GPL");

module_init(flifo_init);
module_exit(flifo_exit);
