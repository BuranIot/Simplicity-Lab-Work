/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_log.h"
#include "driver/custom_adv.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"


CustomAdv_t sData; // Our custom advertising data stored here

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static volatile uint8_t count = 0x00;
/*********  *****************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;
  uint8_t system_id[8];

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);
      app_assert_status(sc);

      // Pad and reverse unique ID to get System ID.
      system_id[0] = address.addr[5];
      system_id[1] = address.addr[4];
      system_id[2] = address.addr[3];
      system_id[3] = 0xFF;
      system_id[4] = 0xFE;
      system_id[5] = address.addr[2];
      system_id[6] = address.addr[1];
      system_id[7] = address.addr[0];

      sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                   0,
                                                   sizeof(system_id),
                                                   system_id);
      app_assert_status(sc);


      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Set advertising interval to 1s.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        1600, // min. adv. interval (milliseconds * 1.6)
        1600, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Setting channel
      sc = sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);
      app_assert_status(sc);

      //Add data to Adv packet (LEDs off, count 0 at boot)
      fill_adv_packet(&sData, FLAG, COMPANY_ID, 0, "4951");
      app_log("fill_adv_packet completed\r\n");

      //Strart advertise
      start_adv(&sData, advertising_set_handle);
      app_log("Started advertising\r\n");
 
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Restart advertising with our custom packet (generate_data would overwrite it)
      start_adv(&sData, advertising_set_handle);
      break;

    case sl_bt_evt_system_external_signal_id: {
      // Read once so the packet and the log agree
      uint8_t led0 = sl_led_get_state(&sl_led_led0);
      uint8_t led1 = sl_led_get_state(&sl_led_led1);
      uint8_t cnt = count;
      // On air after company ID: LED0, LED1, count  (LED: 1 = on, 0 = off)
      uint32_t data = led0
                      | ((uint32_t)led1 << 8)
                      | ((uint32_t)cnt << 16);
      update_adv_data(&sData, advertising_set_handle, data);
      app_log("LED0=%d LED1=%d count=%d\r\n", led0, led1, cnt);
      break;
    }
    

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void sl_button_on_change(const sl_button_t *handle)
{
  if (sl_button_get_state(handle) != SL_SIMPLE_BUTTON_PRESSED) {
    return;  // ignore the release edge
  }
  if (handle == &sl_button_btn0) {
    sl_led_toggle(&sl_led_led0);
    count = (count + 2) % 100;              // keep 0..99: one BCD byte on air
  } else if (handle == &sl_button_btn1) {
    sl_led_toggle(&sl_led_led1);
    count = (count == 0) ? 99 : count - 1;
  }
  sl_bt_external_signal(1);
}