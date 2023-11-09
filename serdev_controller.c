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
#include <linux/ioctl.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FV");
MODULE_DESCRIPTION("A LKM driver to read data from a UART video game controller");

#define MESSAGE_LEN 18
#define DEV_NAME "devDriver"
#define DRIVER_CLASS "dev_mod_class"

static dev_t Major;
static struct class *my_class;
static struct cdev my_device;

static char state[MESSAGE_LEN] = "0000 0000 0 0 0 0\0";
static char faux[MESSAGE_LEN] = "2130-2130-1-1-1-1_";
static int state_index = 0;
static char in;

/* Kernel Module's Paramerters*/
static unsigned int read_active = 1;
module_param(read_active, uint, S_IRUGO);
MODULE_PARM_DESC(read_active, "Indicate whether or not the state should be read from the physical device.");

/* Define IOCTL */
#define R_FLIP _IOW('a', 'b', int32_t *)

/* Declare the function prototypes */
static int serdev_echo_probe(struct serdev_device *serdev);
static void serdev_echo_remove(struct serdev_device *serdev);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long int handle_ioctl(struct file *, unsigned , unsigned long);

/* Map the driver actions to the file operations structure */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
	.unlocked_ioctl = handle_ioctl
};

/* Map the hardware identifier to this driver */
static struct of_device_id serdev_echo_ids[] = {
	{
		.compatible = "brightlight,echodev",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, serdev_echo_ids);

/* Map the serial probe and remove activities */
static struct serdev_device_driver serdev_echo_driver = {
	.probe = serdev_echo_probe,
	.remove = serdev_echo_remove,
	.driver = {
		.name = "serdev-echo",
		.of_match_table = serdev_echo_ids,
	},
};

/**
 * @brief This method is called whenever a character is received
 */
static int serdev_echo_recv(struct serdev_device *serdev, const unsigned char *buffer, size_t size) 
{
	printk(KERN_INFO "GameDriver: Input from serial line.\n");
	memcpy(&in, buffer, size);

	/* filter out any invalid characters that may arise from noise on the line */
	if(in != '0' && in != '1' && in != '2' && in != '3' && in != '4' && in != '5' && in != '6' && in != '7' && in != '8' && in != '9' && in != '_' && in != '-')
	{
		printk(KERN_ALERT "GameDriver: Invalid input cha read from serial line.\n");
	}
	else
	{
		state[state_index] = in;
		state_index++;
	}
	if(in == '_') /* Indicates the end of the message */
	{
		state[state_index] = '\0';
		state_index = 0;
		printk(KERN_INFO "GameDriver: Recieved: %s\n", state);
	}
	
	return size;
}

/* Map the serial recieve function */
static const struct serdev_device_ops serdev_echo_ops = {
	.receive_buf = serdev_echo_recv,
};

/**
 * @brief This function is called when the associated physical device is detected
 */
static int serdev_echo_probe(struct serdev_device *serdev) {
	int status;
	printk(KERN_INFO "GameDriver: Controller detected.\n");

	serdev_device_set_client_ops(serdev, &serdev_echo_ops);
	status = serdev_device_open(serdev);
	if(status) {
		printk(KERN_ALERT "GameDriver: Could not open the serial port.\n");
		return -status;
	}

	serdev_device_set_baudrate(serdev, 2400); /* Faulty internal clock on arduino board. This mtched baud rate of 9600 */
	serdev_device_set_flow_control(serdev, false);
	serdev_device_set_parity(serdev, SERDEV_PARITY_NONE);
	
	printk(KERN_INFO "Gamedriver: Serial connection established.\n");

	return 0;
}

/**
 * @brief This function is called when the driver is unloaded
 */
static void serdev_echo_remove(struct serdev_device *serdev) {
	printk(KERN_INFO "GameDriver: The assciated physical device has been disconnected.\n");
	serdev_device_close(serdev);
}

/**
 * @brief This function is called when the associated devce file is opened
 */
static int dev_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "GameDriver: The device file has been opened\n");
	try_module_get(THIS_MODULE); /* Keep track of the number of devices using this file */
	return 0;
}

/**
 * @brief This function is called when the associated device file is closed
 */
static int dev_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "GameDriver: The device file has been closed\n");
	module_put(THIS_MODULE); /* Inform the kernel that one less process is using this file */
	return 0;
}


/**
 * @brief This function is called when the device file is written to (action not supported)
 */
static ssize_t dev_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	printk(KERN_ALERT "GameDriver: Write is not supported.\n");
	return -EINVAL;
}

/**
 * @brief This function is called when the device file is read (query the controller state)
 */
static ssize_t dev_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	ssize_t bytes_read = 0;
	size_t bytes_to_read = 0;

	printk(KERN_INFO "GameDriver: Reading from the device file.\n");
	
	
	if(*off >= MESSAGE_LEN)
	{
		*off = 0;
		return 0;
	}

	bytes_to_read = min(len, (size_t)(MESSAGE_LEN - *off));

	if(read_active)
	{
		if(copy_to_user(buf, state + *off, bytes_to_read))
		{
			return -EFAULT;
		}
	}else
	{
		if(copy_to_user(buf, faux + *off, bytes_to_read))
		{
			return -EFAULT;
		}
	}
	*off += bytes_to_read;
	bytes_read = bytes_to_read;

	return bytes_read;
}

static long int handle_ioctl(struct file *file, unsigned cmd, unsigned long arg){
	switch(cmd){
		case R_FLIP:
			read_active = (read_active + 1) % 2;
			break;
	}
	return 0;
}

/**
 * @brief This function is called, when the module is loaded into the kernel
 */
static int __init my_init(void) {
	printk(KERN_INFO "GameDriver: Loading the driver...\n");
	state_index = 0;
	if(serdev_device_driver_register(&serdev_echo_driver)) {
		printk(KERN_ALERT "GameDriver: Error! Could not load driver\n");
		return -1;
	}

	if(alloc_chrdev_region(&Major, 0, 1, DEV_NAME) < 0)
	{
		printk(KERN_ALERT "GameDriver: Device number could not be allocated.\n");
		return -1;
	}

	printk("GameDriver: Device number --> Major: %d, Minor: %d\n", Major >> 20, Major && 0xfffff);
	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL)
	{
		printk(KERN_ALERT "GameDriver: Device class could not be created.\n");
		goto ClassError;
	}

	if(device_create(my_class, NULL, Major, NULL, DEV_NAME) == NULL)
	{
		printk(KERN_ALERT "GameDriver: Could not create device file.\n");
		goto FileError;
	}
	
	cdev_init(&my_device, &fops);
	if(cdev_add(&my_device, Major, 1) == -1)
	{
		printk(KERN_ALERT "GameDriver: Failed to register device to kernel.\n");
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
	printk(KERN_INFO "GameDriver:  Unloading driver.\n");
	serdev_device_driver_unregister(&serdev_echo_driver);

	cdev_del(&my_device);
	device_destroy(my_class, Major);
	class_destroy(my_class);
	unregister_chrdev(Major, DEV_NAME);
	
	printk(KERN_INFO "GameDriver: Finished Unloading.\n");
}


/* Register the init and exit methods properly */
module_init(my_init);
module_exit(my_exit);

//TODO: Improve check for invalid chars

