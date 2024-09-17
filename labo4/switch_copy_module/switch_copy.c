/**
*  @file :switch_copy.c
* @Author : REDS 
* Modification by : Rafael Dousse
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
#include <linux/uaccess.h>
#include <linux/slab.h>

#define LEDR_BASE      0x00000000
#define SWITCH_BASE    0x00000040
#define INTERRUPT_MASK 0x00000058
#define EDGE_MASK      0x0000005C
#define SET_VALUE      0xf
#define OFF_VALUE      0x0
#define HEX_1	       0x1
#define HEX_2	       0x2

MODULE_LICENSE("GPL");
MODULE_AUTHOR("REDS");
MODULE_DESCRIPTION("Introduction to the interrupt and platform drivers");

struct priv {
	// Registre for the memory mapping
	void *mem_ptr;
	// Adresse of switch
	void *switches;
	// Adresse of leds
	void *leds;
	// Adresse of the edge
	void *edge;
	// Adresse of the interrupt mask
	void *interruptMask;
	// IRQ number
	int irqNum;
};

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	pr_info("Interrupt received\n");
	struct priv *priv = (struct priv *)dev_id;

	// Read the value of the switches
	uint32_t switches = ioread32(priv->switches);
	// Read the value of the leds
	uint32_t leds = ioread32(priv->leds);
	// Read the value of the edge
	uint32_t edge = ioread32(priv->edge);
	// Read the value of the interrupt mask
	uint32_t interruptMask = ioread32(priv->interruptMask);

	// Uncomment next line to enable the rotation
	//int rotation = 0;

	iowrite32(SET_VALUE, priv->interruptMask);
	iowrite32(SET_VALUE, priv->edge);

	if (edge & HEX_1) {
		iowrite32(switches, priv->leds);
	} else if (edge & HEX_2) {
		// Special version with rotation. LSB is shifted to the MSB.
		// Uncomment next line  to enable the rotation
		/* 		if (leds & HEX_1) {
			
			rotation = 1;
		} else {
			rotation = 0;
		}
		 */
		// Shift the bits to the right
		leds >>= 1;

		// Uncomment next line  to enable the rotation
		//leds |= (rotation << 9);

		iowrite32(leds, priv->leds);
	} else {
		pr_info("nothing pressed\n");
	}

	return IRQ_HANDLED;
}

static int switch_copy_probe(struct platform_device *pdev)
{
	// Structure that represents the private data
	struct priv *priv;
	// Struc that represents a physical ressource such as memory, I/O port, IRQ, etc.
	struct resource *res;

	// Allocation of a private data structure in memory
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}
	// Store the private data in the platform device
	platform_set_drvdata(pdev, priv);

	//  Retrieve the address of the register's region from the DT.
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (!res) {
		return -EINVAL; // Invalid argument
	}

	// Mapping of the physical address to a virtual address
	priv->mem_ptr = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(priv->mem_ptr)) {
		return PTR_ERR(priv->mem_ptr);
	}

	// Initialization of the pointers
	priv->switches = priv->mem_ptr + SWITCH_BASE;
	priv->leds = priv->mem_ptr + LEDR_BASE;
	priv->edge = priv->mem_ptr + EDGE_MASK;
	priv->interruptMask = priv->mem_ptr + INTERRUPT_MASK;

	//  Retrieve the IRQ number from the DT.
	priv->irqNum = platform_get_irq(pdev, 0);
	if (priv->irqNum < 0) {
		return priv->irqNum;
	}

	// Register the interrupt handler associated with the IRQ
	int err = devm_request_irq(&pdev->dev, priv->irqNum, irq_handler,
				   IRQF_SHARED, "irq_handler", (void *)priv);
	if (err != 0) {
		return err;
	}

	// Enable the interrupts
	iowrite32(SET_VALUE, priv->interruptMask);
	iowrite32(SET_VALUE, priv->edge);

	pr_info("Switch copy driver probed\n");

	return 0;
}

static int switch_copy_remove(struct platform_device *pdev)
{
	struct priv *priv = platform_get_drvdata(pdev);

	// Turn off the LEDs
	iowrite32(OFF_VALUE, priv->leds);

	// Free the IRQ, should be done automatically
	devm_free_irq(&pdev->dev, priv->irqNum, priv);

	pr_info("Switch copy driver removed\n");

	return 0;
}

static const struct of_device_id switch_copy_driver_id[] = {
	{ .compatible = "drv2024" },
	{ /* END */ },
};

MODULE_DEVICE_TABLE(of, switch_copy_driver_id);

static struct platform_driver switch_copy_driver = {
	.driver = { 
		.name = "drv-lab4",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(switch_copy_driver_id),
	},
	.probe = switch_copy_probe,
	.remove = switch_copy_remove,
};

module_platform_driver(switch_copy_driver);
