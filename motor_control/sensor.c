/*
  mydrv.c - kernel 3.0 skeleton device driver
               ioctl
 */
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <net/sock.h>
#include <linux/irq.h>
#include <linux/pci.h>
#include "ioctl_mydrv.h"

#define NETLINK_USER 31
#define DEVICE_NAME "sensor"
#define GPIO_MAJOR 		220
#define GPIO_MINOR 		0
#define GPIO_DEVICE             "sensor"              /* 디바이스 장치 파일의 이름 */
#define GPIO_ECHO				24
#define GPIO_TRIG				23
#define GPIO_SENSOR_FLAG		26		

static int switch_irq=0;
static int mydrv_major = 220;
char chardistance[20]={0};
int distance=0;
module_param(mydrv_major, int, 0);
void output_sonicburst(void);
struct task_struct *my_task=NULL;
struct timeval after, before;

void itoa(int num, char *str);
static void hello_nl_recv_msg(struct sk_buff *skb);
struct sock *nl_sk = NULL;


static irqreturn_t isr_func(int irq, void *data)
{
    do_gettimeofday( &after);
	distance =(after.tv_usec - before.tv_usec) / 58;
	itoa(distance ,chardistance);
    printk("%scm\n", chardistance);
 
        return IRQ_HANDLED;
}

	
	//		while((gpio_get_value(GPIO_ECHO) == 0));
	//        do_gettimeofday( &before);
	//		while((gpio_get_value(GPIO_ECHO) == 1));
	//	    do_gettimeofday( &after);
	

static ssize_t mydrv_open(struct inode *inode, struct file *file)
{
  printk("mydrv opened !!\n");
  return 0;
}

static ssize_t mydrv_release(struct inode *inode, struct file *file)
{
  printk("mydrv released !!\n");
  return 0;
}

static ssize_t mydrv_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
  printk("mydrv_read is invoked\n");
  return 0;

}

static ssize_t mydrv_write(struct file *filp,const char __user *buf, size_t count,
                            loff_t *f_pos)
{
  printk("mydrv_write is invoked\n");
  return 0;
}

static ssize_t mydrv_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)  
{  
  
   ioctl_buf *k_buf;
   int   i,err, size;  
      
   if( _IOC_TYPE( cmd ) != IOCTL_MAGIC ) return -EINVAL;  
   if( _IOC_NR( cmd )   >= IOCTL_MAXNR ) return -EINVAL;  

   size = _IOC_SIZE( cmd );   
 
   if( size )  
   {  
       err = -EFAULT;  
       if( _IOC_DIR( cmd ) & _IOC_READ  ) 
		err = !access_ok( VERIFY_WRITE, (void __user *) arg, size );  
       else if( _IOC_DIR( cmd ) & _IOC_WRITE ) 
		err = !access_ok( VERIFY_READ , (void __user *) arg, size );  
       if( err ) return err;          
   }  
            
   switch( cmd )  
   {  
      
       case IOCTL_MYDRV_TEST :
            printk("IOCTL_MYDRV_TEST Processed!!\n");
	    break;

       case IOCTL_MYDRV_READ :
            k_buf = kmalloc(size,GFP_KERNEL);
            for(i = 0 ;i < size;i++)
                      k_buf->data[i] = 'A' + i;
            if(copy_to_user ( (void __user *) arg, k_buf, (unsigned long ) size ))
	          return -EFAULT;   
            kfree(k_buf);
            printk("IOCTL_MYDRV_READ Processed!!\n");
	    break;
	    	       
       case IOCTL_MYDRV_WRITE :
            k_buf = kmalloc(size,GFP_KERNEL);
            if(copy_from_user(k_buf,(void __user *) arg,size))
		   return -EFAULT;
            printk("k_buf = %s\n",k_buf->data);
			if(!strcmp(k_buf->data,"0"))
				output_sonicburst();
            kfree(k_buf);
            printk("IOCTL_MYDRV_WRITE Processed!!\n");
	    break;
	                          
       case IOCTL_MYDRV_WRITE_READ : 
            k_buf = kmalloc(size,GFP_KERNEL);
            if(copy_from_user(k_buf,(void __user *) arg,size))
                return -EFAULT;
            printk("k_buf = %s\n",k_buf->data);
            
            for(i = 0 ;i < size;i++)
                      k_buf->data[i] = 'Z' - i;
            if(copy_to_user ( (void __user *) arg, k_buf, (unsigned long ) size ))
       	   return -EFAULT;
	      kfree(k_buf);
            printk("IOCTL_MYDRV_WRITE_READ Processed!!\n");
	    break;
       
      default :
            printk("Invalid IOCTL  Processed!!\n");
            break; 
    }  
  
    return 0;  
}  

//인터럽트 키보드를 누르게 되면 컨트롤러가 프로세서에 전기적 신호를 보내 운영체제에 알리게 되는데 그 전기적 신호를 인터럽트라고 합니다.
//

/* Set up the cdev structure for a device. */
static void mydrv_setup_cdev(struct cdev *dev, int minor,
		struct file_operations *fops)
{
	int err, devno = MKDEV(mydrv_major, minor);
    
	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	err = cdev_add (dev, devno, 1);
	
	if (err)
		printk (KERN_NOTICE "Error %d adding mydrv%d", err, minor);
}



static void hello_nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
	int res;
    struct sk_buff *skb_out;
    int msg_size;
    char mesg10[20]={0};
	strcpy(mesg10,chardistance);
    


    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    msg_size=strlen(mesg10);
    
    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */


    skb_out = nlmsg_new(msg_size,0);


    if(!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    } 
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);


    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh),mesg10,msg_size);
    
    res=nlmsg_unicast(nl_sk,skb_out,pid);
    
    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");
}



void itoa(int num, char *str){ 
    int i=0; 
    int radix = 10;  // 진수 
    int deg=1; 
    int cnt = 0; 

    while(1){    // 자리수의 수를 뽑는다 
        if( (num/deg) > 0) 
            cnt++; 
        else 
            break; 
        deg *= radix; 
    } 
    deg /=radix;    // deg가 기존 자리수보다 한자리 높게 카운트 되어서 한번 나누어줌  
    // EX) 1241 ->    cnt = 4; deg = 1000; 
    for(i=0; i<cnt; i++)    {    // 자리수만큼 순회 
        *(str+i) = num/deg + '0';    // 가장 큰 자리수의 수부터 뽑음 
        num -= ((num/deg) * deg);        // 뽑은 자리수의 수를 없엠 
        deg /=radix;    // 자리수 줄임 
    } 
    *(str+i) = '\0';  // 문자열끝널.. 
}  

void output_sonicburst(void)
{
        gpio_set_value( GPIO_TRIG, 1);
        udelay(100);
        gpio_set_value( GPIO_TRIG, 0);
		udelay(10);

	
		gpio_set_value( GPIO_TRIG, 1);
	     do_gettimeofday( &before);
}




static struct file_operations mydrv_fops = {
	.owner   = THIS_MODULE,
   	.read	   = mydrv_read,
    .write   = mydrv_write,
	.unlocked_ioctl   = mydrv_ioctl,
	.open    = mydrv_open,
	.release = mydrv_release,
};

struct netlink_kernel_cfg cfg = {
    .input = hello_nl_recv_msg,
};

#define MAX_MYDRV_DEV 1

static struct cdev MydrvDevs[MAX_MYDRV_DEV];

static int mydrv_init(void)
{
	int result,req;
	dev_t dev = MKDEV(mydrv_major, 0);

	/* Figure out our device number. */
	if (mydrv_major)
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	else {
		result = alloc_chrdev_region(&dev,0, 1, DEVICE_NAME);
		mydrv_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "mydrv: unable to get major %d\n", mydrv_major);
		return result;
	}
	if (mydrv_major == 0)
		mydrv_major = result;

    printk("'mknod /dev/%s c %d 0'\n", DEVICE_NAME, mydrv_major);
    printk("'chmod 666 /dev/%s'\n", DEVICE_NAME);


	gpio_request(GPIO_ECHO,"ECHO");	
	gpio_request(GPIO_TRIG,"TRIG");	
	gpio_direction_output(GPIO_TRIG, 0);
	gpio_request(GPIO_SENSOR_FLAG,"FLAG");

	switch_irq = gpio_to_irq(GPIO_ECHO);   
	mydrv_setup_cdev(MydrvDevs,0, &mydrv_fops);

    nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	if(!nl_sk)
	{
	printk(KERN_ALERT "Error creating socket.\n");
	return -10;
	}

#if 1
	
    #define IRQF_DISABLED 0x00000020
   req= request_irq(switch_irq, isr_func,IRQF_TRIGGER_FALLING | IRQF_DISABLED, "switch", NULL);
#endif								// IRQF_TRIGGER_
	

	printk("mydrv_init done\n");	
	return 0;
}

static void mydrv_exit(void)
{
	cdev_del(MydrvDevs);
	unregister_chrdev_region(MKDEV(mydrv_major, 0), 1);
	netlink_kernel_release(nl_sk);
	free_irq(switch_irq, NULL);

	gpio_free(GPIO_ECHO);
	gpio_free(GPIO_TRIG);
    module_put(THIS_MODULE);
	gpio_free(GPIO_SENSOR_FLAG);

	printk("mydrv_exit done\n");
}

module_init(mydrv_init);
module_exit(mydrv_exit);

MODULE_LICENSE("Dual BSD/GPL");
