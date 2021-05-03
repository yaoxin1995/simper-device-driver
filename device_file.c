#include <linux/slab.h>		/*kmalloc*/
#include <linux/fs.h>   /* file stuff */
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/module.h>   /* THIS_MODULE */
#include <linux/cdev.h>     /* char device stuff */
#include <linux/uaccess.h>  /* copy_to_user() */
#include <linux/mutex.h>
#include "device_file.h"  //local header file shoudl behind the header file in kernel


dev_t dev;
static struct class *dev_class;
static struct cdev etx_cdev;
static loff_t max_offset;  //max. offset of the device file,track the end of the file
struct xarray key_value_store;
struct mutex my_mutex;   // mutex for max_offset

//unsigned long next_index; //next posible index in the key_value_store


// void key_value_store_init(struct xarray *array);
// long  key_exist(loff_t key);
// int key_value_store_store(struct key_value_pair *pair,long index);
// struct key_value_pair * key_value_store_read(long index);
// loff_t update_key(long index,loff_t key);
// void key_value_store_erase(void);


/*===============================================================================================*/
static ssize_t device_file_read(struct file *file_ptr, char __user *user_buffer,
size_t count, loff_t *possition)
{
	struct data *data;
	size_t len;

	if (*possition < 0)  // loff_t: long long, index of xarry is unsighed long, invalid offset return 0
		return 0;
	data = xa_load(&key_value_store, *possition);
	if (xa_is_err(data)) {   // xa_is_err is ture if any op in xarray failed
		printk(KERN_NOTICE " xa_load FAILED in read system call at offset = %lld, read bytes count = %zu\n", *possition, count);
		return -1;
	}

	if (!data) // date point to null,no data in this possition
		return 0;
	len = min(data->size, count);
	printk(KERN_NOTICE "read value=%s, offset = %lld, read bytes count = %zu\n", data->value, *possition, count);
	if (copy_to_user(user_buffer, data->value, len))
		return -EFAULT;
	*possition += len;
	return len;
}



/*if key"position" is exist in xarray, change the value of this key value pair
 *if key "position" doesm't exist in xarray, create a new key value paire in it
 */
static ssize_t device_file_write(struct file *file_ptr, const char __user *user_buffer,
size_t count, loff_t *possition)
{
	struct data *data;
	// loff_t: long long, index of xarry is unsighed long, invalid offset return 0
	if (*possition < 0)
		return -1;
	// allocate memory for struct "data"
	data = kmalloc(sizeof(struct data) + sizeof(size_t), GFP_KERNEL);
	// kmalloc faied if it return null pointer
	if (!data)
		return -ENOMEM;
	//get data from user space
	if (copy_from_user(data->value, user_buffer, count))
		return -EFAULT;
	data->size = count;
	//store data at xarray,xa_is_err is ture, if xa_store failed
	if (xa_is_err(xa_store(&key_value_store, *possition, data, GFP_KERNEL))) {
		printk(KERN_NOTICE " xa_store FAILED in device_file_write at offset = %lld, write bytes count = %zu\n", *possition, count);
		return -1;
	}
	printk(KERN_NOTICE "write value=%s, offset = %lld, write bytes count = %zu\n", data->value, *possition, count);
	//update the offset und return the amount of input data
	*possition += count;
	if (*possition > max_offset) {
		mutex_lock(&my_mutex);
		max_offset = *possition;  // track the end of the file
		mutex_unlock(&my_mutex);
	}
	return count;
}



static loff_t device_file_llseek(struct file *file_ptr, loff_t new_off, int whence)
{
	//struct scull_dev *dev = filp->private_data;
	loff_t newpos;

	switch (whence) {
	case 0: /* SEEK_SET */
		newpos = new_off;
		break;
	case 1: /* SEEK_CUR */
		newpos = file_ptr->f_pos + new_off;
		break;
	case 2: /* SEEK_END */
		newpos = max_offset + new_off;
		break;
	default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0)
		return -EINVAL;
	file_ptr->f_pos = newpos;
	return newpos;
}

/*===============================================================================================*/
static struct file_operations simple_driver_fops = {
.owner = THIS_MODULE,
.read = device_file_read,
.write = device_file_write,
.llseek = device_file_llseek,
};

static int device_file_major_number;
static const char device_name[] = "Simple-driver";

/*=====================================  key value store function     ==========================================================*/

void key_value_store_init(struct xarray *array)
{
	xa_init_flags(array, XA_FLAGS_LOCK_BH);// Initialise an empty XArray with flags.
}


void key_value_store__exit(void)
{
	unsigned long index;
	struct key_value_pair *entry;

	xa_lock(&key_value_store);
	xa_for_each(&key_value_store, index, entry) {
		if (entry == NULL)
			continue;
		kfree(entry);
		__xa_erase(&key_value_store, index);
	}
	xa_unlock(&key_value_store);
}






/*=======================================================================================================*/
int register_device(void)
{
	/*Allocating Major number*/
	if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
		pr_info("Cannot allocate major number\n");
		return -1;
	}
	pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&etx_cdev, &simple_driver_fops);

	/*Adding character device to the system*/
	if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
		pr_info("Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	if ((dev_class = class_create(THIS_MODULE, "etx_class")) == NULL) {
		pr_info("Cannot create the struct class\n");
		goto r_class;
	}

	/*Creating device*/
	if ((device_create(dev_class, NULL, dev, NULL, "etx_device")) == NULL) {
		pr_info("Cannot create the Device 1\n");
		goto r_device;
	}
	mutex_init(&my_mutex);
	key_value_store_init(&key_value_store);
	pr_info("Device Driver Insert...Done!!!\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

/*===============================================================================================*/
void unregister_device(void)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&etx_cdev);
	unregister_chrdev_region(dev, 1);
	key_value_store__exit();
	pr_info("Device Driver Remove...Done!!!\n");
}
