#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "gpio-hal-arch.h"
#include "sys/log.h"
#include <lib/assert.h>

#include <net/routing/rpl-lite/rpl.h>
#define LOG_MODULE "CLIENT_NODE_APP"
#define LOG_LEVEL LOG_LEVEL_INFO
#define DIS_INTERVAL_SECONDS 1
#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	1234
#define UDP_SERVER_PORT	2345

#define SEND_INTERVAL		  (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;


/*---------------------------------------------------------------------------*/
PROCESS(client_gecko, "Client Gecko");
PROCESS(debug, "Debug");
AUTOSTART_PROCESSES(&client_gecko, &debug);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(client_gecko, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;
  static struct etimer et;
  PROCESS_BEGIN();
  assert(DIS_INTERVAL_SECONDS < RPL_DIO_INTERVAL_MIN);
  etimer_set(&et, CLOCK_SECOND * DIS_INTERVAL_SECONDS);
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    LOG_INFO_LLADDR(&linkaddr_node_addr); 
    LOG_INFO_("\n");
    if(NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
        
      /* Print statistics eve ry 10th TX */
      if(tx_count % 10 == 0) {
        LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                 tx_count, rx_count, missed_tx_count);
      }

      /* Send to DAG root */
      LOG_INFO("Sending request %"PRIu32" to ", tx_count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %" PRIu32 "", tx_count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      tx_count++;
    } else {

      LOG_INFO("Not reachable yet\n");
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      
      rpl_icmp6_dis_output(NULL);
      if(tx_count > 0) {
        missed_tx_count++;
      }
      etimer_reset(&et);
    }
    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(debug, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();
  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 2);
  while(1) {
    printf("Hello World!!!\n");
    gpio_hal_arch_toggle_pin(gpioPortC, 7);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}


