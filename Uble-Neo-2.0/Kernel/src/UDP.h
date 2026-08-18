#ifndef UDP_H
#define UDP_H

#include <stdint.h>
#include <stdbool.h>

#include "IPv4.h"

#define UDP_MAX_LISTENERS 16

typedef struct {
    bool active;
    uint16_t port;
} udp_listener_t;

void udp_init(void);

bool udp_poll(void);

bool udp_bind(uint16_t port);

bool udp_unbind(uint16_t port);

bool udp_is_bound(uint16_t port);

bool udp_send(uint16_t source_port,ipv4_address_t destination,uint16_t destination_port,const uint8_t *payload,uint16_t payload_length);

bool udp_receive(ipv4_address_t source,ipv4_address_t destination,const uint8_t *payload,uint16_t length);

uint16_t udp_checksum(ipv4_address_t source,ipv4_address_t destination,const uint8_t *data,uint16_t length);

#endif