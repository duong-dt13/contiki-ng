#include "contiki.h"
#include "sys/log.h"
#include "gpio-hal-arch.h"
#include "myDriver.h"
#include "MAX30100.h"
#include "uart-arch.h"
/*---------------------------------------------------------------------------*/
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "frame802154.h"
#include <stdio.h>
#include "node-id.h"
#define WITH_SERVER_REPLY 1
#define UDP_CLIEN_PORT 8765
#define UDP_SERVER_PORT 5678
#define LOG_MODULE "GPIO_EXAMPLE"
#define LOG_LEVEL LOG_LEVEL_INFO
hrData_t data1 = {0,0};
uint8_t buff[15] = "hello world\n";
uint8_t buffer[2]={0};

PROCESS(gpioC, "gpioC process");
PROCESS(gpioD, "gpioD process");
PROCESS(gpioE, "gpioE process");
//  1111 0001: 0xF1 : Interrupt status
//  0000 0010: 0x02: Mode Config
//  0100 0111: 0x07: Sp02 config
//  RED:IR: 0110 0110
AUTOSTART_PROCESSES(&gpioC, &gpioD, &gpioE);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gpioC, ev, data)
{
  static struct etimer timer;
  hrData_t mydata;
  PROCESS_BEGIN();
  reset();
  setSPO2(sr100);
  setLEDs(pw1600, i50, i27);
  setHighresModeEnabled(1);
  etimer_set(&timer, CLOCK_SECOND * 4);
  while(1) {
    readSensor(&mydata);
    printf("%d   %d\n", mydata.IR, mydata.RED);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}

PROCESS_THREAD(gpioD, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();
  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 3);
  while(1) {
    printf("Hello World!!!\n");
    gpio_hal_arch_toggle_pin(gpioPortC, 7);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}

PROCESS_THREAD(gpioE, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();
  
  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 4);
  while(1) {
    printf("Bye!!!\n");
    gpio_hal_arch_toggle_pin(gpioPortC, 6);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}