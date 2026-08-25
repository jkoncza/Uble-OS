#include <stdint.h>
#include <stdbool.h>

#include "IPv4.h"
#include "arp.h"
#include "Ethernet.h"
#include "E1000.h"
#include "ICMP.h"
#include "UDP.h"


extern void print_string(
    const char *s
);


/*
 * ================================================================
 * IPv4 STATE
 * ================================================================
 */

static ipv4_address_t ipv4_local_ip;

static bool ipv4_initialized = false;


/*
 * ================================================================
 * BYTE ORDER HELPERS
 * ================================================================
 */

static uint16_t ipv4_read_u16(
    const uint8_t *data
)
{
    return
        ((uint16_t)data[0] << 8) |
        (uint16_t)data[1];
}


static void ipv4_write_u16(
    uint8_t *data,
    uint16_t value
)
{
    data[0] =
        (uint8_t)(value >> 8);

    data[1] =
        (uint8_t)(value & 0xFF);
}


/*
 * ================================================================
 * ADDRESS HELPERS
 * ================================================================
 */

static bool ipv4_address_equal(
    ipv4_address_t a,
    ipv4_address_t b
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


static bool ipv4_address_is_zero(
    ipv4_address_t address
)
{
    return
        address.bytes[0] == 0 &&
        address.bytes[1] == 0 &&
        address.bytes[2] == 0 &&
        address.bytes[3] == 0;
}


static bool ipv4_address_is_limited_broadcast(
    ipv4_address_t address
)
{
    return
        address.bytes[0] == 255 &&
        address.bytes[1] == 255 &&
        address.bytes[2] == 255 &&
        address.bytes[3] == 255;
}


/*
 * ================================================================
 * PRINTING
 * ================================================================
 */

static void ipv4_print_hex8(
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

    print_string(
        text
    );
}


static void ipv4_print_decimal8(
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

        print_string(
            text
        );

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

        print_string(
            text
        );

        return;
    }


    text[0] =
        (char)('0' + ones);

    text[1] =
        '\0';

    print_string(
        text
    );
}


void ipv4_print_address(
    ipv4_address_t address
)
{
    ipv4_print_decimal8(
        address.bytes[0]
    );

    print_string(
        "."
    );

    ipv4_print_decimal8(
        address.bytes[1]
    );

    print_string(
        "."
    );

    ipv4_print_decimal8(
        address.bytes[2]
    );

    print_string(
        "."
    );

    ipv4_print_decimal8(
        address.bytes[3]
    );
}


/*
 * ================================================================
 * CHECKSUM
 * ================================================================
 *
 * RFC 1071-style Internet checksum.
 */

uint16_t ipv4_checksum(
    const uint8_t *data,
    uint16_t length
)
{
    if (
        data == 0
    )
    {
        return 0;
    }

    uint32_t sum = 0;

    uint16_t i = 0;


    while (
        i + 1 < length
    )
    {
        uint16_t word =
            ((uint16_t)data[i] << 8) |
            (uint16_t)data[i + 1];

        sum +=
            word;

        i += 2;
    }


    if (
        i < length
    )
    {
        sum +=
            ((uint16_t)data[i] << 8);
    }


    while (
        sum >> 16
    )
    {
        sum =
            (sum & 0xFFFF) +
            (sum >> 16);
    }


    return
        (uint16_t)(~sum);
}


/*
 * ================================================================
 * INITIALIZATION
 * ================================================================
 */

void ipv4_init(
    ipv4_address_t local_ip
)
{
    ipv4_local_ip =
        local_ip;

    ipv4_initialized =
        true;

    print_string(
        "IPv4: initialized.\n"
    );

    print_string(
        "IPv4: local IP = "
    );

    ipv4_print_address(
        ipv4_local_ip
    );

    print_string(
        "\n"
    );
}


ipv4_address_t ipv4_get_local_ip(
    void
)
{
    return ipv4_local_ip;
}


/*
 * ================================================================
 * IPv4 TRANSMISSION
 * ================================================================
 */

bool ipv4_send(
    ipv4_address_t destination,
    uint8_t protocol,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    if (
        !ipv4_initialized
    )
    {
        print_string(
            "IPv4: ERROR: not initialized.\n"
        );

        return false;
    }


    if (
        payload_length > 0 &&
        payload == 0
    )
    {
        print_string(
            "IPv4: ERROR: NULL payload.\n"
        );

        return false;
    }


    /*
     * No IPv4 fragmentation yet.
     *
     * Maximum IPv4 payload for a normal 1500-byte MTU:
     *
     * 1500 - 20 = 1480.
     */
    if (
        payload_length >
        (IPV4_MTU - IPV4_MIN_HEADER_SIZE)
    )
    {
        print_string(
            "IPv4: ERROR: payload exceeds MTU.\n"
        );

        return false;
    }


    /*
     * We currently don't support unspecified destinations.
     */
    if (
        ipv4_address_is_zero(
            destination
        )
    )
    {
        print_string(
            "IPv4: ERROR: invalid destination.\n"
        );

        return false;
    }


    uint16_t total_ip_length =
        IPV4_MIN_HEADER_SIZE +
        payload_length;


    uint16_t frame_length =
        ETHERNET_HEADER_SIZE +
        total_ip_length;


    /*
     * Ethernet requires at least 60 bytes before FCS.
     *
     * E1000/NIC handles the actual Ethernet FCS.
     */
    if (
        frame_length < 60
    )
    {
        frame_length = 60;
    }


    if (
        frame_length >
        ETHERNET_FRAME_SIZE
    )
    {
        print_string(
            "IPv4: ERROR: frame too large.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * ARP LOOKUP
     * ============================================================
     */

    arp_ipv4_t arp_destination =
    {
        {
            destination.bytes[0],
            destination.bytes[1],
            destination.bytes[2],
            destination.bytes[3]
        }
    };


    uint8_t destination_mac[6];


    if (
        !arp_lookup(
            arp_destination,
            destination_mac
        )
    )
    {
        print_string(
            "IPv4: ERROR: destination MAC not in ARP cache.\n"
        );

        print_string(
            "IPv4: destination = "
        );

        ipv4_print_address(
            destination
        );

        print_string(
            "\n"
        );

        return false;
    }


    /*
     * ============================================================
     * LOCAL MAC
     * ============================================================
     */

    const uint8_t *source_mac =
        e1000_get_mac();


    if (
        source_mac == 0
    )
    {
        print_string(
            "IPv4: ERROR: local MAC unavailable.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * FRAME
     * ============================================================
     */

    uint8_t frame[
        ETHERNET_FRAME_SIZE
    ] =
    {
        0
    };


    /*
     * ============================================================
     * ETHERNET HEADER
     * ============================================================
     */

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        frame[i] =
            destination_mac[i];

        frame[6 + i] =
            source_mac[i];
    }


    frame[12] =
        0x08;

    frame[13] =
        0x00;


    /*
     * ============================================================
     * IPv4 HEADER
     * ============================================================
     */

    const uint16_t ip =
        ETHERNET_HEADER_SIZE;


    /*
     * Version 4
     *
     * IHL = 5
     *
     * 0x45
     */
    frame[ip + 0] =
        0x45;


    /*
     * DSCP / ECN.
     */
    frame[ip + 1] =
        0x00;


    /*
     * Total length.
     */
    ipv4_write_u16(
        &frame[ip + 2],
        total_ip_length
    );


    /*
     * Identification.
     *
     * Fragmentation is not currently implemented.
     */
    frame[ip + 4] =
        0x00;

    frame[ip + 5] =
        0x00;


    /*
     * Flags + fragment offset.
     *
     * DF = 1.
     *
     * We explicitly request no fragmentation because this
     * implementation does not perform reassembly.
     */
    frame[ip + 6] =
        0x40;

    frame[ip + 7] =
        0x00;


    /*
     * TTL.
     */
    frame[ip + 8] =
        64;


    /*
     * Protocol.
     */
    frame[ip + 9] =
        protocol;


    /*
     * Checksum initially zero.
     */
    frame[ip + 10] =
        0;

    frame[ip + 11] =
        0;


    /*
     * Source address.
     */
    frame[ip + 12] =
        ipv4_local_ip.bytes[0];

    frame[ip + 13] =
        ipv4_local_ip.bytes[1];

    frame[ip + 14] =
        ipv4_local_ip.bytes[2];

    frame[ip + 15] =
        ipv4_local_ip.bytes[3];


    /*
     * Destination address.
     */
    frame[ip + 16] =
        destination.bytes[0];

    frame[ip + 17] =
        destination.bytes[1];

    frame[ip + 18] =
        destination.bytes[2];

    frame[ip + 19] =
        destination.bytes[3];


    /*
     * ============================================================
     * CHECKSUM
     * ============================================================
     */

    uint16_t checksum =
        ipv4_checksum(
            &frame[ip],
            IPV4_MIN_HEADER_SIZE
        );


    ipv4_write_u16(
        &frame[ip + 10],
        checksum
    );


    /*
     * ============================================================
     * PAYLOAD
     * ============================================================
     */

    for (
        uint16_t i = 0;
        i < payload_length;
        i++
    )
    {
        frame[
            ip +
            IPV4_MIN_HEADER_SIZE +
            i
        ] =
            payload[i];
    }


    /*
     * ============================================================
     * TRANSMIT
     * ============================================================
     */

    if (
        !e1000_send(
            frame,
            frame_length
        )
    )
    {
        print_string(
            "IPv4: ERROR: E1000 transmission failed.\n"
        );

        return false;
    }


    print_string(
        "IPv4: packet submitted.\n"
    );

    return true;
}


/*
 * ================================================================
 * IPv4 RECEIVE / PROCESS
 * ================================================================
 *
 * IMPORTANT:
 *
 * This function DOES NOT call e1000_receive().
 *
 * Ethernet owns RX.
 *
 * The caller gives us an already received Ethernet frame.
 */

bool ipv4_process_frame(
    const ethernet_frame_t *frame
)
{
    if (
        !ipv4_initialized
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


    /*
     * Ethernet must contain:
     *
     * 14-byte Ethernet header
     * 20-byte minimum IPv4 header
     */
    if (
        frame->length <
        (
            ETHERNET_HEADER_SIZE +
            IPV4_MIN_HEADER_SIZE
        )
    )
    {
        return false;
    }


    /*
     * ============================================================
     * ETHERNET TYPE
     * ============================================================
     */

    if (
        ethernet_get_ethertype(
            frame
        ) !=
        ETHERNET_ETHERTYPE_IPV4
    )
    {
        return false;
    }


    /*
     * Only process Ethernet frames addressed to this host,
     * broadcast, or multicast.
     *
     * Ethernet currently allows multicast through, but this IPv4
     * implementation only accepts IPv4 unicast or limited
     * broadcast destinations below.
     */
    if (
        !ethernet_is_for_us(
            frame
        )
    )
    {
        return false;
    }


    /*
     * IPv4 header begins immediately after Ethernet.
     */
    const uint8_t *ip =
        &frame->data[
            ETHERNET_HEADER_SIZE
        ];


    /*
     * ============================================================
     * VERSION
     * ============================================================
     */

    uint8_t version =
        (uint8_t)(ip[0] >> 4);


    if (
        version != IPV4_VERSION
    )
    {
        print_string(
            "IPv4: ERROR: invalid version.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * IHL
     * ============================================================
     */

    uint8_t ihl =
        (uint8_t)(ip[0] & 0x0F);


    if (
        ihl < 5
    )
    {
        print_string(
            "IPv4: ERROR: invalid IHL.\n"
        );

        return false;
    }


    uint16_t header_length =
        (uint16_t)ihl * 4;


    if (
        header_length >
        IPV4_MAX_HEADER_SIZE
    )
    {
        print_string(
            "IPv4: ERROR: header too large.\n"
        );

        return false;
    }


    if (
        header_length >
        (
            frame->length -
            ETHERNET_HEADER_SIZE
        )
    )
    {
        print_string(
            "IPv4: ERROR: header exceeds frame.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * TOTAL LENGTH
     * ============================================================
     */

    uint16_t total_length =
        ipv4_read_u16(
            &ip[2]
        );


    if (
        total_length <
        header_length
    )
    {
        print_string(
            "IPv4: ERROR: invalid total length.\n"
        );

        return false;
    }


    if (
        total_length <
        IPV4_MIN_HEADER_SIZE
    )
    {
        print_string(
            "IPv4: ERROR: packet smaller than minimum header.\n"
        );

        return false;
    }


    /*
     * The IPv4 packet must fit entirely within the Ethernet frame.
     *
     * Ethernet padding may make frame->length larger than
     * total_length + 14, which is normal.
     */
    uint16_t ethernet_payload_length =
        frame->length -
        ETHERNET_HEADER_SIZE;


    if (
        total_length >
        ethernet_payload_length
    )
    {
        print_string(
            "IPv4: ERROR: packet exceeds Ethernet frame.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * DESTINATION IP
     * ============================================================
     */

    ipv4_address_t destination_ip;

    destination_ip.bytes[0] =
        ip[16];

    destination_ip.bytes[1] =
        ip[17];

    destination_ip.bytes[2] =
        ip[18];

    destination_ip.bytes[3] =
        ip[19];


    /*
     * Only accept packets actually addressed to us or the
     * limited IPv4 broadcast address.
     *
     * We do not yet implement subnet configuration/multicast
     * routing, so don't pretend we do.
     */
    if (
        !ipv4_address_equal(
            destination_ip,
            ipv4_local_ip
        ) &&
        !ipv4_address_is_limited_broadcast(
            destination_ip
        )
    )
    {
        return false;
    }


    /*
     * ============================================================
     * SOURCE IP
     * ============================================================
     */

    ipv4_address_t source_ip;

    source_ip.bytes[0] =
        ip[12];

    source_ip.bytes[1] =
        ip[13];

    source_ip.bytes[2] =
        ip[14];

    source_ip.bytes[3] =
        ip[15];


    /*
     * A normal received IPv4 packet should not have an unspecified
     * source address. DHCP/bootstrapping can eventually add an
     * exception if the kernel needs it.
     */
    if (
        ipv4_address_is_zero(
            source_ip
        )
    )
    {
        return false;
    }


    /*
     * ============================================================
     * FLAGS / FRAGMENT OFFSET
     * ============================================================
     *
     * IPv4 fragmentation/reassembly isn't implemented yet.
     *
     * Therefore:
     *
     *   MF = 0
     *   fragment offset = 0
     *
     * are required.
     */

    uint16_t flags_fragment =
        ipv4_read_u16(
            &ip[6]
        );


    uint16_t fragment_offset =
        (uint16_t)(
            flags_fragment &
            0x1FFF
        );


    bool more_fragments =
        (
            (flags_fragment & 0x2000) != 0
        );


    if (
        fragment_offset != 0 ||
        more_fragments
    )
    {
        print_string(
            "IPv4: fragmented packet dropped; reassembly unavailable.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * HEADER CHECKSUM
     * ============================================================
     *
     * A correctly received IPv4 header produces zero when the
     * complete header, including its checksum field, is checked.
     */

    uint16_t checksum =
        ipv4_checksum(
            ip,
            header_length
        );


    if (
        checksum != 0
    )
    {
        print_string(
            "IPv4: ERROR: checksum invalid.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * PROTOCOL
     * ============================================================
     */

    uint8_t protocol =
        ip[9];


    /*
     * ============================================================
     * PAYLOAD
     * ============================================================
     */

    uint16_t payload_length =
        total_length -
        header_length;


    const uint8_t *payload =
        &ip[
            header_length
        ];


    /*
     * ============================================================
     * DEBUG
     * ============================================================
     */

    print_string(
        "IPv4: packet received.\n"
    );

    print_string(
        "IPv4: source = "
    );

    ipv4_print_address(
        source_ip
    );

    print_string(
        "\n"
    );

    print_string(
        "IPv4: destination = "
    );

    ipv4_print_address(
        destination_ip
    );

    print_string(
        "\n"
    );

    print_string(
        "IPv4: protocol = 0x"
    );

    ipv4_print_hex8(
        protocol
    );

    print_string(
        "\n"
    );


    /*
     * ============================================================
     * PROTOCOL DISPATCH
     * ============================================================
     *
     * Each protocol gets the exact IPv4 payload and length.
     *
     * There is deliberately only ONE UDP dispatch and ONE ICMP
     * dispatch.
     */


    /*
     * ------------------------------------------------------------
     * ICMP
     * ------------------------------------------------------------
     */

    if (
        protocol ==
        IPV4_PROTOCOL_ICMP
    )
    {
        return icmp_receive(
            source_ip,
            destination_ip,
            payload,
            payload_length
        );
    }


    /*
     * ------------------------------------------------------------
     * UDP
     * ------------------------------------------------------------
     */

    if (
        protocol ==
        IPV4_PROTOCOL_UDP
    )
    {
        return udp_receive(
            source_ip,
            destination_ip,
            payload,
            payload_length
        );
    }


    /*
     * ------------------------------------------------------------
     * TCP
     * ------------------------------------------------------------
     *
     * TCP isn't implemented yet.
     *
     * The packet itself was valid IPv4, so don't report the IPv4
     * layer as corrupt merely because the protocol isn't supported.
     */

    if (
        protocol ==
        IPV4_PROTOCOL_TCP
    )
    {
        print_string(
            "IPv4: TCP packet received; TCP not implemented.\n"
        );

        return true;
    }


    /*
     * ------------------------------------------------------------
     * UNKNOWN PROTOCOL
     * ------------------------------------------------------------
     */

    print_string(
        "IPv4: unsupported protocol.\n"
    );

    return true;
}


/*
 * ================================================================
 * COMPATIBILITY POLL
 * ================================================================
 *
 * This exists because current main.c still calls ipv4_poll().
 *
 * IMPORTANT:
 *
 * It now receives through Ethernet rather than E1000 directly.
 *
 * Eventually main.c will call ethernet_receive() once and dispatch
 * the resulting frame itself.
 */

bool ipv4_poll(
    void
)
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
        ) !=
        ETHERNET_ETHERTYPE_IPV4
    )
    {
        return false;
    }


    return ipv4_process_frame(
        &frame
    );
}
