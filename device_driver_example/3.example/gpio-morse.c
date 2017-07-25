#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel.h>

#define DEV_MORSE_MAJOR_NUMBER	230
#define DEV_MORSE_NAME		"gpio-morse"
#define GPIO			[DEV_MORSE_NAME]

static int morse_open( struct inode* inode, struct file* filp )
{
	printk( "GPIO morse_open()\n" );
	
	return 0;
}

static int morse_release( struct inode* inode, struct file* filp )
{
	printk( "GPIO morse_close()\n" );
	
	return 0;
}

static struct file_operations morse_fops = { 
	.owner		= THIS_MODULE,
	.open		= morse_open,
	.release	= morse_release,
};

static int morse_init( void )
{
	printk( "GPIO morse_init()\n" );
	register_chrdev( DEV_MORSE_MAJOR_NUMBER, DEV_MORSE_NAME, &morse_fops );
	
	return 0;
}

static void morse_exit( void )
{
	printk( "GPIO morse_exit()\n" );
	unregister_chrdev( DEV_MORSE_MAJOR_NUMBER, DEV_MORSE_NAME );
}

module_init( morse_init );
module_exit( morse_exit );

MODULE_LICENSE( "Dual BSD/GPL" );
