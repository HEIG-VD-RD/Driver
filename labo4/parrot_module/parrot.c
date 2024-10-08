// SPDX-License-Identifier: GPL-2.0
/*
 * Parrot file
 * Author: REDS 
 * Modification by : Rafael Dousse
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/string.h>

#define MAJOR_NUM   98
#define MAJMIN	    MKDEV(MAJOR_NUM, 0)
#define DEVICE_NAME "parrot"

static struct cdev cdev;
static struct class *cl;

uint8_t *device_buffer;
size_t device_buffer_size = 8;

/**
 * @brief Read back previously written data in the internal buffer.
 *
 * @param filp pointer to the file descriptor in use
 * @param buf destination buffer in user space
 * @param count maximum number of data to read
 * @param ppos current position in file from which data will be read
 *              will be updated to new location
 *
 * @return Actual number of bytes read from internal buffer,
 *         or a negative error code
 */
static ssize_t parrot_read(struct file *filp, char __user *buf, size_t count,
			   loff_t *ppos)
{
	size_t available_bytes = device_buffer_size - *ppos;
	size_t read_bytes = min(count, available_bytes);

	// check if the current position is at the end of the buffer
	if (*ppos >= device_buffer_size) {
		return 0;
	}

	if (!available_bytes) {
		return 0;
	}

	// Copy data from kernel space to user space
	if (copy_to_user(buf, device_buffer + *ppos, read_bytes) != 0) {
		return -EFAULT;
	}

	*ppos += read_bytes;

	return read_bytes;
}

/**
 * @brief Write data to the internal buffer
 *
 * @param filp pointer to the file descriptor in use
 * @param buf source buffer in user space
 * @param count number of data to write in the buffer
 * @param ppos current position in file to which data will be written
 *              will be updated to new location
 *
 * @return Actual number of bytes writen to internal buffer,
 *         or a negative error code
 */
static ssize_t parrot_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	size_t new_size = *ppos + count;
	//Check if the new size is greater than 1024
	if (new_size > 1024) {
		return -EFBIG;
	}

	// Check if the current position is at the end of the buffer
	if (new_size > device_buffer_size) {
		uint8_t *new_buffer =
			krealloc(device_buffer, new_size, GFP_KERNEL);
		if (!new_buffer) {
			return -ENOMEM;
		}
		device_buffer = new_buffer;
		device_buffer_size = new_size;
	}

	// Copy data from user space to kernel space
	if (copy_from_user(device_buffer + *ppos, buf, count)) {
		return -EFAULT;
	}

	// Update the position
	*ppos += count;

	return count;
}

/**
 * @brief uevent callback to set the permission on the device file
 *
 * @param dev pointer to the device
 * @param env ueven environnement corresponding to the device
 */
static int parrot_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	// Set the permissions of the device file
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static const struct file_operations parrot_fops = {
	.owner = THIS_MODULE,
	.read = parrot_read,
	.write = parrot_write,
	.llseek = default_llseek, // Use default to enable seeking to 0
};

static int __init parrot_init(void)
{
	int err;

	device_buffer = kzalloc(8, GFP_KERNEL);

	if (device_buffer == NULL) {
		pr_err("Parrot: Error allocating memory\n");
		return -ENOMEM;
	}

	// Register the device
	err = register_chrdev_region(MAJMIN, 1, DEVICE_NAME);
	if (err != 0) {
		pr_err("Parrot: Registering char device failed\n");
		return err;
	}

	cl = class_create(THIS_MODULE, DEVICE_NAME);
	if (cl == NULL) {
		pr_err("Parrot: Error creating class\n");
		unregister_chrdev_region(MAJMIN, 1);
		return -1;
	}
	cl->dev_uevent = parrot_uevent;

	if (device_create(cl, NULL, MAJMIN, NULL, DEVICE_NAME) == NULL) {
		pr_err("Parrot: Error creating device\n");
		class_destroy(cl);
		unregister_chrdev_region(MAJMIN, 1);
		return -1;
	}

	cdev_init(&cdev, &parrot_fops);
	err = cdev_add(&cdev, MAJMIN, 1);
	if (err < 0) {
		pr_err("Parrot: Adding char device failed\n");
		device_destroy(cl, MAJMIN);
		class_destroy(cl);
		unregister_chrdev_region(MAJMIN, 1);
		return err;
	}

	pr_info("Parrot ready!\n");
	return 0;
}

static void __exit parrot_exit(void)
{
	// Unregister the device
	cdev_del(&cdev);
	device_destroy(cl, MAJMIN);
	class_destroy(cl);
	unregister_chrdev_region(MAJMIN, 1);

	pr_info("Parrot done!\n");

	// Free the allocated memory
	kfree(device_buffer);
}

MODULE_AUTHOR("REDS");
MODULE_LICENSE("GPL");

module_init(parrot_init);
module_exit(parrot_exit);
