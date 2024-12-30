#include "MAX30100.h"
#include "contiki-conf.h"
#include "lib/sensors.h" // sensor api
#include "sys/ctimer.h" // timer for sensor ready times
#include "ti-lib.h"
#include "board-i2c.h" // generic i2c interface
#include "sensor-common.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "string.h"
#include "log.h"
#define LOG_MODULE "Test"
#define LOG_LEVEL LOG_LEVEL_DBG

//#include "myDriver.h"

//Set up I2C, (SDA,SCL)

#define SENSOR_STATUS_DISABLED 0
#define SENSOR_STATUS_INITIALISED 1
#define SENSOR_STATUS_NOT_READY 2
#define SENSOR_STATUS_READY 3

static int enabled = SENSOR_STATUS_DISABLED;
/*---------------------------------------------------------------------------*/
/* A buffer for the raw reading from the sensor */
#define SENSOR_DATA_BUF_SIZE 8

static uint8_t sensor_value[3]; // max of three registers per type of data
/*---------------------------------------------------------------------------*/
/* Wait SENSOR_STARTUP_DELAY clock ticks for the sensor to be ready - ~80ms */
#define SENSOR_STARTUP_DELAY 3

static struct ctimer startup_timer;
/*---------------------------------------------------------------------------*/
static void notify_ready(void *not_used) {
  enabled = SENSOR_STATUS_READY;
  sensors_changed(&MAX30100);
}
/*---------------------------------------------------------------------------*/
void select_on_bus(void) {
  /* Set up I2C */
  board_i2c_select(MAX30100_ADDRESS);
}

void setLEDs(pulseWidth pw, ledCurrent red, ledCurrent ir){
  uint8_t reg;
  sensor_common_read_reg(MAX30100_SPO2_CONFIG, &reg, sizeof(reg));
  LOG_DBG("%d",reg);
  reg = reg & 0xFC; // Set LED_PW to 00
  reg = reg | pw;
  sensor_common_write_reg(MAX30100_SPO2_CONFIG, &reg, sizeof(reg));     // Mask LED_PW
  reg = (red<<4) | ir;
  sensor_common_write_reg(MAX30100_LED_CONFIG, &reg, sizeof(reg)); // write LED configs
}

void setSPO2(sampleRate sr){
  uint8_t reg;
  sensor_common_read_reg(MAX30100_SPO2_CONFIG, &reg, sizeof(reg));
  reg = reg & 0xE3; // Set SPO2_SR to 000
  reg = reg | (sr<<2);
  sensor_common_write_reg(MAX30100_SPO2_CONFIG, &reg, sizeof(reg)); // Mask SPO2_SR
  sensor_common_read_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));
  reg = reg & 0xf8; // Set Mode to 000
  reg = reg | 0x03;
  sensor_common_write_reg(MAX30100_SPO2_CONFIG, &reg, sizeof(reg)); // Mask MODE
}

int getNumSamp(void){
    uint8_t wrPtr;
    uint8_t rdPtr;
    sensor_common_read_reg(MAX30100_FIFO_WR_PTR, &wrPtr, sizeof(wrPtr));
    sensor_common_read_reg(MAX30100_FIFO_RD_PTR, &rdPtr, sizeof(rdPtr));
    return (abs( 16 + wrPtr - rdPtr ) % 16);
}

void setInterrupt(interruptSource intsrc)
{
  uint8_t reg;
  reg = (intsrc + 1) << 4;
  sensor_common_write_reg(MAX30100_INT_ENABLE, &reg, sizeof(reg));
  sensor_common_read_reg(MAX30100_INT_STATUS, &reg, sizeof(reg));
}

void setHighresModeEnabled(int enabled)
{
    uint8_t previous;
    sensor_common_read_reg(MAX30100_SPO2_CONFIG, &previous, sizeof(previous));
    if (enabled == 1)
    {
      previous = previous | MAX30100_SPC_SPO2_HI_RES_EN;
      sensor_common_write_reg(MAX30100_SPO2_CONFIG, &previous, sizeof(previous));
    }
    else
    {
      previous = previous & ~MAX30100_SPC_SPO2_HI_RES_EN;
      sensor_common_write_reg(MAX30100_SPO2_CONFIG, &previous, sizeof(previous));
    }
}

void readSensor(hrData_t *data){
  uint8_t temp[4] = {0};  // Temporary buffer for read values
  sensor_common_read_reg(MAX30100_FIFO_DATA, temp, sizeof(temp));  // Read four times from the FIFO
  data->IR = (temp[0]<<8) | temp[1];    // Combine values to get the actual number
  data->RED = (temp[2]<<8) | temp[3];   // Combine values to get the actual number
}

void shutdown(void){
  uint8_t reg;
  sensor_common_read_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));  // Get the current register
  reg = reg | 0x80;
  sensor_common_write_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));   // mask the SHDN bit
  board_i2c_shutdown();
}

void reset(void){
  uint8_t reg;
  sensor_common_read_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));  // Get the current register
  reg = reg | 0x40;
  sensor_common_write_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));   // mask the RESET bit
}

void startup(void){
  board_i2c_wakeup();
  uint8_t reg;
  sensor_common_read_reg(MAX30100_MODE_CONFIG, &reg, sizeof(reg));  // Get the current register
  reg = reg & 0x7F;
  sensor_common_write_reg( MAX30100_MODE_CONFIG, &reg, sizeof(reg));   // mask the SHDN bit
}
uint8_t getLostSample(void){
  uint8_t buffer;
  sensor_common_read_reg(MAX30100_OVRFLOW_CTR, &buffer, sizeof(buffer));
  return buffer;
}
uint8_t getRevID(void){
  uint8_t buffer;
  sensor_common_read_reg(MAX30100_REV_ID, &buffer, sizeof(buffer));
  return buffer;
}

uint8_t getPartID(void){
  uint8_t buffer;
  sensor_common_read_reg(MAX30100_PART_ID, &buffer, sizeof(buffer));
  return buffer;
}
void begin(pulseWidth pw, ledCurrent ir, sampleRate sr){
  uint8_t buffer;
  buffer = 0x02;
  sensor_common_write_reg(MAX30100_MODE_CONFIG, &buffer, sizeof(buffer)); // Heart rate only
  buffer = ir;
  sensor_common_write_reg(MAX30100_LED_CONFIG, &buffer, sizeof(buffer));
  buffer = (sr<<2)|pw;
  sensor_common_write_reg(MAX30100_SPO2_CONFIG, &buffer, sizeof(buffer));
}

long meanDiff(uint16_t M) {
  #define LM_SIZE 15
  static uint16_t LM[LM_SIZE];      // LastMeasurements
  static uint8_t index = 0;
  static uint16_t sum = 0;
  static int count = 0;
  long avg = 0;

  // keep sum updated to improve speed.
  sum -= LM[index];
  LM[index] = M;
  sum += LM[index];
  index++;
  index = index % LM_SIZE;
  if (count < LM_SIZE) count++;

  avg = sum / count;
  return avg - M;
}