#ifndef _IOCTL_MYDRV_H_
#define _IOCTL_MYDRV_H_

#define IOCTL_MAGIC    254

typedef struct
{
	unsigned char data[26];	
	
} __attribute__ ((packed)) ioctl_buf;

#define IOCTL_MYDRV_TEST                _IO(  IOCTL_MAGIC, 0 )
#define IOCTL_MYDRV_READ                _IOR( IOCTL_MAGIC, 1 , ioctl_buf )
#define IOCTL_MYDRV_WRITE               _IOW( IOCTL_MAGIC, 2 , ioctl_buf )
#define IOCTL_MYDRV_WRITE_READ     _IOWR( IOCTL_MAGIC, 3 , ioctl_buf )

#define IOCTL_MAXNR                   4
  
#endif // _IOCTL_MYDRV_H_

