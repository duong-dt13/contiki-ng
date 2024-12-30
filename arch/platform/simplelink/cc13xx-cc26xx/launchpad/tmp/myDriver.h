#ifndef _MY_DRIVER_H_
#define _MY_DRIVER_H_

#define BUFLEN 13
#include <stdio.h>


void initCMU(void);

void initGPIO(void);

void initI2C(void);

uint8_t I2C_LeaderRead(uint8_t followerAddress, uint8_t targetAddress, uint8_t *rxBuff, uint8_t numBytes);

uint8_t I2C_LeaderWrite(uint8_t followerAddress, uint8_t targetAddress, uint8_t *txBuff, uint8_t numBytes);

bool testI2C(void);

void initUSART0(void);

void Uart0Transmit(uint16_t *buff, uint8_t len);

void Uart0TX(uint8_t *buff);

void Uart0Receive(uint8_t *buff, uint8_t len);

#endif    //  End _MY_DRIVER_H_
