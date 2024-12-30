#include "contiki.h"
#include "MAX30100.h"
#include "lib/sensors.h"
#include "sys/ctimer.h"
#include "dev/i2c-arch.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "board-conf.h"
#include "sys/log.h"
#define LOG_MODULE "Test"
#define LOG_LEVEL LOG_LEVEL_DBG
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/I2C.h>
//Set up I2C, (SDA,SCL)

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static I2C_Handle i2c_handle;

bool
sensor_init(void)
{
  bool rv;

  i2c_handle = i2c_arch_acquire(Board_I2C0);

  if(!i2c_handle) {
	printf("error\n");
	LOG_DBG("e\n");
    return false;
  }
  LOG_DBG("ok\n");
 
  uint8_t config_data[] = { MAX30100_MODE_CONFIG, 0x10 };
  LOG_DBG("%d", sizeof(config_data));
  rv = i2c_arch_write(i2c_handle, MAX30100_ADDRESS, config_data,
                      sizeof(config_data));			   
  i2c_arch_release(i2c_handle);
  
  return rv;
}

bool start(){
	bool rv;

  i2c_handle = i2c_arch_acquire(Board_I2C0);

  if(!i2c_handle) {
    return false;
  }
  
  