#include <stdint.h>
#include <stdbool.h>

#include "IPv4.h"
#include "ARP.h"
#include "E1000.h"
#include "ICMP.h"
#include "UDP.h"


extern void print_string(const char *s);

/*
 * ================================================================
 * ETHERNET
 * ================================================================
 */

#define IPV4_ETHERTYPE 0x0800

/*
 * ================================================================
 * IPv4
 * ================================================================
 */

#define IPV4_VERSION 4

#define IPV4_HEADER_LENGTH 20


/*
 * ================================================================
 * PROTOCOL NUMBERS
 * ================================================================
 */

#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_TCP  6
#define IPV4_PROTOCOL_UDP  17

/*
 * ================================================================
 * IPv4 STATE
 * ================================================================
 */

static ipv4_address_t ipv4_local_ip;

ipv4_address_t ipv4_get_local_ip(void) {
    return ipv4_local_ip;
}

static bool ipv4_initialized = false;


/*
 * ================================================================
 * HEX PRINTING
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

    print_string(text);
}


/*
 * ================================================================
 * DECIMAL PRINTING
 * ================================================================
 */

static void ipv4_print_decimal8(
    uint8_t value
)
{
    char text[4];

    uint32_t hundreds;
    uint32_t tens;
    uint32_t ones;


    hundreds =
        value / 100;

    tens =
        (value / 10) % 10;

    ones =
        value % 10;


    /*
     * 100 - 255
     */
    if (hundreds != 0)
    {
        text[0] =
            '0' + hundreds;

        text[1] =
            '0' + tens;

        text[2] =
            '0' + ones;

        text[3] =
            '\0';

        print_string(text);

        return;
    }


    /*
     * 10 - 99
     */
    if (tens != 0)
    {
        text[0] =
            '0' + tens;

        text[1] =
            '0' + ones;

        text[2] =
            '\0';

        print_string(text);

        return;
    }


    /*
     * 0 - 9
     */
    text[0] =
        '0' + ones;

    text[1] =
        '\0';

    print_string(text);
}


/*
 * ================================================================
 * PRINT IPv4 ADDRESS
 * ================================================================
 */

void ipv4_print_address(
    ipv4_address_t address
)
{
    ipv4_print_decimal8(
        address.bytes[0]
    );

    print_string(".");


    ipv4_print_decimal8(
        address.bytes[1]
    );

    print_string(".");


    ipv4_print_decimal8(
        address.bytes[2]
    );

    print_string(".");


    ipv4_print_decimal8(
        address.bytes[3]
    );
}


/*
 * ================================================================
 * IPv4 CHECKSUM
 * ================================================================
 *
 * Internet checksum:
 *
 * 1. Add all 16-bit words.
 * 2. Fold carries back into the lower 16 bits.
 * 3. One's complement the result.
 *
 * IPv4 header checksum is calculated with the
 * checksum field set to zero.
 */

uint16_t ipv4_checksum(
    const uint8_t *data,
    uint16_t length
)
{
    uint32_t sum = 0;

    uint16_t i = 0;


    /*
     * Process complete 16-bit words.
     */
    while (
        i + 1 < length
    )
    {
        uint16_t word;

        word =
            ((uint16_t)data[i] << 8) |
            ((uint16_t)data[i + 1]);


        sum += word;


        /*
         * Fold carry.
         */
        while (
            sum >> 16
        )
        {
            sum =
                (sum & 0xFFFF) +
                (sum >> 16);
        }


        i += 2;
    }


    /*
     * If the length is odd, the final byte
     * occupies the high byte of the final word.
     */
    if (
        i < length
    )
    {
        uint16_t word =
            ((uint16_t)data[i] << 8);


        sum += word;


        while (
            sum >> 16
        )
        {
            sum =
                (sum & 0xFFFF) +
                (sum >> 16);
        }
    }


    return
        (uint16_t)(~sum);
}


/*
 * ================================================================
 * IPv4 INITIALIZATION
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


/*
 * ================================================================
 * IPv4 SEND
 * ================================================================
 */

bool ipv4_send(
    ipv4_address_t destination,
    uint8_t protocol,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    /*
     * Make sure IPv4 was initialized.
     */
    if (
        !ipv4_initialized
    )
    {
        print_string(
            "IPv4: ERROR: not initialized.\n"
        );

        return false;
    }


    /*
     * IPv4 minimum header:
     *
     * 20 bytes.
     */
    uint16_t total_ip_length =
        IPV4_HEADER_LENGTH +
        payload_length;


    /*
     * Ethernet header:
     *
     * 6 destination MAC
     * 6 source MAC
     * 2 EtherType
     *
     * = 14 bytes
     */
    uint16_t frame_length =
        14 +
        total_ip_length;


    /*
     * Ethernet minimum frame size is
     * 60 bytes before the NIC-generated FCS.
     */
    if (
        frame_length < 60
    )
    {
        frame_length = 60;
    }


    /*
     * E1000 TX buffers are 2048 bytes.
     */
    if (
        frame_length >
        2048
    )
    {
        print_string(
            "IPv4: ERROR: packet too large.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * ARP LOOKUP
     * ============================================================
     *
     * We need the Ethernet MAC belonging to
     * the destination IPv4 address.
     */

    uint8_t destination_mac[6];

    arp_ipv4_t arp_destination =
    {
        {
            destination.bytes[0],
            destination.bytes[1],
            destination.bytes[2],
            destination.bytes[3]
        }
    };


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
     * GET OUR MAC
     * ============================================================
     */

    const uint8_t *source_mac =
        e1000_get_mac();


    /*
     * ============================================================
     * FRAME BUFFER
     * ============================================================
     */

    uint8_t frame[2048] = { 0 };


    /*
     * ============================================================
     * ETHERNET HEADER
     * ============================================================
     */


    /*
     * Destination MAC.
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
     * Source MAC.
     */
    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        frame[6 + i] =
            source_mac[i];
    }


    /*
     * EtherType:
     *
     * 0x0800 = IPv4
     *
     * Network byte order:
     *
     * 08 00
     */
    frame[12] =
        0x08;

    frame[13] =
        0x00;


    /*
     * ============================================================
     * IPv4 HEADER
     * ============================================================
     *
     * Ethernet header occupies bytes 0-13.
     *
     * IPv4 header starts at byte 14.
     */

    uint16_t ip =
        14;


    /*
     * ------------------------------------------------------------
     * Byte 0
     * ------------------------------------------------------------
     *
     * Version = 4
     *
     * IHL = 5
     *
     * 4 << 4 | 5
     *
     * = 0x45
     */

    frame[ip + 0] =
        0x45;


    /*
     * ------------------------------------------------------------
     * Byte 1
     * ------------------------------------------------------------
     *
     * DSCP + ECN
     *
     * Currently unused.
     */

    frame[ip + 1] =
        0x00;


    /*
     * ------------------------------------------------------------
     * Bytes 2-3
     * ------------------------------------------------------------
     *
     * Total IPv4 packet length.
     */

    frame[ip + 2] =
        (uint8_t)(
            total_ip_length >> 8
        );

    frame[ip + 3] =
        (uint8_t)(
            total_ip_length & 0xFF
        );


    /*
     * ------------------------------------------------------------
     * Bytes 4-5
     * ------------------------------------------------------------
     *
     * Identification.
     *
     * Fragmentation support will be added later.
     */

    frame[ip + 4] =
        0x00;

    frame[ip + 5] =
        0x00;


    /*
     * ------------------------------------------------------------
     * Bytes 6-7
     * ------------------------------------------------------------
     *
     * Flags + fragment offset.
     *
     * 0x4000 = Don't Fragment.
     */

    frame[ip + 6] =
        0x40;

    frame[ip + 7] =
        0x00;


    /*
     * ------------------------------------------------------------
     * Byte 8
     * ------------------------------------------------------------
     *
     * TTL.
     */

    frame[ip + 8] =
        64;


    /*
     * ------------------------------------------------------------
     * Byte 9
     * ------------------------------------------------------------
     *
     * Protocol.
     *
     * ICMP = 1
     * TCP  = 6
     * UDP  = 17
     */

    frame[ip + 9] =
        protocol;


    /*
     * ------------------------------------------------------------
     * Bytes 10-11
     * ------------------------------------------------------------
     *
     * Header checksum.
     *
     * Must initially be zero.
     */

    frame[ip + 10] =
        0x00;

    frame[ip + 11] =
        0x00;


    /*
     * ------------------------------------------------------------
     * Bytes 12-15
     * ------------------------------------------------------------
     *
     * Source IPv4 address.
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
     * ------------------------------------------------------------
     * Bytes 16-19
     * ------------------------------------------------------------
     *
     * Destination IPv4 address.
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
     * CALCULATE IPv4 HEADER CHECKSUM
     * ============================================================
     */

    uint16_t checksum =
        ipv4_checksum(
            &frame[ip],
            IPV4_HEADER_LENGTH
        );


    frame[ip + 10] =
        (uint8_t)(
            checksum >> 8
        );

    frame[ip + 11] =
        (uint8_t)(
            checksum & 0xFF
        );


    /*
     * ============================================================
     * COPY PAYLOAD
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
            IPV4_HEADER_LENGTH +
            i
        ] =
            payload[i];
    }


    /*
     * ============================================================
     * DEBUG OUTPUT
     * ============================================================
     */

    print_string(
        "IPv4: destination = "
    );

    ipv4_print_address(
        destination
    );

    print_string(
        "\n"
    );


    print_string(
        "IPv4: destination MAC = "
    );


    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        ipv4_print_hex8(
            destination_mac[i]
        );


        if (
            i != 5
        )
        {
            print_string(":");
        }
    }


    print_string(
        "\n"
    );


    print_string(
        "IPv4: protocol = "
    );

    ipv4_print_hex8(
        protocol
    );

    print_string(
        "\n"
    );


    print_string(
        "IPv4: total length = "
    );

    ipv4_print_hex8(
        (uint8_t)(
            total_ip_length >> 8
        )
    );

    ipv4_print_hex8(
        (uint8_t)(
            total_ip_length & 0xFF
        )
    );

    print_string(
        "\n"
    );


    print_string(
        "IPv4: header checksum = 0x"
    );

    ipv4_print_hex8(
        (uint8_t)(
            checksum >> 8
        )
    );

    ipv4_print_hex8(
        (uint8_t)(
            checksum & 0xFF
        )
    );

    print_string(
        "\n"
    );


    /*
     * ============================================================
     * SEND THROUGH E1000
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
 * IPv4 RECEIVE
 * ================================================================
 */

bool ipv4_poll(void)
{
    /*
     * Receive buffer.
     *
     * E1000 currently supports packets up to
     * the size of this buffer.
     */
    uint8_t buffer[2048];

    uint16_t length;


    /*
     * Ask E1000 for a packet.
     */
    if (
        !e1000_receive(
            buffer,
            sizeof(buffer),
            &length
        )
    )
    {
        return false;
    }


    /*
     * Ethernet header:
     *
     * 14 bytes
     *
     * IPv4 minimum header:
     *
     * 20 bytes
     *
     * Total minimum:
     *
     * 34 bytes.
     */
    if (
        length < 34
    )
    {
        return false;
    }


    /*
     * ============================================================
     * ETHERNET TYPE
     * ============================================================
     */

    uint16_t ether_type =
        ((uint16_t)buffer[12] << 8) |
        buffer[13];


    /*
     * Not IPv4.
     */
    if (
        ether_type !=
        IPV4_ETHERTYPE
    )
    {
        return false;
    }


    /*
     * ============================================================
     * IPv4 HEADER
     * ============================================================
     */

    uint16_t ip =
        14;


    /*
     * ============================================================
     * VERSION
     * ============================================================
     */

    uint8_t version =
        buffer[ip] >> 4;


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
     * HEADER LENGTH
     * ============================================================
     */

    uint8_t ihl =
        buffer[ip] & 0x0F;


    uint16_t header_length =
        (uint16_t)ihl * 4;


    if (
        header_length < 20
    )
    {
        print_string(
            "IPv4: ERROR: invalid header length.\n"
        );

        return false;
    }


    if (
        ip + header_length > length
    )
    {
        print_string(
            "IPv4: ERROR: header exceeds packet.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * TOTAL LENGTH
     * ============================================================
     */

    uint16_t total_length =
        ((uint16_t)buffer[ip + 2] << 8) |
        buffer[ip + 3];


    if (
        total_length < header_length
    )
    {
        print_string(
            "IPv4: ERROR: invalid total length.\n"
        );

        return false;
    }


    /*
     * The IPv4 packet must fit inside the
     * received Ethernet frame.
     */
    if (
        total_length >
        (uint16_t)(length - 14)
    )
    {
        print_string(
            "IPv4: ERROR: packet exceeds frame.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * SOURCE IP
     * ============================================================
     */

    ipv4_address_t source_ip;


    source_ip.bytes[0] =
        buffer[ip + 12];

    source_ip.bytes[1] =
        buffer[ip + 13];

    source_ip.bytes[2] =
        buffer[ip + 14];

    source_ip.bytes[3] =
        buffer[ip + 15];


    /*
     * ============================================================
     * DESTINATION IP
     * ============================================================
     */

    ipv4_address_t destination_ip;


    destination_ip.bytes[0] =
        buffer[ip + 16];

    destination_ip.bytes[1] =
        buffer[ip + 17];

    destination_ip.bytes[2] =
        buffer[ip + 18];

    destination_ip.bytes[3] =
        buffer[ip + 19];


    /*
     * ============================================================
     * PROTOCOL
     * ============================================================
     */

    uint8_t protocol =
        buffer[ip + 9];


    /*
     * ============================================================
     * VERIFY CHECKSUM
     * ============================================================
     *
     * When the complete IPv4 header is checksummed,
     * a valid header produces zero.
     */

    uint16_t checksum =
        ipv4_checksum(
            &buffer[ip],
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
    */

    uint16_t ipv4_payload_offset =
        ip + header_length;

    uint16_t ipv4_payload_length =
        total_length - header_length;


    /*
    * UDP
    *
    * Protocol 17 = UDP
    */
    if (protocol == 17)
    {
        print_string(
            "IPv4: dispatching packet to UDP.\n"
        );

        return udp_receive(
            source_ip,
            destination_ip,
            &buffer[ipv4_payload_offset],
            ipv4_payload_length
        );
    }

    print_string(
        "IPv4: total length = 0x"
    );

    ipv4_print_hex8(
        (uint8_t)(
            total_length >> 8
        )
    );

    ipv4_print_hex8(
        (uint8_t)(
            total_length & 0xFF
        )
    );

    print_string(
        "\n"
    );


    /*
    * ============================================================
    * IPv4 PROTOCOL DISPATCH
    * ============================================================
    *
    * Protocol 1 = ICMP.
    */

    if (
        protocol == 1
    )
    {
        const uint8_t *payload =
            &buffer[ip + header_length];

        uint16_t payload_length =
            total_length - header_length;

        return icmp_receive(
            source_ip,
            destination_ip,
            payload,
            payload_length
        );
    }

    const uint8_t *ipv4_payload =
        &buffer[ip + header_length];


    /*
    * ============================================================
    * UDP
    * ============================================================
    */

    if (
        protocol == 17
    )
    {
        return udp_receive(
            source_ip,
            destination_ip,
            ipv4_payload,
            ipv4_payload_length
        );
    }

        /*
     * ============================================================
     * PROTOCOL DISPATCH
     * ============================================================
     */

    if (
        protocol == 17
    )
    {
        const uint8_t *ipv4_payload =
            &buffer[ip + header_length];

        uint16_t ipv4_payload_length =
            total_length - header_length;


        return udp_receive(
            source_ip,
            destination_ip,
            ipv4_payload,
            ipv4_payload_length
        );
    }

    return true;
}