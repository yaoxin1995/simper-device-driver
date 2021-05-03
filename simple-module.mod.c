#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x557fb2c4, "module_layout" },
	{ 0xcb67e44, "cdev_del" },
	{ 0x9c16ed84, "kmalloc_caches" },
	{ 0xd101bb01, "cdev_init" },
	{ 0xdf36914b, "xa_find_after" },
	{ 0x5b3e282f, "xa_store" },
	{ 0xc44451e5, "device_destroy" },
	{ 0x8fa25c24, "xa_find" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x80b883b1, "pv_ops" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x977f511b, "__mutex_init" },
	{ 0xc5850110, "printk" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0xa85a3e6d, "xa_load" },
	{ 0x8c2e3bd5, "device_create" },
	{ 0xcd8e03ba, "cdev_add" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xdf94b07c, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x37a0cba, "kfree" },
	{ 0xe02c9c92, "__xa_erase" },
	{ 0xa60672ed, "class_destroy" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x7dd524cd, "__class_create" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0F47C5067188AFFE30866C5");
