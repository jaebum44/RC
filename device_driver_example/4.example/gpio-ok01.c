#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#define DEV_OK01_MAJOR_NUMBER	221
#define DEV_OK01_NAME		"gpio-ok01"
#define M_INPUT			0
#define M_OUTPUT		1
#define S_LOW			0
#define S_HIGH			1
#define BCM2835_PERI_BASE	0xF2000000
#define GPIO_BASE		(BCM2835_PERI_BASE + 0x00200000)

static volatile unsigned int* get_gpio_addr( void )
{
	return ( volatile unsigned int* )GPIO_BASE;
}

static int set_bits(
	volatile unsigned int*	addr,
	const unsigned int	shift,
	const unsigned int	val,
	const unsigned int	mask
)
{
	unsigned int	temp	= *addr;

	temp	&= ~( mask << shift );
	temp	|= ( val & mask ) << shift;
	*addr	= temp;

	return 0;
}

static int func_pin_16( const unsigned int mode )
{
	volatile unsigned int*	gpio	= get_gpio_addr();
	const unsigned int	sel	= 0x04;
	const unsigned int	shift	= 18;

	if( mode > 7 ) return -1;

	set_bits( gpio + sel / sizeof( unsigned int ), shift, mode, 0x7 );

	return 0;
}

static int set_pin_16( void )
{
	volatile unsigned int*	gpio	= get_gpio_addr();
	const unsigned int	sel	= 0x28;
	const unsigned int	shift	= 16;

	set_bits( gpio + sel / sizeof( unsigned int ), shift, S_HIGH, 0x1 );
	set_bits( gpio + sel / sizeof( unsigned int ), shift, S_LOW, 0x1 );

	return 0;
}

static int ok01_open( struct inode* inode, struct file* filp )
{
	if( func_pin_16( M_OUTPUT ) != 0 )
	{
		printk( "[gpio-ok01] func_pin_16() error!\n" );

		return -1;
	}
	set_pin_16();
	printk( "[gpio-ok01] ok01_open()\n" );

	return 0;
}

static int ok01_release( struct inode* inode, struct file* filp)
{
	clr_pin_16();
	printk( "[gpio-ok01] ok01_close()\n" );

	return 0;
}

static struct file_operations ok01_fops = \
{
	.owner		= THIS_MODULE,
	.open		= ok01_open,
	.release	= ok01_release,
};

static int ok01_init( void )
{
	printk( "[gpio-ok01] ok01_init()\n" );
	register_chrdev( DEV_OK01_MAJOR_NUMBER, DEV_OK01_NAME, &ok01_fops );
	
	return 0;
}

static void ok01_exit( void )
{
	printk( "[gpio-ok01] ok01_exit()\n" );
	unregister_chrdev( DEV_OK01_MAJOR_NUMBER, DEV_OK01_NAME );
}

module_init( ok01_init );
module_exit( ok01_exit );

MODULE_LICENSE( "Dual BSD/GPL" );
