#include "linux/module.h"
#include "linux/fs.h"
#include "linux/cdev.h"
#include "linux/device.h"
#include "linux/printk.h"
#include "linux/kdev_t.h"
#include "linux/uaccess.h"

// #define PCD_FOR_ONE_DEV

// kernel console output macro
#undef pr_fmt
#define pr_fmt(fmt) "%s + Truong :" fmt, __func__

#define NO_OF_DEVICE 4

#ifdef PCD_FOR_ONE_DEV
	// Comment: Legacy code for setting the limited memory for 1 char device
	#define DEV_MEM_SIZE 512
	char device_buffer[DEV_MEM_SIZE];
#endif

/* pseudo device's memory (Here are 4 char devices)*/
#define MAX_MEM_SIZE_DEV_1 1024
#define MAX_MEM_SIZE_DEV_2 1024
#define MAX_MEM_SIZE_DEV_3 2048
#define MAX_MEM_SIZE_DEV_4 2048

char pcd1_buffer[MAX_MEM_SIZE_DEV_1];
char pcd2_buffer[MAX_MEM_SIZE_DEV_2];
char pcd3_buffer[MAX_MEM_SIZE_DEV_3];
char pcd4_buffer[MAX_MEM_SIZE_DEV_4];

/* Device private data struct*/
struct pcddev_priv_data {
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev pcd_cdev;
};

/* Driver private device data struct */
struct pcdriv_priv_data {
	unsigned dev_num; // Number of devices that this driver manage
	dev_t device_number; // This hold the device number
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcddev_priv_data dev_priv_data[NO_OF_DEVICE]; // Array of device private data (pcddev_priv_data)struct
};

/* Initialize driver private data structure*/
struct pcdriv_priv_data driv_priv_data = {
	.dev_num 	= NO_OF_DEVICE,
	.dev_priv_data 	= {	
		[0] 	= {
				.buffer 		= pcd1_buffer,
				.size 			= MAX_MEM_SIZE_DEV_1,
				.serial_number 	= "000001",
				.perm 			= 0x01 // Read-only permission
				},

		[1]     = {
                .buffer         = pcd2_buffer,
                .size           = MAX_MEM_SIZE_DEV_2,
                .serial_number  = "000002",
                .perm           = 0x10 // Write-only permission
                },

		[2]     = {
                .buffer         = pcd3_buffer,
                .size           = MAX_MEM_SIZE_DEV_3,
                .serial_number  = "000003",
                .perm           = 0x11 // Read-Write permission
                },

		[3]     = {
                .buffer         = pcd4_buffer,
                .size           = MAX_MEM_SIZE_DEV_4,
                .serial_number  = "000004",
                .perm           = 0x11 // Read-Write permission
                }
		}
};

#ifdef PCD_FOR_ONE_DEV
	dev_t device_number; 
	// cdev variable
	struct cdev pcd_cdev;

	struct class *class_pcd;
	struct device *device_pcd;
#endif

#define RDONLY 	0x01
#define WRONLY 	0x10
#define RDWR 	0x11

int check_permission(int dev_file_perm, mode_t mode){
	if(dev_file_perm == RDWR) return 0;
	if((dev_file_perm == RDONLY) && ((mode & FMODE_READ) && !(mode & FMODE_WRITE))) return 0;
	if((dev_file_perm == WRONLY) && (!(mode & FMODE_READ) && (mode & FMODE_WRITE))) return 0;

	return -EPERM;
}

/* File operation's open function */
static int pcd_open(struct inode *inode_ptr, struct file *file_ptr) {
	int ret;
	int minor_n;

	/* Get the MInor Number from opend device inode*/
	minor_n = MINOR(inode_ptr->i_rdev);

	/* Get device private driver */
	struct pcddev_priv_data *pcddev_data;

	pcddev_data = container_of(inode_ptr->i_cdev, struct pcddev_priv_data, pcd_cdev);

	/* To supply other method like read, write, ... each device's data from the file object*/
	file_ptr->private_data = pcddev_data;
	
	ret = check_permission(pcddev_data->perm, file_ptr->f_mode);

	(!ret) ? pr_info("Device file open successfully") : pr_warn("Device file was opened unsuccessfully");

	return ret;
}

/* File operation's release function */
static int pcd_release(struct inode *inode_ptr, struct file *file_ptr) {
	pr_info("Release file successfully");
	return 0;
}

/* File operation's llseek implementation*/
static loff_t pcd_llseek(struct file *file_ptr, loff_t offset, int whence) {
#ifdef PCD_FOR_ONE_DEV
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
#endif
	loff_t tmp;
	struct pcddev_priv_data *pcddev_data = (struct pcddev_priv_data *)file_ptr->private_data;

	int max_size = pcddev_data->size;

	pr_info("llseek requested");
	pr_info("Current file position = %lld \n", file_ptr->f_pos);
	switch(whence) {
	case SEEK_SET:
		if((offset > max_size) || (offset < 0)) return -EINVAL;
		file_ptr->f_pos = offset;
		break;
	case SEEK_CUR:
		tmp = file_ptr->f_pos + offset;
		if((tmp > max_size) || (tmp < 0)) return -EINVAL;
		file_ptr->f_pos = tmp;
		break;
	case SEEK_END:
		tmp = max_size + offset;
		if((tmp > max_size) || (tmp < 0)) return -EINVAL;
		file_ptr->f_pos = tmp;
		break;
	default:
		return -EINVAL;
	}
	pr_info("New file position = %lld \n", file_ptr->f_pos);

	return 0;
}

static ssize_t pcd_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos) {
#ifdef PCD_FOR_ONE_DEV
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
#endif
	struct pcddev_priv_data *pcddev_data = (struct pcddev_priv_data *)filp->private_data;

	int max_size = pcddev_data->size;

	pr_info("write requested for %zu bytes \n", count);
    pr_info("Current file position = %lld \n", *f_pos);

    /* Step 1: Adjust the 'count' */
    if((*f_pos + count) > max_size) {
            count = max_size - *f_pos;
    }

	if(!count) {
		return -ENOMEM;
	}

    /* Step 2: Copy form user */
    if(copy_from_user(pcddev_data->buffer+(*f_pos), buffer, count)) {
            return -EFAULT;
    }
    /* Step 3: Update the current file position*/
    *f_pos += count;
    pr_info("Number of byte successfully written %zu \n", count);
    pr_info("Updated file position = %lld \n", *f_pos);
    return count;
}

static ssize_t pcd_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_pos) {

#ifdef PCD_FOR_ONE_DEV
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
#else
	struct pcddev_priv_data *pcddev_data = (struct pcddev_priv_data *)filp->private_data;

	int max_size = pcddev_data->size;

	pr_info("read requested for %zu bytes \n", count);
	pr_info("Current file position = %lld \n", *f_pos);

	/* Step 1: Adjust the 'count' */
	if((*f_pos + count) > max_size) {
		count = max_size - *f_pos;
	}

	/* Step 2: Copy to user */
      	if(copy_to_user(buffer, pcddev_data->buffer+(*f_pos), count)) {
		return -EFAULT;
	}

	/* Step 3: Update the current file position*/
	*f_pos += count;
	pr_info("Number of byte successfully read %zu \n", count);
	pr_info("Updated file position = %lld \n", *f_pos);
	return count;
#endif		
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

static int __init pcd_driver_init(void) 
{
	int ret;
	unsigned i;
	/* Step 1: Dynamically allocate a device number*/
	ret = alloc_chrdev_region(&driv_priv_data.device_number, 0, NO_OF_DEVICE, "pcd_devices");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		goto out;
	}

	/* Step 2: Create device class under /sys/class/ */
	driv_priv_data.class_pcd = class_create("pcd_class_test");

	if(IS_ERR(driv_priv_data.class_pcd)) {
		pr_err("Failed to register device class\n");
		ret = PTR_ERR(driv_priv_data.class_pcd);
		goto unreg_chrdev;
	}
	
	for(; i < NO_OF_DEVICE; i++) {
		pr_info("Device number <major>:<minor> = <%d>:<%d> \n", MAJOR(driv_priv_data.device_number + i), MINOR(driv_priv_data.device_number + i));

		/* Initialize the cdev structure with fops*/
		// cdev_init(&dev_priv_data.pcd_cdev, &pcd_fops);
		cdev_init(&driv_priv_data.dev_priv_data[i].pcd_cdev,&pcd_fops);

		driv_priv_data.dev_priv_data[i].pcd_cdev.owner = THIS_MODULE;

		/* Register a device (cdev structure) with VFS */
		ret = cdev_add(&driv_priv_data.dev_priv_data[i].pcd_cdev, driv_priv_data.device_number + i, 1);
		if(ret < 0) {
			pr_err("Cdev add failed\n");
			goto cdev_del;
		}

		/* Populate the sysfs with device information */
		driv_priv_data.device_pcd = device_create(driv_priv_data.class_pcd, NULL, driv_priv_data.device_number + i, NULL, "pcdev-%d", i+1);
		if (IS_ERR(driv_priv_data.device_pcd)) {
			pr_err("Device create failed\n");
			ret = PTR_ERR(driv_priv_data.device_pcd);
			goto class_del;
		}
	}

	return 0;

/* Err handling - begin*/
cdev_del:
class_del:
	for(;i>=0;i--){
		device_destroy(driv_priv_data.class_pcd, driv_priv_data.device_number+i);
		cdev_del(&driv_priv_data.dev_priv_data[i].pcd_cdev);
	}
	class_destroy(driv_priv_data.class_pcd);

unreg_chrdev:
	unregister_chrdev_region(driv_priv_data.device_number, NO_OF_DEVICE);
out:
	pr_info("Module insertion failed\n");
	return ret; 
/* Err handling - end*/
}

static void __exit pcd_driver_cleanup(void) {
	for(unsigned i = 0; i < NO_OF_DEVICE; i++){
		device_destroy(driv_priv_data.class_pcd, driv_priv_data.device_number+i);
		cdev_del(&driv_priv_data.dev_priv_data[i].pcd_cdev);
	}
	class_destroy(driv_priv_data.class_pcd);
	unregister_chrdev_region(driv_priv_data.device_number, NO_OF_DEVICE);
	pr_info("Module unloaded \n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Truong Nguyen - tangtruong2812@gmail.com");
MODULE_DESCRIPTION("This is only a pseudo character device driver");
