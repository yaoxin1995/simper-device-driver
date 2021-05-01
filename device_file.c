#include <linux/slab.h>    /*kmalloc*/
#include <linux/fs.h>   /* file stuff */
#include <linux/kernel.h>   /* printk() */
#include <linux/errno.h>    /* error codes */
#include <linux/module.h>   /* THIS_MODULE */
#include <linux/cdev.h>     /* char device stuff */
#include <linux/uaccess.h>  /* copy_to_user() */

#include "device_file.h"  //local header file shoudl behind the header file in kernel


dev_t dev;
static struct class *dev_class;
static struct cdev etx_cdev;



struct xarray key_value_store;

unsigned long next_index; //next posible index in the key_value_store


// void key_value_store_init(struct xarray *array);
// long  key_exist(loff_t key);
// int key_value_store_store(struct key_value_pair *pair,long index);
// struct key_value_pair * key_value_store_read(long index);
// loff_t update_key(long index,loff_t key);
// void key_value_store_erase(void);


/*===============================================================================================*/
static ssize_t device_file_read(struct file *file_ptr, char __user *user_buffer, size_t count, loff_t *possition)
{
        struct key_value_pair *temp;
        long index;
        size_t len;

        index = key_exist(*possition);
        printk(KERN_NOTICE "Simple-driver: Device file is read at offset = %i, read bytes count = %u\n", (int)*possition, (unsigned int)count);
        if (index < 0) 
                return 0;
        temp = key_value_store_read(index);
        if (!temp->size) 
                return 0;
        len = min(count,temp->size);
        if (copy_to_user(user_buffer, temp->value, len))
                return -EFAULT;
        *possition += count;
        return len;
}



/*if key"position" is exist in xarray, change the value of this key value pair
 *if key "position" doesm't exist in xarray, create a new key value paire in it
 */
static ssize_t device_file_write(struct file *file_ptr, const char __user *user_buffer, size_t count, loff_t *possition)
{
        long index;
        char *kernel_buffer;
        struct key_value_pair *pair;

        printk(KERN_NOTICE "Simple-driver: key_value_store_store() is called.\n");
        index = key_exist(*possition);
        kernel_buffer = kmalloc(count, GFP_KERNEL);
        if (!kernel_buffer)
                printk(KERN_NOTICE "Simple-driver: allocated memory for kernel buffer failed.\n");
        if (copy_from_user(kernel_buffer, user_buffer, count)) {
                kfree(kernel_buffer);
                return -EFAULT;
        }
        pair = kmalloc(sizeof(struct key_value_pair), GFP_KERNEL);
        pair->key = *possition;
        pair->value = kernel_buffer;
        pair->size = count;
        if (!key_value_store_save(pair, index)){
                printk(KERN_NOTICE "Simple-driver: stored data to store failed.\n");
                return -EFAULT;
        }
        *possition += count;
        return count;
}


static loff_t device_file_llseek(struct file *file_ptr, loff_t new_off, int mod)
{

        long index;

        index = key_exist(new_off);
        //alocate a new key in key value store
        if (index < 0) {
                if (!update_key(index, new_off)) {
                        file_ptr->f_pos = new_off;
                        return new_off;
                }
                return -EFAULT;
        }
    //update a key in store,return old key
        file_ptr->f_pos = new_off;
        return update_key(index, new_off);
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

/* checke the key "offset" is exist in the key value store
 * if key exist ,return the index
 * if key not exist ,return -1
 */
long  key_exist(loff_t key)
{
        unsigned long index;
        struct key_value_pair *entry;

        if (xa_empty(&key_value_store))
                return -1;
        xa_for_each(&key_value_store,index,entry) {
                if(entry == NULL)
                        continue;
                else if (entry->key == key)
                        return index;
        }
        return -1;
}

// return 0: no error or return a negative errno
int key_value_store_save(struct key_value_pair *pair, long index)
{
        int err;
    // storie a new entry in the key value store
        if (index<0) {
                xa_lock_bh(&key_value_store);
                err = xa_err(__xa_store(&key_value_store, next_index, pair, GFP_KERNEL));
                if (!err)
                        next_index++;
                xa_unlock_bh(&key_value_store);
                return err; //A negative errno or 0
        }
        //change the already exist key-value in the store
        xa_store(&key_value_store, index, pair, GFP_KERNEL);
        return 0;
}


// notice : the  retured key_value_pair may contain a null char* value
struct key_value_pair *key_value_store_read(long index)
{
        return xa_load(&key_value_store, index);
}


loff_t update_key(long index, loff_t key)
{
        int err;
        struct key_value_pair *p;
        struct key_value_pair *temp;
        loff_t old_key;

        if (index < 0) {
        //create a new key in key vlaue store
                p = kmalloc(sizeof(struct key_value_pair), GFP_KERNEL);
                p->key = key;
                p->value = NULL;
                p->size = 0;
                xa_lock_bh(&key_value_store);
                err = xa_err(__xa_store(&key_value_store, next_index, p, GFP_KERNEL));
                if (!err) {
                        next_index++;
                        xa_unlock_bh(&key_value_store);
                        return key;
                }
                xa_unlock_bh(&key_value_store);
                return 0; //error 0 or key
        }
        // update the key in the key value store
        temp = xa_load(&key_value_store, index);
        old_key = temp->key;
        temp->key = key;
        return old_key;
}

void key_value_store__exit(void)
{
        unsigned long index;
        struct key_value_pair *entry;

        xa_lock(&key_value_store);
        xa_for_each(&key_value_store,index,entry){
                if(entry==NULL)
                        continue;
                kfree(entry->value);
                kfree(entry);
                __xa_erase(&key_value_store, index);
        }
        next_index=0;
        xa_unlock(&key_value_store);
}






/*=======================================================================================================*/
int register_device(void)
{
        /*Allocating Major number*/
        if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0) {
                pr_info("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

        /*Creating cdev structure*/
        cdev_init(&etx_cdev, &simple_driver_fops);

        /*Adding character device to the system*/
        if ((cdev_add(&etx_cdev, dev,1)) < 0) {
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
        key_value_store_init(&key_value_store);
        pr_info("Device Driver Insert...Done!!!\n");
        return 0;

r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
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
