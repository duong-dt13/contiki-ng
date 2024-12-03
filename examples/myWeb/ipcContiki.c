#include "contiki.h"
#include "sys/log.h"
#include "max30100.h"
#include "gpio-hal-arch.h"
#include "dht11-sensor.h"
#include "netstack.h"
#include "net/routing/routing.h"
#include <stdio.h>
#include "net/ipv6/simple-udp.h"
#include "mutex.h"

process_event_t myevent;
mutex_t dataMutex;
int arr[4];
int panID;
#define LOG_MODULE "RPL BR"
#define LOG_LEVEL LOG_LEVEL_INFO
#define DHT11_GPIO_PORT (gpioPortC)
#define DHT11_GPIO_PIN  (7)

#define LOCAL_PORT  1234
#define REMOTE_PORT 4321

struct simple_udp_connection conn, conn1;
void simpleudpCallback(struct simple_udp_connection *c,
                    const uip_ipaddr_t *sourceAddr,
                    uint16_t sourcePort,
                    const uip_ipaddr_t *destAddr,
                    uint16_t destPort,
                    const uint8_t *data, uint16_t datalen)
{
    if(sourcePort == LOCAL_PORT){
        printf("Process 3 receive from Process 2\n");
    }
    if(sourcePort == REMOTE_PORT){
        printf("Process 2 receive from Process 3\n");
    }
}


PROCESS(gpioA, "process1 data");
PROCESS(gpioB, "process2 data");
PROCESS(gpioC, "process3 data");
//PROCESS(contiki_ng_br, "Contiki-NG Border Router");
AUTOSTART_PROCESSES(&gpioA, &gpioB, &gpioC);

PROCESS_THREAD(gpioA, ev, data)
{
    static struct etimer etimer;
    PROCESS_BEGIN();

    dht11_sensor.configure(DHT11_CONFIGURE_GPIO_PORT, DHT11_GPIO_PORT);
    dht11_sensor.configure(DHT11_CONFIGURE_GPIO_PIN, DHT11_GPIO_PIN);
    dht11_sensor.configure(SENSORS_HW_INIT, 0);
    etimer_set(&etimer, CLOCK_SECOND *1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
    etimer_set(&etimer, CLOCK_SECOND *5);
    myevent = process_alloc_event();
    while(1) {
        SENSORS_ACTIVATE(dht11_sensor);
        printf("%ld ", (unsigned  long)clock_time());
        switch (dht11_sensor.status(0))
        {
        case DHT11_STATUS_OKAY:
            arr[0] = dht11_sensor.value(DHT11_VALUE_HUMIDITY_INTEGER);
            arr[1] = dht11_sensor.value(DHT11_VALUE_HUMIDITY_DECIMAL);
            arr[2] = dht11_sensor.value(DHT11_VALUE_TEMPERATURE_INTEGER);
            arr[3] = dht11_sensor.value(DHT11_VALUE_TEMPERATURE_DECIMAL);
            printf("Humidity %d.%d %% ",
                dht11_sensor.value(DHT11_VALUE_HUMIDITY_INTEGER),
                dht11_sensor.value(DHT11_VALUE_HUMIDITY_DECIMAL));
            printf("Temperature = %d.%d *C\n",
                dht11_sensor.value(DHT11_VALUE_TEMPERATURE_INTEGER),
                dht11_sensor.value(DHT11_VALUE_TEMPERATURE_DECIMAL));   
            process_post(PROCESS_BROADCAST, myevent, NULL);     
            break;
        case DHT11_STATUS_CHECKSUM_FAILED:
            printf("Check sum failed\n");
            break;
        case DHT11_STATUS_TIMEOUT:
            printf("Reading timed out\n");
            break;
        default:
            break;
        }
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
        etimer_reset(&etimer);
    }
    PROCESS_END();
}

PROCESS_THREAD(gpioB, ev, data)
{
    int a;
    PROCESS_BEGIN();
    simple_udp_register(&conn, LOCAL_PORT, NULL, REMOTE_PORT, simpleudpCallback);
    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev = myevent);
        mutex_try_lock(&dataMutex);
        printf("process 2 is running\n");
        arr[0] += 10;
        arr[1] += 12;
        a = NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &panID);
        printf("Result-%d | PanID %d\n",a, panID);
        printf("Process 2: arr[3] = %d and arr[4] = %d\n", arr[2], arr[3]);
        mutex_unlock(&dataMutex);
        PROCESS_PAUSE();
    }
    PROCESS_END();
}

PROCESS_THREAD(gpioC, ev, data)
{
    PROCESS_BEGIN();
    simple_udp_register(&conn1, REMOTE_PORT, NULL, LOCAL_PORT, simpleudpCallback);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(ev = myevent);
        mutex_try_lock(&dataMutex);
        printf("process 3 is running\n");
        arr[2] += 15;
        arr[3] += 16;
        simple_udp_send(&conn1, arr, 4);
        printf("Process 3: arr[1] = %d and arr[2] = %d\n", arr[0], arr[1]);
        mutex_unlock(&dataMutex); 
        PROCESS_PAUSE();       
    }
    PROCESS_END();
}

// PROCESS_THREAD(contiki_ng_br, ev, data)
// {
//   PROCESS_BEGIN();

// #if BORDER_ROUTER_CONF_WEBSERVER
//   PROCESS_NAME(webserver_nogui_process);
//   process_start(&webserver_nogui_process, NULL);
// #endif /* BORDER_ROUTER_CONF_WEBSERVER */

//   LOG_INFO("Contiki-NG Border Router started\n");

//   PROCESS_END();
// }