#include "contiki.h"
#include "rail.h"
#include <ieee802154/rail_ieee802154.h>
#include "sys/log.h"
#include "gpio-hal-arch.h"
#define LOG_MODULE "TEST-MGM240"
#define LOG_LEVEL LOG_LEVEL_INFO
static RAILSched_Config_t rail_sched_config;
static RAIL_Config_t rail_config = {
  .eventsCallback = NULL,
  .scheduler = &rail_sched_config,
};
static RAIL_Handle_t railHandle = NULL; // Initialized somewhere else.
static const RAIL_IEEE802154_Config_t rail154Config = {
  .addresses = NULL,
  .ackConfig = {
    .enable = true,     // Turn on auto ACK for IEEE 802.15.4.
    .ackTimeout = 864,  // 54 symbols * 16 us/symbol = 864 us.
    .rxTransitions = {
      .success = RAIL_RF_STATE_TX,  // Go to TX to send the ACK.
      .error = RAIL_RF_STATE_RX,    // For an always-on device stay in RX.
    },
    .txTransitions = {
      .success = RAIL_RF_STATE_RX,  // Go to RX for receiving the ACK.
      .error = RAIL_RF_STATE_RX,    // For an always-on device stay in RX.
    },
  },
  .timings = {
    .idleToRx = 100,
    .idleToTx = 100,
    .rxToTx = 192,    // 12 symbols * 16 us/symbol = 192 us
    .txToRx = 192,    // 12 symbols * 16 us/symbol = 192 us
    .rxSearchTimeout = 0, // Not used
    .txToRxSearchTimeout = 0, // Not used
  },
  .framesMask = RAIL_IEEE802154_ACCEPT_STANDARD_FRAMES,
  .promiscuousMode = false,  // Enable format and address filtering.
  .isPanCoordinator = false,
};
typedef enum {
  TX_IDLE,
  TX_SENDING,
  TX_SENT,
  TX_NO_ACK,
  TX_CHANNEL_BUSY,
  TX_ERROR
} tx_status_t;

void config154(void)
{
  // Configure the radio and channels for 2.4 GHz IEEE 802.15.4.
  RAIL_IEEE802154_Config2p4GHzRadio(railHandle);
  // Initialize the IEEE 802.15.4 configuration using the static configuration above.
  RAIL_IEEE802154_Init(railHandle, &rail154Config);
}

PROCESS(railTest, "railTest process");
AUTOSTART_PROCESSES(&railTest);
PROCESS_THREAD(railTest, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();
  railHandle = RAIL_Init(&rail_config,
                          NULL);
  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 3);
  while(1) {
    config154();
    gpio_hal_arch_toggle_pin(gpioPortC, 7);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
  PROCESS_END();
}