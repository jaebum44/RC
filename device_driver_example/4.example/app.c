#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define led_dev "/dev/gpioled"

int main( void )
{
	int		dev;
	unsigned char	data = 0;

	dev = open( led_dev, O_RDWR );

	if( dev < 0 )
	{
		fprintf( stderr, "cannot open LED Device ( %d )", dev );

		exit( 2 );
	}

	write( dev, &data, sizeof( unsigned char ) );
	usleep( 100000 );
	write( dev, "0", sizeof( unsigned char ) );
	close( dev );

	return 0;
}
