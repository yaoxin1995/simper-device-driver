#ifndef DEVICE_FILE_H_
#define DEVICE_FILE_H_
#include <linux/compiler.h> /* __must_check */
#include <linux/xarray.h>

static struct data{
char value[0];
size_t size;
};


__must_check int register_device(void); /* 0 if Ok*/
void unregister_device(void);

void key_value_store_init(struct xarray *array);
// long  key_exist(loff_t key);
// int key_value_store_save(struct key_value_pair *pair, long index);
// struct key_value_pair *key_value_store_read(long index);
// loff_t update_key(long index, loff_t key);
void key_value_store_erase(void);



#endif //DEVICE_FILE_H_
