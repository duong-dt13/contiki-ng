#include "em_device.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_i2c.h"
#include "pin_config.h"
#include "myDriver.h"
#include <stdio.h>
#include "max30100.h"
#define I2C_TXBUFFER_SIZE                 1
#define I2C_RXBUFFER_SIZE                 1

// Buffers
uint8_t i2c_txBuffer;
uint8_t i2c_rxBuffer;

//  Transmission flags
volatile bool i2c_startTx;

/*
 * @brief
 *    CMU initialization
 */

void initCMU(void)
{
  //  Enable clock to GPIO and USART0
  CMU_ClockEnable(cmuClock_GPIO, true);
  //CMU_ClockEnable(cmuClock_USART0, true);
  CMU_ClockEnable(cmuClock_I2C0, true);
}

void initGPIO(void)
{
  GPIO_PinModeSet(gpioPortD, 3, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortD, 2, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 7, gpioModePushPull, 0);
  GPIO_PinModeSet(gpioPortC, 2, gpioModePushPull, 0);

  GPIO_PinModeSet(gpioPortC, 5, gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(gpioPortC, 6, gpioModeWiredAndPullUpFilter, 1);
}

/*
 * brief
 *      I2C0 SETUP
 */
void initI2C(void)
{
  //  Use default settings
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  //  Config SDA and SCL pins
  //  Route I2C pins to GPIO
  GPIO->I2CROUTE[0].SDAROUTE = ((GPIO->I2CROUTE[0].SDAROUTE & ~_GPIO_I2C_SDAROUTE_MASK)
                              | (gpioPortC << _GPIO_I2C_SDAROUTE_PORT_SHIFT
                              | (5 << _GPIO_I2C_SDAROUTE_PIN_SHIFT)));
  GPIO->I2CROUTE[0].SCLROUTE = ((GPIO->I2CROUTE[0].SCLROUTE & ~_GPIO_I2C_SCLROUTE_MASK)
                              | (gpioPortC << _GPIO_I2C_SCLROUTE_PORT_SHIFT
                              | (6 << _GPIO_I2C_SCLROUTE_PIN_SHIFT)));
  GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SCLPEN | GPIO_I2C_ROUTEEN_SDAPEN;
  //  Initialization the I2C0
  I2C_Init(I2C0, &i2cInit);
  //  Set the status flags and index
  i2c_startTx = false;
  //   Enable automatic STOP on NACK
  I2C0->CTRL = I2C_CTRL_AUTOSN;
}

uint8_t I2C_LeaderRead(uint8_t followerAddress, uint8_t targetAddress, uint8_t *rxBuff, uint8_t numBytes)
{
  // Transfer structure
  uint8_t ret = 1;
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;
  // Initialize I2C transfer
  i2cTransfer.addr          = followerAddress;
  i2cTransfer.flags         = I2C_FLAG_WRITE_READ; // must write target address before reading
  i2cTransfer.buf[0].data   = &targetAddress;
  i2cTransfer.buf[0].len    = 1;
  i2cTransfer.buf[1].data   = rxBuff;
  i2cTransfer.buf[1].len    = numBytes;

  result = I2C_TransferInit(I2C0, &i2cTransfer);

  // Send data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }
  if (result != i2cTransferDone) {
    // LED1 ON and infinite while loop to indicate I2C transmission problem
    printf("False to read sensor %d\n", result);
    ret = 0;
  }
  return ret;
}


uint8_t I2C_LeaderWrite(uint8_t followerAddress, uint8_t targetAddress, uint8_t *txBuff, uint8_t numBytes)
{
  uint8_t ret = 1;
  // Transfer structure
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef result;
  uint8_t txBuffer[numBytes+1];

  txBuffer[0] = targetAddress;
  for(int i = 0; i < numBytes; i++)
  {
      txBuffer[i + 1] = txBuff[i];
  }
  i2cTransfer.addr          = followerAddress;
  i2cTransfer.flags         = I2C_FLAG_WRITE_WRITE;
  if(numBytes == 0){
    i2cTransfer.buf[0].data   = &targetAddress;
    i2cTransfer.buf[0].len    = 1;
    i2cTransfer.buf[1].data   = NULL;
    i2cTransfer.buf[1].len    = 0;    
  }
  else{
    i2cTransfer.buf[0].data   = txBuffer;
    i2cTransfer.buf[0].len    = numBytes+1;
    i2cTransfer.buf[1].data   = NULL;
    i2cTransfer.buf[1].len    = 0;      
  }
  // Initialize I2C transfer
  result = I2C_TransferInit(I2C0, &i2cTransfer);
  // Send data
  while (result == i2cTransferInProgress) {
    result = I2C_Transfer(I2C0);
  }
  
  if (result != i2cTransferDone) {
    // LED1 ON and infinite while loop to indicate I2C transmission problem
      printf("False to write sensor %d\n", result);
      ret = 0;
  }
  return ret;
}


bool testI2C(void)
{
  bool I2CWriteVerify;
  //uint8_t data[4];
  
  i2c_rxBuffer = 0x03;
  I2C_LeaderWrite(0xAE, MAX30100_LED_CONFIG, &i2c_rxBuffer, 1);
  I2C_LeaderRead(0xAE, MAX30100_LED_CONFIG, &i2c_rxBuffer, 1);
  printf("%x\n", i2c_rxBuffer);
  
  i2c_rxBuffer = 0x07;
  I2C_LeaderWrite(0xAE, MAX30100_MODE_CONFIG, &i2c_rxBuffer, 1);
  I2C_LeaderRead(0xAE, MAX30100_MODE_CONFIG, &i2c_rxBuffer, 1);
  printf("%x\n", i2c_rxBuffer);

  I2C_LeaderWrite(0xAE, MAX30100_SPO2_CONFIG, &i2c_rxBuffer, 1);
  I2C_LeaderRead(0xAE, MAX30100_SPO2_CONFIG, &i2c_rxBuffer, 1);
  printf("%x\n", i2c_rxBuffer);
  
  I2CWriteVerify = true;
  if(i2c_rxBuffer != i2c_txBuffer)
    I2CWriteVerify = false;

  return I2CWriteVerify;
}







/*
 * brief
 *      USART0 initialization
 */
// void initUSART0(void)
// {
//   //  Default asynchronous initializer (115.2 Kbps, 8N1, no follow control)
//   USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;

//   //  Route USART0 TX and RX to the board controller TX and RX pins

//   GPIO->USARTROUTE[0].TXROUTE = (PIN_C6_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
//                             | (PIN_C6_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);

//   GPIO->USARTROUTE[0].RXROUTE = (PIN_C5_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
//                             | (PIN_C5_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);

//   // Enable RX and TX signals now that they have been routed
//   GPIO->USARTROUTE[0].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN;

//   //  Configure and enable USART0
//   USART_InitAsync(USART0, &init);
// }



