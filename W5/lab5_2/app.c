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

#define TICKS_PER_SECOND 32768   
#define SPEED     1       
#define RESET     2       

CustomAdv_t sData; // Our custom advertising data stored here

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
static uint32_t uptime = 0;   
static uint8_t speed = 2;     
// Pack the current uptime as hh | mm << 8 | ss << 16 (matches custom_adv.c).
static uint32_t uptime_data(void)
{
  uint32_t hh = uptime/3600;
  uint32_t mm = (uptime/60)%60;
  uint32_t ss = uptime%60;
  return hh | (mm << 8) | ((uint32_t)ss <<16);
}
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
      sl_bt_advertiser_set_channel_map(advertising_set_handle, 7);
      app_assert_status(sc);

      //Add data to Adv packet
      fill_adv_packet(&sData, FLAG, COMPANY_ID, 0, "4951"); // starts at 00:00:00
      app_log("fill_adv_packet completed\r\n");

      //Strart advertise
      start_adv(&sData, advertising_set_handle);
      app_log("Started advertising\r\n");

      // Repeating 1 s timer (handle 0, single_shot 0 = repeat)
      sc = sl_bt_system_set_lazy_soft_timer(TICKS_PER_SECOND, 0, 0, 0);
      app_assert_status(sc);

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // The soft timer fired: one counted second has passed.
    case sl_bt_evt_system_soft_timer_id:
      uptime++;
      update_adv_data(&sData, advertising_set_handle, uptime_data());
      break;

    // -------------------------------
    // A button was pressed (sent from sl_button_on_change).
    case sl_bt_evt_system_external_signal_id: {
      uint32_t signals = evt->data.evt_system_external_signal.extsignals;
      if (signals & SPEED) {
        if (speed != 4) {
          speed *= 2;
        }else{
          speed = 2;
        }
        sl_bt_system_set_lazy_soft_timer(TICKS_PER_SECOND / speed, 0, 0, 0);
      }
      if (signals & RESET) {
        uptime = 0;
        update_adv_data(&sData, advertising_set_handle, uptime_data());
      }
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
  // Interrupt context: only signal the stack, do the work in sl_bt_on_event.
  if (handle == &sl_button_btn0) {
    sl_bt_external_signal(SPEED);
  } else if (handle == &sl_button_btn1) {
    sl_bt_external_signal(RESET);
  }
}