#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xbca7617b, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x748269e3, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xfe990052, __VMLINUX_SYMBOL_STR(gpio_free) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x5e0e323e, __VMLINUX_SYMBOL_STR(netlink_kernel_release) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0xe3684a00, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x731fea20, __VMLINUX_SYMBOL_STR(__netlink_kernel_create) },
	{ 0x31ff2455, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0x41979d26, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0x53a7e037, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xf6bfbfb2, __VMLINUX_SYMBOL_STR(gpiod_to_irq) },
	{ 0x834567b5, __VMLINUX_SYMBOL_STR(gpiod_direction_output_raw) },
	{ 0x47229b5c, __VMLINUX_SYMBOL_STR(gpio_request) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xd8e484f0, __VMLINUX_SYMBOL_STR(register_chrdev_region) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0xf4fa543b, __VMLINUX_SYMBOL_STR(arm_copy_to_user) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x28cc25db, __VMLINUX_SYMBOL_STR(arm_copy_from_user) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x93b44950, __VMLINUX_SYMBOL_STR(gpiod_set_raw_value) },
	{ 0x8e865d3c, __VMLINUX_SYMBOL_STR(arm_delay_ops) },
	{ 0xb06d6add, __VMLINUX_SYMBOL_STR(gpio_to_desc) },
	{ 0xff178f6, __VMLINUX_SYMBOL_STR(__aeabi_idivmod) },
	{ 0x2196324, __VMLINUX_SYMBOL_STR(__aeabi_idiv) },
	{ 0xa83adb84, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0x52bfd1f2, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xbb81e8cd, __VMLINUX_SYMBOL_STR(netlink_unicast) },
	{ 0xcbe1909e, __VMLINUX_SYMBOL_STR(__nlmsg_put) },
	{ 0x4bec5e52, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0xec3a9034, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xdb2e6e9e, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x4f68e5c9, __VMLINUX_SYMBOL_STR(do_gettimeofday) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1A9B2A2B8EE9F3AC5B5E9EC");
