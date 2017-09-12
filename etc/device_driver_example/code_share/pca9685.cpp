#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdint.h>

#define filename "/dev/i2c-1"
// 레지스터 어드레스
#define MODE1 0x00
#define MODE2 0x01
#define PRE_SCALE 0xFE
#define LED8_ON_L 0x26
#define LED8_ON_H 0x27
#define LED8_OFF_L 0x28
#define LED8_OFF_H 0x29
#define ALL_LED_ON_L 0xFA
// 내부 25MHz OSC
#define CLOCK_FREQ 25000000.0
#define PCA_ADDR 0x40

int fd;
int pca_addr = PCA_ADDR;

int reg_read8(int addr)
{
	unsigned char buffer[60] = {0};
	int temp = 0,length = 0;

	// reg addr write 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = LED8_OFF_L;
	length =1;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}

	// reg read 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	if(read(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	temp = buffer[0];
	printf("addr[%d] = %d\n",addr,temp);

}

int reg_read16(int addr)
{
	unsigned char buffer[60] = {0};
	int temp = 0 , length = 0;

	// reg addr write 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = addr;
	length =1;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	// reg read 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	if(read(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	temp = buffer[0];
		// reg addr write 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	buffer[0] = addr + 1;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	// reg read 
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	if(read(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	temp = buffer[0]<<8 | temp;
	printf("addr[%d] = %d\n",addr,temp);
}

int reg_write8(int addr, int data)
{
	unsigned char buffer[60] = {0};
	int length =2;
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}

	buffer[0] = addr;
	buffer[1] = data;
	
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}	
}

int reg_write16(int addr, int data)
{
	unsigned char buffer[60] = {0};
	int length =2;

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}

	buffer[0] = addr;
	buffer[1] = data & 0xff;

	
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}	

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}

	buffer[0] = addr+1;
	buffer[1] = (data>>8) & 0xff;

	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}	
	printf("addr[%d]= %d\n",addr,data);
}

int led_on(int fd)
{
	int time_val_on = 2047 ,time_val = 4000;
	char key;
	while(key != 'c'){
		printf("key insert :");
		key = getchar();
		if(key == 'a'){
			if(time_val_on<3800){ 
				
				time_val_on += 10;
				reg_write16(LED8_ON_L, time_val_on);
				reg_read16(LED8_ON_L);

				reg_write16(LED8_OFF_L, time_val - time_val_on);
				reg_read16(LED8_OFF_L);
			}
			else printf("값 초과\n");
		}
		else if(key == 's'){
			if(time_val_on>0){
				time_val_on -= 10;
				reg_write16(LED8_ON_L, time_val_on);
				reg_read16(LED8_ON_L);

				reg_write16(LED8_OFF_L, time_val - time_val_on);
				reg_read16(LED8_OFF_L);
			}
			else printf("값 초과\n");
		}
	}
}

int pca9685_freq(int fd)
{
	int length = 2, freq = 100, pca_addr = 0x40;
	unsigned char buffer[60] = {0};
	uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq) -1;
	printf("prescale_val = %d \n", prescale_val);
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x10;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = PRE_SCALE;
	buffer[1] = prescale_val;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x80;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode1 %x\n",buffer[0]);
	}

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE2;
	buffer[1] = 0x04;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode2 %x\n",buffer[0]);
	}
}

int pca9685_reset(int fd)
{
	unsigned char buffer[60] = {0};
	int length, pca_addr = 0x40;
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x00;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode1 %x\n",buffer[0]);
	}

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE2;
	buffer[1] = 0x04;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode2 %x\n",buffer[0]);
	}
}

void fileopen()
{
	if((fd = open(filename, O_RDWR))<0){
		printf("Failed to open the i2c bus\n");
		return ;
	}
	pca9685_reset(fd);
	pca9685_freq(fd);
	led_on(fd);
}

int main()
{
	fileopen();
	return 0;
}
