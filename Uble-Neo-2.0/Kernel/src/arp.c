#include <stdint.h>
#include <stdbool.h>

#include "arp.h"
#include "E1000.h"


extern void print_string(const char *s);


/*
 * ================================================================
 * ARP CONSTANTS
 * ================================================================
 */

#define ARP_ETHERTYPE 0x0806

#define ARP_HTYPE_ETHERNET 0x0001
#define ARP_PTYPE_IPV4    0x0800

#define ARP_HLEN_ETHERNET 6
#define ARP_PLEN_IPV4     4

#define ARP_OPCODE_REQUEST 0x0001
#define ARP_OPCODE_REPLY   0x0002

#define ARP_PACKET_SIZE 28

#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_MIN_FRAME 60

#define ARP_FRAME_SIZE \
    (ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE)


/*
 * ================================================================
 * ARP STATE
 * ================================================================
 */

static arp_ipv4_t arp_local_ip;

static bool arp_initialized = false;

/*
 * ================================================================
 * ARP CACHE
 * ================================================================
 *
 * For our first implementation we use a small fixed-size table.
 *
 * Later we can add:
 *
 * - expiration
 * - replacement
 * - dynamic allocation
 * - states
 * - pending requests
 */

#define ARP_CACHE_SIZE 16

typedef struct {
    bool valid;
    arp_ipv4_t ip;
    uint8_t mac[6];
} arp_cache_entry_t;


static arp_cache_entry_t
    arp_cache[ARP_CACHE_SIZE];

/*
 * ================================================================
 * BYTE ORDER HELPERS
 * ================================================================
 *
 * Ethernet protocols use network byte order (big endian).
 *
 * The x86 CPU is little endian, so we explicitly encode/decode
 * multi-byte fields.
 */



/*
 * Store a 16-bit value in network byte order.
 */
static void arp_write_u16(
    uint8_t *buffer,
    uint16_t value
)
{
    buffer[0] =
        (uint8_t)((value >> 8) & 0xFF);

    buffer[1] =
        (uint8_t)(value & 0xFF);
}


/*
 * Read a 16-bit network byte order value.
 */
static uint16_t arp_read_u16(
    const uint8_t *buffer
)
{
    return
        ((uint16_t)buffer[0] << 8) |
        ((uint16_t)buffer[1]);
}

static void arp_cache_insert(
    arp_ipv4_t ip,
    const uint8_t *mac
)
{
    /*
     * First check whether this IP is already cached.
     *
     * If it is, update the MAC address.
     */
    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (!arp_cache[i].valid)
            continue;


        bool same_ip = true;

        for (uint32_t j = 0; j < 4; j++)
        {
            if (
                arp_cache[i].ip.bytes[j] !=
                ip.bytes[j]
            )
            {
                same_ip = false;
                break;
            }
        }


        if (same_ip)
        {
            for (uint32_t j = 0; j < 6; j++)
            {
                arp_cache[i].mac[j] =
                    mac[j];
            }

            print_string(
                "ARP: cache entry updated.\n"
            );

            return;
        }
    }


    /*
     * Find an unused entry.
     */
    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (arp_cache[i].valid)
            continue;


        arp_cache[i].valid = true;

        arp_cache[i].ip = ip;


        for (uint32_t j = 0; j < 6; j++)
        {
            arp_cache[i].mac[j] =
                mac[j];
        }


        print_string(
            "ARP: cache entry added.\n"
        );

        return;
    }


    /*
     * Cache is full.
     *
     * For version 1 we simply report it.
     */
    print_string(
        "ARP: WARNING: cache is full.\n"
    );
}

bool arp_lookup(
    arp_ipv4_t ip,
    uint8_t *mac_out
)
{
    if (!arp_initialized)
    {
        return false;
    }
    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (!arp_cache[i].valid)
            continue;

        bool same_ip = true;
        for (uint32_t j = 0; j < 4; j++)
        {
            if (
                arp_cache[i].ip.bytes[j] !=
                ip.bytes[j]
            )
            {
                same_ip = false;
                break;
            }
        }
        if (!same_ip)
            continue;

        /*
         * Copy MAC to caller.
         */
        for (uint32_t j = 0; j < 6; j++)
        {
            mac_out[j] =
                arp_cache[i].mac[j];
        }
        return true;
    }
    return false;
}

/*
 * ================================================================
 * PRINTING
 * ================================================================
 */

static void arp_print_hex8(uint8_t value)
{
    static const char hex[] =
        "0123456789ABCDEF";

    char text[3];

    text[0] =
        hex[(value >> 4) & 0x0F];

    text[1] =
        hex[value & 0x0F];

    text[2] = '\0';

    print_string(text);
}

static void arp_print_decimal8(uint8_t value)
{
    char text[4];

    uint32_t hundreds;
    uint32_t tens;
    uint32_t ones;

    hundreds = value / 100;
    tens = (value / 10) % 10;
    ones = value % 10;


    if (hundreds != 0)
    {
        text[0] = '0' + hundreds;
        text[1] = '0' + tens;
        text[2] = '0' + ones;
        text[3] = '\0';

        print_string(text);

        return;
    }


    if (tens != 0)
    {
        text[0] = '0' + tens;
        text[1] = '0' + ones;
        text[2] = '\0';

        print_string(text);

        return;
    }


    text[0] = '0' + ones;
    text[1] = '\0';

    print_string(text);
}

void arp_print_ip(arp_ipv4_t ip)
{
    arp_print_decimal8(ip.bytes[0]);
    print_string(".");

    arp_print_decimal8(ip.bytes[1]);
    print_string(".");

    arp_print_decimal8(ip.bytes[2]);
    print_string(".");

    arp_print_decimal8(ip.bytes[3]);
}

void arp_print_mac(const uint8_t *mac)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        arp_print_hex8(mac[i]);

        if (i != 5)
            print_string(":");
    }
}

void arp_print_cache(void)
{
    print_string(
        "\nARP: CACHE\n"
    );

    print_string(
        "------------------------------------------------\n"
    );


    bool found = false;


    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (!arp_cache[i].valid)
            continue;


        found = true;


        print_string(
            "  "
        );

        arp_print_ip(
            arp_cache[i].ip
        );

        print_string(
            " -> "
        );

        arp_print_mac(
            arp_cache[i].mac
        );

        print_string(
            "\n"
        );
    }


    if (!found)
    {
        print_string(
            "  <empty>\n"
        );
    }


    print_string(
        "------------------------------------------------\n"
    );
}

/*
 * ================================================================
 * INITIALIZATION
 * ================================================================
 */

void arp_init(arp_ipv4_t local_ip) {
    arp_local_ip = local_ip;

    for (uint32_t i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = false;
        for (uint32_t j = 0; j < 4; j++) {
            arp_cache[i].ip.bytes[j] = 0;
        }
        for (uint32_t j = 0; j < 6; j++) {
            arp_cache[i].mac[j] = 0;
        }
    }
    arp_initialized = true;
    print_string("ARP: initialized.\n");
    print_string("ARP: local IPv4 = ");
    arp_print_ip(arp_local_ip);
    print_string("\n");
}


/*
 * ================================================================
 * SEND ARP REQUEST
 * ================================================================
 */

bool arp_send_request(
    arp_ipv4_t target_ip
)
{
    uint8_t frame[ETHERNET_MIN_FRAME] = { 0 };

    const uint8_t *local_mac;

    /*
     * Make sure ARP was initialized.
     */
    if (!arp_initialized)
    {
        print_string(
            "ARP: ERROR: not initialized.\n"
        );

        return false;
    }


    /*
     * Get our Ethernet MAC address.
     */
    local_mac =
        e1000_get_mac();


    /*
     * ============================================================
     * ETHERNET HEADER
     * ============================================================
     *
     * Destination:
     *
     * FF:FF:FF:FF:FF:FF
     *
     * ARP requests are Ethernet broadcasts.
     */

    for (uint32_t i = 0; i < 6; i++)
    {
        frame[i] = 0xFF;
    }


    /*
     * Source MAC.
     */

    for (uint32_t i = 0; i < 6; i++)
    {
        frame[6 + i] =
            local_mac[i];
    }


    /*
     * EtherType:
     *
     * 0x0806 = ARP
     */

    arp_write_u16(
        &frame[12],
        ARP_ETHERTYPE
    );


    /*
     * ============================================================
     * ARP HEADER
     * ============================================================
     *
     * Ethernet header ends at byte 13.
     *
     * ARP begins at byte 14.
     */

    uint8_t *arp =
        &frame[14];


    /*
     * Hardware type:
     *
     * Ethernet = 1
     */

    arp_write_u16(
        &arp[0],
        ARP_HTYPE_ETHERNET
    );


    /*
     * Protocol type:
     *
     * IPv4 = 0x0800
     */

    arp_write_u16(
        &arp[2],
        ARP_PTYPE_IPV4
    );


    /*
     * Hardware address length:
     *
     * MAC = 6 bytes
     */

    arp[4] =
        ARP_HLEN_ETHERNET;


    /*
     * Protocol address length:
     *
     * IPv4 = 4 bytes
     */

    arp[5] =
        ARP_PLEN_IPV4;


    /*
     * Operation:
     *
     * REQUEST = 1
     */

    arp_write_u16(
        &arp[6],
        ARP_OPCODE_REQUEST
    );


    /*
     * Sender hardware address:
     *
     * Our MAC.
     */

    for (uint32_t i = 0; i < 6; i++)
    {
        arp[8 + i] =
            local_mac[i];
    }


    /*
     * Sender protocol address:
     *
     * Our IPv4 address.
     */

    for (uint32_t i = 0; i < 4; i++)
    {
        arp[14 + i] =
            arp_local_ip.bytes[i];
    }


    /*
     * Target hardware address:
     *
     * Unknown.
     *
     * ARP request uses zeroes here.
     */

    for (uint32_t i = 0; i < 6; i++)
    {
        arp[18 + i] = 0;
    }


    /*
     * Target protocol address:
     *
     * The IPv4 address we're asking about.
     */

    for (uint32_t i = 0; i < 4; i++)
    {
        arp[24 + i] =
            target_ip.bytes[i];
    }


    /*
     * ARP is only 42 bytes:
     *
     * 14 Ethernet
     * 28 ARP
     *
     * Ethernet requires a minimum 60-byte frame
     * before the FCS, so the remaining bytes stay zero.
     */

    print_string(
        "ARP: sending request for "
    );

    arp_print_ip(
        target_ip
    );

    print_string("\n");


    /*
     * Send through the existing E1000 driver.
     */

    if (!e1000_send(
        frame,
        ETHERNET_MIN_FRAME
    ))
    {
        print_string(
            "ARP: ERROR: E1000 TX failed.\n"
        );

        return false;
    }


    print_string(
        "ARP: request submitted.\n"
    );

    return true;
}


/*
 * ================================================================
 * RECEIVE / PROCESS ARP
 * ================================================================
 */

bool arp_poll(void)
{
    uint8_t frame[2048];

    uint16_t length;


    /*
     * Ask E1000 whether a packet has arrived.
     */
    if (!e1000_receive(
        frame,
        sizeof(frame),
        &length
    ))
    {
        return false;
    }


    /*
     * Need enough room for Ethernet + ARP.
     */
    if (length < ARP_FRAME_SIZE)
    {
        return false;
    }


    /*
     * Check Ethernet EtherType.
     */

    uint16_t ether_type =
        arp_read_u16(
            &frame[12]
        );

    if (ether_type != ARP_ETHERTYPE)
    {
        /*
         * It was a packet, but not ARP.
         */
        return false;
    }


    /*
     * ARP begins after the Ethernet header.
     */

    const uint8_t *arp =
        &frame[14];


    /*
     * Verify:
     *
     * Ethernet
     * IPv4
     * MAC length = 6
     * IPv4 length = 4
     */

    if (
        arp_read_u16(&arp[0]) !=
            ARP_HTYPE_ETHERNET ||

        arp_read_u16(&arp[2]) !=
            ARP_PTYPE_IPV4 ||

        arp[4] !=
            ARP_HLEN_ETHERNET ||

        arp[5] !=
            ARP_PLEN_IPV4
    )
    {
        print_string(
            "ARP: unsupported ARP packet.\n"
        );

        return false;
    }


    uint16_t opcode =
        arp_read_u16(
            &arp[6]
        );


    /*
     * Sender MAC.
     */

    const uint8_t *sender_mac =
        &arp[8];


    /*
     * Sender IP.
     */

    arp_ipv4_t sender_ip;

    for (uint32_t i = 0; i < 4; i++)
    {
        sender_ip.bytes[i] =
            arp[14 + i];
    }


    /*
     * Target IP.
     */

    arp_ipv4_t target_ip;

    for (uint32_t i = 0; i < 4; i++)
    {
        target_ip.bytes[i] =
            arp[24 + i];
    }


    /*
     * ============================================================
     * ARP REQUEST
     * ============================================================
     */

    if (opcode == ARP_OPCODE_REQUEST)
    {
        print_string(
            "ARP: REQUEST received from "
        );

        arp_print_ip(
            sender_ip
        );

        print_string(
            " ["
        );

        arp_print_mac(
            sender_mac
        );

        print_string(
            "] asking for "
        );

        arp_print_ip(
            target_ip
        );

        print_string("\n");


        /*
         * For now, we only report the request.
         *
         * ARP replies will be the next step.
         */

        return true;
    }


    if (opcode == ARP_OPCODE_REPLY) {
        print_string(
            "ARP: REPLY received!\n"
        );

        print_string(
            "ARP: IP = "
        );

        arp_print_ip(
            sender_ip
        );

        print_string(
            "\n"
        );

        print_string(
            "ARP: MAC = "
        );

        arp_print_mac(
            sender_mac
        );

        print_string(
            "\n"
        );

        /*
        * Remember this IP -> MAC mapping.
        */
        arp_cache_insert(
            sender_ip,
            sender_mac
        );

        return true;
    }
    print_string("ARP: unknown opcode.\n");
    return false;
}