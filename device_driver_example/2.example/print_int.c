#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

int print_integer( int value )
{
	printk( "%d\n", value );
}

int __init init_print_int( void )
{
	return 0;
}

void __exit exit_print_int( void )
{
}

EXPORT_SYMBOL( print_integer );

module_init( init_print_int );
module_exit( exit_print_int );

MODULE_LICENSE( "GPL" );
