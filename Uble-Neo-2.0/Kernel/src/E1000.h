#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include <stdbool.h>

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

bool e1000_init(void);

bool e1000_present(void);

const uint8_t *e1000_get_mac(void);

bool e1000_receive(uint8_t *buffer,uint16_t buffer_size,uint16_t *length);

bool e1000_send(const uint8_t *data,uint16_t length);

bool e1000_tx_complete(void);
void e1000_print_hex8(uint8_t value);

void e1000_print_hex16(uint16_t value);

void e1000_print_hex32(uint32_t value);

#endif