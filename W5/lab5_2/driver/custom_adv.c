/***************************************************************************//**
 * @file custom_adv.c
 * @brief customize advertising
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <string.h>
#include "custom_adv.h"
#include "stdio.h"

static uint8_t to_bcd(uint8_t dec)
{
  return (uint8_t)(((dec / 10) << 4) | (dec % 10));
}

void fill_adv_packet(CustomAdv_t *pData, uint8_t flags, uint16_t companyID, uint32_t data, char *name)
{
  int n;

  pData->len_flags = 0x02;
  pData->type_flags = 0x01;
  pData->val_flags = flags;

  pData->len_manuf = 1+2+3;  // type + company ID + ss + mm + hh
  pData->type_manuf = 0xFF;
  pData->company_LO = companyID & 0xFF;
  pData->company_HI = (companyID >> 8) & 0xFF;

  /*pData->student_id_3 = student_id & 0xFF;
  pData->student_id_2 = (student_id >> 8) & 0xFF;
  pData->student_id_1 = (student_id >> 16) & 0xFF;
  pData->student_id_0 = (student_id >> 24) & 0xFF;*/
  pData->hh = to_bcd(data) & 0xFF;          // byte 0 = hours
  pData->mm = to_bcd(data >> 8) & 0xFF;   // byte 1 = minutes
  pData->ss = to_bcd(data >> 16) & 0xFF;  // byte 2 = seconds


  // Name length, excluding null terminator
  n = strlen(name);
  if (n > NAME_MAX_LENGTH) {
    // Incomplete name
    pData->type_name = 0x08;
  } else {
    pData->type_name = 0x09;
  }

  strncpy(pData->name, name, NAME_MAX_LENGTH);

  if (n > NAME_MAX_LENGTH) {
    n = NAME_MAX_LENGTH;
  }

  pData->len_name = 1 + n; // length of name element is the name string length + 1 for the AD type

  // Calculate total length of advertising data
  pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);
}

void start_adv(CustomAdv_t *pData, uint8_t advertising_set_handle)
{
  sl_status_t sc;
  // Set custom advertising payload 
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, pData->data_size, (const uint8_t *)pData);
  app_assert(sc == SL_STATUS_OK,
                "[E: 0x%04x] Failed to set advertising data\n",
                (int)sc);

  // Start advertising using custom data 
  sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_legacy_advertiser_connectable);
  app_assert(sc == SL_STATUS_OK,
                  "[E: 0x%04x] Failed to start advertising\n",
                  (int)sc);
}



void update_adv_data(CustomAdv_t *pData, uint8_t advertising_set_handle, uint32_t data)
{
  sl_status_t sc;
  // Update the variable fields in the custom advertising packet
  /*pData->student_id_3 = student_id & 0xFF;
  pData->student_id_2 = (student_id >> 8) & 0xFF;
  pData->student_id_1 = (student_id >> 16) & 0xFF;
  pData->student_id_0 = (student_id >> 24) & 0xFF;*/
  pData->hh = to_bcd(data) & 0xFF;          // byte 0 = hours
  pData->mm = to_bcd(data >> 8) & 0xFF;   // byte 1 = minutes
  pData->ss = to_bcd(data >> 16) & 0xFF;  // byte 2 = seconds


  // Set custom advertising payload 
  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle, 0, pData->data_size, (const uint8_t *)pData);
  app_assert(sc == SL_STATUS_OK,
                  "[E: 0x%04x] Failed to set advertising data\n",
                  (int)sc);
}
