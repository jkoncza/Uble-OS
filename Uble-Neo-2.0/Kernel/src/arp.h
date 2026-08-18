#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t bytes[4];
} arp_ipv4_t;

void arp_init(arp_ipv4_t local_ip);

bool arp_send_request(arp_ipv4_t target_ip);

bool arp_poll(void);

void arp_print_ip(arp_ipv4_t ip);

void arp_print_mac(const uint8_t *mac);

bool arp_lookup(arp_ipv4_t ip,uint8_t *mac_out);

void arp_print_cache(void);

#endif