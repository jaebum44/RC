#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/io.h>
#include <asm/uaccess.h>
//#include <mach/platform.h>

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "YoungJin Suh" );
MODULE_DESCRIPTION( "Raspberry Pi GPIO LED Device Module" );

#define BCM_IO_BASE	0x3f000000
#define GPIO_BASE	( BCM_IO_BASE + 0x200000 )
#define	GPIO_SIZE	256

#define GPIO_IN( g )	( *( gpio + ( ( g ) / 10 ) )	&= ~( 7 << ( ( ( g ) % 10 ) * 3 )))
#define GPIO_OUT( g )	( *( gpio + ( ( g ) / 10 ) )	|= ( 1 << ( ( ( g ) % 10 ) * 3 )))

#define GPIO_SET( g )	( *( gpio + 7 )		= 1 << g )
#define GPIO_CLR( g )	( *( gpio + 10 )	= 1 << g )
#define GPIO_GET( g )	( *( gpio + 13 ) & ( 1 << g ) )

#define GPIO_MAJOR	200
#define GPIO_MINOR	0
#define GPIO_DEVICE	"gpioled"
#define GPIO_LED	2

volatile unsigned*	gpio;
static char		msg[ BLOCK_SIZE ] = { 0 };

static int	gpio_open( struct inode*, struct file* );
static ssize_t	gpio_read( struct file*, char*, size_t, loff_t* );
static ssize_t	gpio_write( struct file*, const char*, size_t, loff_t* );
static int	gpio_close( struct inode*, struct file* );

static struct file_operations gpio_fops = \
{
	.owner		= THIS_MODULE,
	.read		= gpio_read,
	.write		= gpio_write,
	.open		= gpio_open,
	.release	= gpio_close,
};

struct cdev gpio_cdev;

static int gpio_open( struct inode* inod, struct file* fil )
{
	printk( "GPIO Device opened( %d / %d )\n", imajor( inod ), iminor( inod ) );
	
	return 0;
}

static int gpio_close( struct inode* inod, struct file* fil )
{
	return 0;
}

static ssize_t gpio_read( struct file* inode, char* buff, size_t len, loff_t* off )
{
	int count;

	strcat( msg, "from Kernel" );

	count = copy_to_user( buff, msg, strlen( msg ) + 1 );

	return count;
}

static ssize_t gpio_write( struct file* inode, const char* buff, size_t len, loff_t* off )
{
	short count;
	
	memset( msg, 0, BLOCK_SIZE );

	count = copy_from_user( msg, buff, len );

	( !strcmp( msg, "0" ) ) ? GPIO_CLR( GPIO_LED ) : GPIO_SET( GPIO_LED );

	return count;
}

int initModule( void )
{
	dev_t		devno;
	unsigned int	count;
	static void*	map;
	int		err;

	printk( KERN_INFO "Hello module!\n" );

	devno = MKDEV( GPIO_MAJOR, GPIO_MINOR );

	register_chrdev_region( devno, 1, GPIO_DEVICE );
	cdev_init( &gpio_cdev, &gpio_fops );

	gpio_cdev.owner	= THIS_MODULE;
	count		= 1;
	err		= cdev_add( &gpio_cdev, devno, count );

	if( err < 0 )
	{
		printk( "Error : Device Add\n" );

		return -1;
	}

	printk( "'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR );
	printk( "'chmod 666 /dev/%s'\n", GPIO_DEVICE );
	
	map = ioremap( GPIO_BASE, GPIO_SIZE );

	if( !map )
	{
		printk( "Error : mapping GPIO memory\n" );
		
		iounmap( map );

		return -EBUSY;
	}

	gpio = ( volatile unsigned int* ) map;

	GPIO_IN( GPIO_LED );
	GPIO_OUT( GPIO_LED );

	return 0;
}

void cleanupModule( void )
{
	dev_t devno = MKDEV( GPIO_MAJOR, GPIO_MINOR );
	
	unregister_chrdev_region( devno, 1 );
	cdev_del( &gpio_cdev );

	if( gpio )
	{
		iounmap( gpio );
	}

	module_put( THIS_MODULE );

	printk( KERN_INFO "Good-bye module!\n" );
}

module_init( initModule );
module_exit( cleanupModule );
