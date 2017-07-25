#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "print_int.h"

int __init init_sample( void )
{
	printk( KERN_ALERT "print_integer( 99 ) called\n" );
	print_integer( 99 );

	return 0;
}

void __exit exit_sample( void )
{
	printk( KERN_ALERT "Good-bye~\n" );
}

module_init( init_sample );
module_exit( exit_sample );

MODULE_LICENSE( "GPL" );
