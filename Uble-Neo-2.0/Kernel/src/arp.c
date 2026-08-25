#include <stdint.h>
#include <stdbool.h>

#include "arp.h"
#include "Ethernet.h"
#include "E1000.h"

extern void print_string(const char *s);


/*
 * ================================================================
 * ARP CONSTANTS
 * ================================================================
 */

#define ARP_ETHERTYPE          0x0806

#define ARP_HTYPE_ETHERNET    0x0001
#define ARP_PTYPE_IPV4        0x0800

#define ARP_HLEN_ETHERNET     6
#define ARP_PLEN_IPV4         4

#define ARP_OPCODE_REQUEST    0x0001
#define ARP_OPCODE_REPLY      0x0002

#define ARP_PACKET_SIZE       28

#define ARP_CACHE_SIZE        16

#define ARP_ENTRY_MAX_AGE     300


/*
 * ================================================================
 * ARP STATE
 * ================================================================
 */

static arp_ipv4_t arp_local_ip;

static bool arp_initialized = false;

static uint32_t arp_age_counter = 1;


/*
 * ================================================================
 * ARP CACHE
 * ================================================================
 */

typedef struct
{
    bool valid;

    arp_ipv4_t ip;

    uint8_t mac[6];

    uint32_t age;

} arp_cache_entry_t;


static arp_cache_entry_t
    arp_cache[ARP_CACHE_SIZE];


/*
 * ================================================================
 * BYTE ORDER
 * ================================================================
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


static uint16_t arp_read_u16(
    const uint8_t *buffer
)
{
    return
        ((uint16_t)buffer[0] << 8) |
        (uint16_t)buffer[1];
}


/*
 * ================================================================
 * IP HELPERS
 * ================================================================
 */

static bool arp_ip_equal(
    arp_ipv4_t a,
    arp_ipv4_t b
)
{
    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        if (
            a.bytes[i] !=
            b.bytes[i]
        )
        {
            return false;
        }
    }

    return true;
}


static bool arp_ip_is_zero(
    arp_ipv4_t ip
)
{
    return
        ip.bytes[0] == 0 &&
        ip.bytes[1] == 0 &&
        ip.bytes[2] == 0 &&
        ip.bytes[3] == 0;
}


/*
 * ================================================================
 * MAC VALIDATION
 * ================================================================
 */

static bool arp_mac_is_zero(
    const uint8_t *mac
)
{
    if (
        mac == 0
    )
    {
        return true;
    }

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        if (
            mac[i] != 0
        )
        {
            return false;
        }
    }

    return true;
}


static bool arp_mac_is_broadcast(
    const uint8_t *mac
)
{
    if (
        mac == 0
    )
    {
        return false;
    }

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        if (
            mac[i] != 0xFF
        )
        {
            return false;
        }
    }

    return true;
}


static bool arp_mac_is_valid(
    const uint8_t *mac
)
{
    if (
        mac == 0
    )
    {
        return false;
    }

    if (
        arp_mac_is_zero(mac)
    )
    {
        return false;
    }

    if (
        arp_mac_is_broadcast(mac)
    )
    {
        return false;
    }

    return true;
}


/*
 * ================================================================
 * CACHE HELPERS
 * ================================================================
 */

static void arp_cache_clear(
    uint32_t index
)
{
    if (
        index >= ARP_CACHE_SIZE
    )
    {
        return;
    }

    arp_cache[index].valid =
        false;

    arp_cache[index].age =
        0;

    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        arp_cache[index].ip.bytes[i] =
            0;
    }

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        arp_cache[index].mac[i] =
            0;
    }
}


static int32_t arp_cache_find(
    arp_ipv4_t ip
)
{
    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (
            !arp_cache[i].valid
        )
        {
            continue;
        }

        if (
            arp_ip_equal(
                arp_cache[i].ip,
                ip
            )
        )
        {
            return (int32_t)i;
        }
    }

    return -1;
}


static int32_t arp_cache_find_free(void)
{
    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (
            !arp_cache[i].valid
        )
        {
            return (int32_t)i;
        }
    }

    return -1;
}


static int32_t arp_cache_find_oldest(void)
{
    int32_t oldest_index = -1;

    uint32_t oldest_age = 0;

    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (
            !arp_cache[i].valid
        )
        {
            continue;
        }

        if (
            oldest_index < 0 ||
            arp_cache[i].age <
            oldest_age
        )
        {
            oldest_index =
                (int32_t)i;

            oldest_age =
                arp_cache[i].age;
        }
    }

    return oldest_index;
}


/*
 * ================================================================
 * CACHE AGING
 * ================================================================
 */

static void arp_cache_age(void)
{
    arp_age_counter++;

    if (
        arp_age_counter == 0
    )
    {
        arp_age_counter = 1;

        for (
            uint32_t i = 0;
            i < ARP_CACHE_SIZE;
            i++
        )
        {
            if (
                arp_cache[i].valid
            )
            {
                arp_cache[i].age = 1;
            }
        }
    }

    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        if (
            !arp_cache[i].valid
        )
        {
            continue;
        }

        uint32_t age =
            arp_age_counter -
            arp_cache[i].age;

        if (
            age > ARP_ENTRY_MAX_AGE
        )
        {
            arp_cache_clear(i);
        }
    }
}


/*
 * ================================================================
 * CACHE INSERT / UPDATE
 * ================================================================
 */

static bool arp_cache_insert(
    arp_ipv4_t ip,
    const uint8_t *mac
)
{
    if (
        !arp_initialized
    )
    {
        return false;
    }

    if (
        arp_ip_is_zero(ip)
    )
    {
        return false;
    }

    if (
        !arp_mac_is_valid(mac)
    )
    {
        return false;
    }

    int32_t index =
        arp_cache_find(ip);

    /*
     * Update an existing mapping.
     */
    if (
        index >= 0
    )
    {
        for (
            uint32_t i = 0;
            i < 6;
            i++
        )
        {
            arp_cache[index].mac[i] =
                mac[i];
        }

        arp_cache[index].age =
            arp_age_counter;

        return true;
    }

    /*
     * Find an empty slot.
     */
    index =
        arp_cache_find_free();

    /*
     * If full, replace the oldest entry.
     */
    if (
        index < 0
    )
    {
        index =
            arp_cache_find_oldest();
    }

    if (
        index < 0
    )
    {
        return false;
    }

    arp_cache[index].valid =
        true;

    arp_cache[index].ip =
        ip;

    arp_cache[index].age =
        arp_age_counter;

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        arp_cache[index].mac[i] =
            mac[i];
    }

    return true;
}


/*
 * ================================================================
 * PUBLIC ARP LOOKUP
 * ================================================================
 */

bool arp_lookup(
    arp_ipv4_t ip,
    uint8_t *mac_out
)
{
    if (
        !arp_initialized
    )
    {
        return false;
    }

    if (
        mac_out == 0
    )
    {
        return false;
    }

    arp_cache_age();

    int32_t index =
        arp_cache_find(ip);

    if (
        index < 0
    )
    {
        return false;
    }

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        mac_out[i] =
            arp_cache[index].mac[i];
    }

    /*
     * Refresh entries that are actually used.
     */
    arp_cache[index].age =
        arp_age_counter;

    return true;
}


/*
 * ================================================================
 * PRINTING
 * ================================================================
 */

static void arp_print_hex8(
    uint8_t value
)
{
    static const char hex[] =
        "0123456789ABCDEF";

    char text[3];

    text[0] =
        hex[(value >> 4) & 0x0F];

    text[1] =
        hex[value & 0x0F];

    text[2] =
        '\0';

    print_string(text);
}


static void arp_print_decimal8(
    uint8_t value
)
{
    char text[4];

    uint32_t hundreds =
        value / 100;

    uint32_t tens =
        (value / 10) % 10;

    uint32_t ones =
        value % 10;

    if (
        hundreds != 0
    )
    {
        text[0] =
            (char)('0' + hundreds);

        text[1] =
            (char)('0' + tens);

        text[2] =
            (char)('0' + ones);

        text[3] =
            '\0';

        print_string(text);

        return;
    }

    if (
        tens != 0
    )
    {
        text[0] =
            (char)('0' + tens);

        text[1] =
            (char)('0' + ones);

        text[2] =
            '\0';

        print_string(text);

        return;
    }

    text[0] =
        (char)('0' + ones);

    text[1] =
        '\0';

    print_string(text);
}


void arp_print_ip(
    arp_ipv4_t ip
)
{
    arp_print_decimal8(
        ip.bytes[0]
    );

    print_string(".");

    arp_print_decimal8(
        ip.bytes[1]
    );

    print_string(".");

    arp_print_decimal8(
        ip.bytes[2]
    );

    print_string(".");

    arp_print_decimal8(
        ip.bytes[3]
    );
}


void arp_print_mac(
    const uint8_t *mac
)
{
    if (
        mac == 0
    )
    {
        print_string("<null>");

        return;
    }

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        arp_print_hex8(
            mac[i]
        );

        if (
            i != 5
        )
        {
            print_string(":");
        }
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
        if (
            !arp_cache[i].valid
        )
        {
            continue;
        }

        found = true;

        print_string("  ");

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

    if (
        !found
    )
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

void arp_init(
    arp_ipv4_t local_ip
)
{
    arp_local_ip =
        local_ip;

    arp_age_counter =
        1;

    for (
        uint32_t i = 0;
        i < ARP_CACHE_SIZE;
        i++
    )
    {
        arp_cache_clear(i);
    }

    arp_initialized =
        true;

    print_string(
        "ARP: initialized.\n"
    );

    print_string(
        "ARP: local IPv4 = "
    );

    arp_print_ip(
        arp_local_ip
    );

    print_string(
        "\n"
    );
}


/*
 * ================================================================
 * SEND ARP FRAME
 * ================================================================
 */

static bool arp_send_frame(
    uint16_t opcode,
    const uint8_t *destination_mac,
    const uint8_t *sender_mac,
    arp_ipv4_t sender_ip,
    const uint8_t *target_mac,
    arp_ipv4_t target_ip
)
{
    uint8_t frame[60];

    for (
        uint32_t i = 0;
        i < sizeof(frame);
        i++
    )
    {
        frame[i] = 0;
    }

    if (
        destination_mac == 0 ||
        sender_mac == 0 ||
        target_mac == 0
    )
    {
        return false;
    }

    /*
     * Ethernet destination.
     */
    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        frame[i] =
            destination_mac[i];
    }

    /*
     * Ethernet source.
     */
    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        frame[6 + i] =
            sender_mac[i];
    }

    /*
     * EtherType = ARP.
     */
    arp_write_u16(
        &frame[12],
        ARP_ETHERTYPE
    );

    /*
     * ARP header.
     */
    uint8_t *arp =
        &frame[14];

    arp_write_u16(
        &arp[0],
        ARP_HTYPE_ETHERNET
    );

    arp_write_u16(
        &arp[2],
        ARP_PTYPE_IPV4
    );

    arp[4] =
        ARP_HLEN_ETHERNET;

    arp[5] =
        ARP_PLEN_IPV4;

    arp_write_u16(
        &arp[6],
        opcode
    );

    /*
     * Sender MAC.
     */
    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        arp[8 + i] =
            sender_mac[i];
    }

    /*
     * Sender IP.
     */
    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        arp[14 + i] =
            sender_ip.bytes[i];
    }

    /*
     * Target MAC.
     */
    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        arp[18 + i] =
            target_mac[i];
    }

    /*
     * Target IP.
     */
    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        arp[24 + i] =
            target_ip.bytes[i];
    }

    return e1000_send(
        frame,
        sizeof(frame)
    );
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
    if (
        !arp_initialized
    )
    {
        print_string(
            "ARP: ERROR: not initialized.\n"
        );

        return false;
    }

    if (
        arp_ip_is_zero(target_ip)
    )
    {
        print_string(
            "ARP: ERROR: invalid target IP.\n"
        );

        return false;
    }

    const uint8_t *local_mac =
        e1000_get_mac();

    if (
        local_mac == 0
    )
    {
        print_string(
            "ARP: ERROR: no local MAC.\n"
        );

        return false;
    }

    uint8_t broadcast_mac[6] =
    {
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF,
        0xFF
    };

    uint8_t unknown_mac[6] =
    {
        0,
        0,
        0,
        0,
        0,
        0
    };

    print_string(
        "ARP: sending request for "
    );

    arp_print_ip(
        target_ip
    );

    print_string(
        "\n"
    );

    return arp_send_frame(
        ARP_OPCODE_REQUEST,
        broadcast_mac,
        local_mac,
        arp_local_ip,
        unknown_mac,
        target_ip
    );
}


/*
 * ================================================================
 * SEND ARP REPLY
 * ================================================================
 */

static bool arp_send_reply(
    const uint8_t *destination_mac,
    arp_ipv4_t destination_ip
)
{
    const uint8_t *local_mac =
        e1000_get_mac();

    if (
        local_mac == 0
    )
    {
        return false;
    }

    return arp_send_frame(
        ARP_OPCODE_REPLY,
        destination_mac,
        local_mac,
        arp_local_ip,
        destination_mac,
        destination_ip
    );
}


/*
 * ================================================================
 * PROCESS ARP REQUEST
 * ================================================================
 */

static bool arp_process_request(
    const uint8_t *sender_mac,
    arp_ipv4_t sender_ip,
    arp_ipv4_t target_ip
)
{
    /*
     * Learn the sender.
     */
    arp_cache_insert(
        sender_ip,
        sender_mac
    );

    /*
     * Only answer requests for our IP.
     */
    if (
        !arp_ip_equal(
            target_ip,
            arp_local_ip
        )
    )
    {
        return true;
    }

    print_string(
        "ARP: request is for us.\n"
    );

    if (
        !arp_send_reply(
            sender_mac,
            sender_ip
        )
    )
    {
        print_string(
            "ARP: ERROR: reply TX failed.\n"
        );

        return false;
    }

    return true;
}


/*
 * ================================================================
 * PROCESS ARP REPLY
 * ================================================================
 */

static bool arp_process_reply(
    const uint8_t *sender_mac,
    arp_ipv4_t sender_ip,
    arp_ipv4_t target_ip
)
{
    /*
     * Only accept replies actually directed at our IP.
     */
    if (
        !arp_ip_equal(
            target_ip,
            arp_local_ip
        )
    )
    {
        return false;
    }

    if (
        !arp_mac_is_valid(
            sender_mac
        )
    )
    {
        return false;
    }

    if (
        !arp_cache_insert(
            sender_ip,
            sender_mac
        )
    )
    {
        return false;
    }

    print_string(
        "ARP: learned "
    );

    arp_print_ip(
        sender_ip
    );

    print_string(
        " -> "
    );

    arp_print_mac(
        sender_mac
    );

    print_string(
        "\n"
    );

    return true;
}


/*
 * ================================================================
 * PROCESS AN ETHERNET ARP FRAME
 * ================================================================
 *
 * THIS IS THE IMPORTANT NEW API.
 *
 * Ethernet.c receives the frame from E1000.
 *
 * ARP only parses/processes that already-received frame.
 *
 * ARP no longer owns the E1000 RX queue.
 */

bool arp_process_frame(
    const ethernet_frame_t *frame
)
{
    if (
        !arp_initialized
    )
    {
        return false;
    }

    if (
        frame == 0
    )
    {
        return false;
    }

    if (
        frame->length <
        ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE
    )
    {
        return false;
    }

    /*
     * Ethernet must identify this as ARP.
     */
    if (
        ethernet_get_ethertype(frame) !=
        ARP_ETHERTYPE
    )
    {
        return false;
    }

    /*
     * Make sure the frame is actually for us,
     * broadcast, or multicast.
     */
    if (
        !ethernet_is_for_us(frame)
    )
    {
        return false;
    }

    const uint8_t *arp =
        &frame->data[
            ETHERNET_HEADER_SIZE
        ];

    /*
     * Hardware type.
     */
    if (
        arp_read_u16(&arp[0]) !=
        ARP_HTYPE_ETHERNET
    )
    {
        return false;
    }

    /*
     * Protocol type.
     */
    if (
        arp_read_u16(&arp[2]) !=
        ARP_PTYPE_IPV4
    )
    {
        return false;
    }

    /*
     * Hardware address length.
     */
    if (
        arp[4] !=
        ARP_HLEN_ETHERNET
    )
    {
        return false;
    }

    /*
     * Protocol address length.
     */
    if (
        arp[5] !=
        ARP_PLEN_IPV4
    )
    {
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

    if (
        !arp_mac_is_valid(
            sender_mac
        )
    )
    {
        return false;
    }

    /*
     * Sender IP.
     */
    arp_ipv4_t sender_ip;

    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        sender_ip.bytes[i] =
            arp[14 + i];
    }

    /*
     * Target IP.
     */
    arp_ipv4_t target_ip;

    for (
        uint32_t i = 0;
        i < 4;
        i++
    )
    {
        target_ip.bytes[i] =
            arp[24 + i];
    }

    /*
     * ============================================================
     * REQUEST
     * ============================================================
     */

    if (
        opcode ==
        ARP_OPCODE_REQUEST
    )
    {
        return arp_process_request(
            sender_mac,
            sender_ip,
            target_ip
        );
    }

    /*
     * ============================================================
     * REPLY
     * ============================================================
     */

    if (
        opcode ==
        ARP_OPCODE_REPLY
    )
    {
        return arp_process_reply(
            sender_mac,
            sender_ip,
            target_ip
        );
    }

    /*
     * Unknown ARP operation.
     */
    return false;
}


/*
 * ================================================================
 * LEGACY ARP POLL
 * ================================================================
 *
 * This is retained temporarily because the current main.c still
 * calls arp_poll().
 *
 * IMPORTANT:
 *
 * The final networking architecture should NOT use this function.
 * main.c will eventually receive one Ethernet frame and dispatch
 * it to arp_process_frame() or ipv4_process_frame().
 */

bool arp_poll(void)
{
    ethernet_frame_t frame;

    if (
        !ethernet_receive(
            &frame
        )
    )
    {
        return false;
    }

    if (
        ethernet_get_ethertype(
            &frame
        ) != ARP_ETHERTYPE
    )
    {
        /*
         * A non-ARP packet was consumed.
         *
         * This compatibility function will disappear when main.c
         * is converted to the central Ethernet dispatcher.
         */
        return false;
    }

    return arp_process_frame(
        &frame
    );
}
