#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include <stdbool.h>

#include "IPv4.h"

#define ICMP_TYPE_ECHO_REPLY    0
#define ICMP_TYPE_ECHO_REQUEST  8

#define ICMP_CODE_ECHO          0

void icmp_init(void);

void icmp_print_decimal16(uint16_t value);

bool icmp_send_echo_request(ipv4_address_t destination,uint16_t identifier,uint16_t sequence);

bool icmp_receive(ipv4_address_t source,ipv4_address_t destination,const uint8_t *payload,uint16_t length);

uint16_t icmp_checksum(const uint8_t *data,uint16_t length);

#endif