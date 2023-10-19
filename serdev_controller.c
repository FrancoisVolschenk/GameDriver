#include <linux/module.h>
#include <linux/init.h>
#include <linux/serdev.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FV");
MODULE_DESCRIPTION("A LKM to read data from a UART video game controller");

#define MESSAGE_LEN 18
#define DEV_NAME "devDriver"
#define DRIVER_CLASS "dev_mod_class"

static dev_t Major;
static struct class *my_class;
static struct cdev my_device;

static char state[MESSAGE_LEN] = "0000 0000 0 0 0 0\0";
static int state_index = 0;
static char in;

/* Declate the probe and remove functions */
static int serdev_echo_probe(struct serdev_device *serdev);
static void serdev_echo_remove(struct serdev_device *serdev);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write
};

static struct of_device_id serdev_echo_ids[] = {
	{
		.compatible = "brightlight,echodev",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, serdev_echo_ids);

static struct serdev_device_driver serdev_echo_driver = {
	.probe = serdev_echo_probe,
	.remove = serdev_echo_remove,
	.driver = {
		.name = "serdev-echo",
		.of_match_table = serdev_echo_ids,
	},
};

/**
 * @brief Callback is called whenever a character is received
 */
static int serdev_echo_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size) 
{
	memcpy(&in, buffer, size);

	//(in == NULL || in == '\n')
	if(in != '0' && in != '1' && in != '2' && in != '3' && in != '4' && in != '5' && in != '6' && in != '7' && in != '8' && in != '9' && in != '_' && in != '-')
	{
		//pass
		//printk("========================================================\n");
	}
	else
	{
		state[state_index] = in;
		state_index++;
	}
	if(in == '_')
	{
		state[state_index] = '\0';
		state_index = 0;
		printk("Recieved: %s\n", state);
	}
	return size;
}

static const struct serdev_device_ops serdev_echo_ops = {
	.receive_buf = serdev_echo_recv,
};

/**
 * @brief This function is called on loading the driver 
 */
static int serdev_echo_probe(struct serdev_device *serdev) {
	int status;
	printk("serdev_echo - Now I am in the probe function!\n");

	serdev_device_set_client_ops(serdev, &serdev_echo_ops);
	status = serdev_device_open(serdev);
	if(status) {
		printk("serdev_echo - Error opening serial port!\n");
		return -status;
	}

	serdev_device_set_baudrate(serdev, 2400);
	serdev_device_set_flow_control(serdev, false);
	serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);

	status = serdev_device_write_buf(serdev, "Type something: ", sizeof("Type something: "));
	printk("serdev_echo - Wrote %d bytes.\n", status);

	return 0;
}

/**
 * @brief This function is called on unloading the driver 
 */
static void serdev_echo_remove(struct serdev_device *serdev) {
	printk("serdev_echo - Now I am in the remove function\n");
	serdev_device_close(serdev);
}

static int dev_open(struct inode *inode, struct file *filp)
{
	printk("The file has been opened\n");
	try_module_get(THIS_MODULE);
	return 0;
}

static int dev_release(struct inode *inode, struct file *filp)
{
	printk("The file has been closed\n");
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t dev_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	printk("Operation write is not supported.\n");
	return -EINVAL;
}

static ssize_t dev_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	ssize_t bytes_read = 0;
	size_t bytes_to_read = 0;

	if(*off >= MESSAGE_LEN)
	{
		*off = 0;
		return 0;
	}

	bytes_to_read = min(len, (size_t)(MESSAGE_LEN - *off));

	if(copy_to_user(buf, state + *off, bytes_to_read))
	{
		return -EFAULT;
	}

	*off += bytes_to_read;
	bytes_read = bytes_to_read;

	return bytes_read;
}

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init my_init(void) {
	printk("serdev_echo - Loading the driver...\n");
	state_index = 0;
	if(serdev_device_driver_register(&serdev_echo_driver)) {
		printk("serdev_echo - Error! Could not load driver\n");
		return -1;
	}

	if(alloc_chrdev_region(&Major, 0, 1, DEV_NAME) < 0)
	{
		printk("Device number could not be allocated.\n");
		return -1;
	}

	printk("Device number --> Major: %d, Minor: %d\n", Major >> 20, Major && 0xfffff);
	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL)
	{
		printk("Device class could not be created.\n");
		goto ClassError;
	}

	if(device_create(my_class, NULL, Major, NULL, DEV_NAME) == NULL)
	{
		printk("Could not create device file.\n");
		goto FileError;
	}
	
	cdev_init(&my_device, &fops);
	if(cdev_add(&my_device, Major, 1) == -1)
	{
		printk("Failed to register device to kernel.\n");
		goto AddError;
	}

	return 0;

	AddError:
		device_destroy(my_class, Major);
	FileError:
		class_destroy(my_class);
	ClassError:
		unregister_chrdev(Major, DEV_NAME);
		return -1;
}

/**
 * @brief This function is called, when the module is removed from the kernel
 */
static void __exit my_exit(void) {
	printk("serdev_echo - Unload driver");
	serdev_device_driver_unregister(&serdev_echo_driver);

	cdev_del(&my_device);
	device_destroy(my_class, Major);
	class_destroy(my_class);
	unregister_chrdev(Major, DEV_NAME);
}

module_init(my_init);
module_exit(my_exit);


