#ifndef _CUSTOM_ADV_H_
#define _CUSTOM_ADV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_bt_api.h"
#include "app_assert.h"

#define NAME_MAX_LENGTH 14

 //define for packet
#define FLAG  0x06
#define COMPANY_ID  0x02FF

typedef struct
{
  //3 bytes for Flag type
  uint8_t len_flags;
  uint8_t type_flags;
  uint8_t val_flags;

  //Then use type for Manufacturer Specific Data
  uint8_t len_manuf;
  uint8_t type_manuf;
  // First two bytes must contain the manufacturer ID (little-endian order)
  uint8_t company_LO;
  uint8_t company_HI;

  //Data for Manufacturer specific data
  /*uint8_t student_id_0;
  uint8_t student_id_1;
  uint8_t student_id_2;
  uint8_t student_id_3;*/
  uint8_t led0;
  uint8_t led1;
  uint8_t count;   // 1 byte: a uint16_t here adds a padding byte to the raw packet

  // length of the name AD element is variable, adding it last to keep things simple
  uint8_t len_name;
  uint8_t type_name;

  // NAME_MAX_LENGTH must be sized so that total length of data does not exceed 31 bytes
  char name[NAME_MAX_LENGTH];

  // These values are NOT included in the actual advertising payload, just for bookkeeping
  char dummy;        // Space for null terminator
  uint8_t data_size; // Actual length of advertising data
} CustomAdv_t;

void fill_adv_packet(CustomAdv_t *pData, uint8_t flags, uint16_t companyID, uint32_t data, char *name);
void start_adv(CustomAdv_t *pData, uint8_t advertising_set_handle);
void update_adv_data(CustomAdv_t *pData, uint8_t advertising_set_handle, uint32_t data);
#ifdef __cplusplus
}
#endif

#endif
