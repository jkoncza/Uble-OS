#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include <stdbool.h>

#define IPV4_PROTOCOL_ICMP 1

typedef struct {
    uint8_t bytes[4];
} ipv4_address_t;

void ipv4_init(ipv4_address_t local_ip);

ipv4_address_t ipv4_get_local_ip(void);
/*
 * ================================================================
 * IPv4 TRANSMISSION
 * ================================================================
 *
 * Send an IPv4 packet.
 *
 * protocol:
 *
 *   1  = ICMP
 *   6  = TCP
 *   17 = UDP
 *
 * The destination MAC is obtained from the ARP cache.
 */

bool ipv4_send(ipv4_address_t destination,uint8_t protocol,const uint8_t *payload,uint16_t payload_length);

bool ipv4_poll(void);

void ipv4_print_address(ipv4_address_t address);

uint16_t ipv4_checksum(const uint8_t *data,uint16_t length);

#endif