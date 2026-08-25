#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include <stdbool.h>

#include "Ethernet.h"

#define IPV4_PROTOCOL_ICMP    1
#define IPV4_PROTOCOL_TCP     6
#define IPV4_PROTOCOL_UDP     17

#define IPV4_VERSION           4
#define IPV4_MIN_HEADER_SIZE   20
#define IPV4_MAX_HEADER_SIZE   60

#define IPV4_ETHERTYPE         ETHERNET_ETHERTYPE_IPV4

#define IPV4_MTU               1500

#define IPV4_MAX_PACKET_SIZE   IPV4_MTU

typedef struct
{
    uint8_t bytes[4];

} ipv4_address_t;


void ipv4_init(
    ipv4_address_t local_ip
);

ipv4_address_t ipv4_get_local_ip(
    void
);

bool ipv4_send(
    ipv4_address_t destination,
    uint8_t protocol,
    const uint8_t *payload,
    uint16_t payload_length
);

bool ipv4_process_frame(
    const ethernet_frame_t *frame
);

bool ipv4_poll(
    void
);

void ipv4_print_address(
    ipv4_address_t address
);

uint16_t ipv4_checksum(
    const uint8_t *data,
    uint16_t length
);

#endif
