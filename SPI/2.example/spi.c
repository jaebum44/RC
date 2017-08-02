#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>

#include <wiringPi.h>

#define CS_MCP3208  6        // BCM_GPIO 25

#define SPI_CHANNEL 0
#define SPI_SPEED   1000000  // 1MHz

#define DiffStart   0x04
#define SingleStart 0x06


static const uint8_t     spiBPW   = 8 ;
static const uint16_t    spiDelay = 0 ;

static uint32_t    spiSpeeds [2] ;
static int         spiFds [2] ;


int ADC_RW (int channel, unsigned char *data, int len);

int read_mcp3208_adc(unsigned char adcChannel);

int main (void)
{
  int adcChannel = 0;
  int adcValue   = 0;
  int fd ;
  int mode = 0;

  if(wiringPiSetup() == -1)
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror(errno));
    return 1 ;
  }

  mode    &= 3 ;	// Mode is 0, 1, 2 or 3
  adcChannel &= 1 ;	// Channel is 0 or 1

  if ((fd = open ("/dev/spidev0.0", O_RDWR)) < 0)
    return wiringPiFailure (WPI_ALMOST, "Unable to open SPI device: %s\n", strerror (errno)) ;


  spiSpeeds [adcChannel] = SPI_SPEED ;
  spiFds    [adcChannel] = fd ;

// Set SPI parameters.

  if (ioctl (fd, SPI_IOC_WR_MODE, &mode)            < 0)
    return wiringPiFailure (WPI_ALMOST, "SPI Mode Change failure: %s\n", strerror (errno)) ;
  
  if (ioctl (fd, SPI_IOC_WR_BITS_PER_WORD, &spiBPW) < 0)
    return wiringPiFailure (WPI_ALMOST, "SPI BPW Change failure: %s\n", strerror (errno)) ;

  if (ioctl (fd, SPI_IOC_WR_MAX_SPEED_HZ, &spiSpeeds[adcChannel])   < 0)
    return wiringPiFailure (WPI_ALMOST, "SPI Speed Change failure: %s\n", strerror (errno)) ;


  pinMode(CS_MCP3208, OUTPUT);

  while(1)
  {
    adcValue = read_mcp3208_adc(adcChannel);
    printf("adc0 Value = %u\n", adcValue);
	delay(1000);
  }

  return 0;
}


int ADC_RW (int channel, unsigned char *data, int len){
  struct spi_ioc_transfer spi ;

  channel &= 1 ;
  memset (&spi, 0, sizeof (spi)) ;

  spi.tx_buf        = (unsigned long)data ;
  spi.rx_buf        = (unsigned long)data ;
  spi.len           = len ;
  spi.delay_usecs   = spiDelay ;
  spi.speed_hz      = spiSpeeds [channel] ;
  spi.bits_per_word = spiBPW ;

  return ioctl (spiFds [channel], SPI_IOC_MESSAGE(1), &spi) ;
}


int read_mcp3208_adc(unsigned char adcChannel){
  unsigned char buff[3];
  int adcValue = 0;

  buff[0] = SingleStart | ((adcChannel & 0x07) >> 2);
  buff[1] = ((adcChannel & 0x07) << 6);
  buff[2] = 0x00;

  digitalWrite(CS_MCP3208, 0);  // Low : CS Active

  ADC_RW(SPI_CHANNEL, buff, 3);

  buff[1] = 0x0F & buff[1];
  adcValue = ( buff[1] << 8) | buff[2];

  digitalWrite(CS_MCP3208, 1);  // High : CS Inactive

  return adcValue;
}