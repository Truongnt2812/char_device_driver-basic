#include "linux/module.h"
#include "linux/fs.h"
#include "linux/cdev.h"
#include "linux/device.h"
#include "linux/printk.h"
#include "linux/kdev_t.h"
#include "linux/uaccess.h"

// kernel console output macro
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

// pseudo device's memory
#define DEV_MEM_SIZE 512
char device_buffer[DEV_MEM_SIZE];

// This hold the device number
dev_t device_number;

// cdev variable
struct cdev pcd_cdev;

static int pcd_open(struct inode *inode_ptr, struct file *file_ptr) {
	pr_info("device file open successfully");
	return 0;
}

static loff_t pcd_llseek(struct file *file_ptr, loff_t offset, int whence) {
	loff_t tmp;

	pr_info("llseek requested");
	pr_info("Current file position = %lld \n", file_ptr->f_pos);
	switch(whence) {
	case SEEK_SET:
		if((offset > DEV_MEM_SIZE) || (offset < 0)) return -EINVAL;
		file_ptr->f_pos = offset;
		break;
	case SEEK_CUR:
		tmp = file_ptr->f_pos + offset;
		if((tmp > DEV_MEM_SIZE) || (tmp < 0)) return -EINVAL;
		file_ptr->f_pos = tmp;
		break;
	case SEEK_END:
		tmp = DEV_MEM_SIZE + offset;
		if((tmp > DEV_MEM_SIZE) || (tmp < 0)) return -EINVAL;
		file_ptr->f_pos = DEV_MEM_SIZE + offset;
		break;
	default:
		return -EINVAL;
	}
	pr_info("New file position = %lld \n", file_ptr->f_pos);
	return 0;
}

static int pcd_release(struct inode *inode_ptr, struct file *file_ptr) {
	pr_info("release file successfully");
	return 0;
}

static ssize_t pcd_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos) {
	pr_info("write requested for %zu bytes \n", count);
        pr_info("Current file position = %lld \n", *f_pos);

        /* Step 1: Adjust the 'count' */
        if((*f_pos + count) > DEV_MEM_SIZE) {
                count = DEV_MEM_SIZE - *f_pos;
        }

	if(!count) {
		return -ENOMEM;
	}

        /* Step 2: Copy form user */
        if(copy_from_user(&device_buffer[*f_pos], buffer, count)) {
                return -EFAULT;
        }

        /* Step 3: Update the current file position*/
        *f_pos += count;
        pr_info("Number of byte successfully written %zu \n", count);
        pr_info("Updated file position = %lld \n", *f_pos);
        return count;
	return 0;
}

static ssize_t pcd_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos) {
	pr_info("read requested for %zu bytes \n", count);
	pr_info("Current file position = %lld \n", *f_pos);

	/* Step 1: Adjust the 'count' */
	if((*f_pos + count) > DEV_MEM_SIZE) {
		count = DEV_MEM_SIZE - *f_pos;
	}

	/* Step 2: Copy to user */
      	if(copy_to_user(buffer, &device_buffer[*f_pos], count)) {
		return -EFAULT;
	}

	/* Step 3: Update the current file position*/
	*f_pos += count;
	pr_info("Number of byte successfully read %zu \n", count);
	pr_info("Updated file position = %lld \n", *f_pos);
	return count;
}

// File operations struct of the driver
static const struct file_operations pcd_fops = {
	.owner 		= THIS_MODULE,
	.open 		= pcd_open,
	.write		= pcd_write,
	.read		= pcd_read,
	.llseek		= pcd_llseek,
	.release	= pcd_release,
};

struct class *class_pcd;

struct device *device_pcd;

static int __init pcd_driver_init(void) {
	/* Step 1: Dynamically allocate a device number*/
	alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");

	pr_info("Device number <major>:<minor> = <%d>:<%d> \n", MAJOR(device_number), MINOR(device_number));

	/* Step 2: Initialize the cdev structure with fops*/
	cdev_init(&pcd_cdev, &pcd_fops);
	pcd_cdev.owner = THIS_MODULE;

	/* Step 3: Register a device (cdev structure) with VFS */
	cdev_add(&pcd_cdev, device_number, 1);

	/* Step 4: Create device class under /sys/class/ */
	class_pcd = class_create("pcd_class_test");

	/* Step 5: Populate the sysfs with device information */
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
	return 0;
}

static void __exit pcd_driver_cleanup(void) {
	device_destroy(class_pcd, device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	pr_info("Module unloaded \n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Truong Nguyen - tangtruong2812@gmail.com");
MODULE_DESCRIPTION("This is only a pseudo character device driver");
