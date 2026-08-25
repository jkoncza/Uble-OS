#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Ethernet EtherTypes.
 */
#define ETHERNET_ETHERTYPE_IPV4   0x0800
#define ETHERNET_ETHERTYPE_ARP    0x0806

#define ETHERNET_HEADER_SIZE      14
#define ETHERNET_MAC_SIZE         6


#define ETHERNET_FRAME_SIZE       2048

typedef struct
{
    uint8_t data[ETHERNET_FRAME_SIZE];
    uint16_t length;

} ethernet_frame_t;

void ethernet_init(void);

bool ethernet_receive(
    ethernet_frame_t *frame
);

uint16_t ethernet_get_ethertype(
    const ethernet_frame_t *frame
);

const uint8_t *ethernet_destination_mac(
    const ethernet_frame_t *frame
);

const uint8_t *ethernet_source_mac(
    const ethernet_frame_t *frame
);

bool ethernet_is_for_us(
    const ethernet_frame_t *frame
);

bool ethernet_is_broadcast_mac(
    const uint8_t *mac
);

bool ethernet_is_multicast_mac(
    const uint8_t *mac
);

#endif
