#include "UDP.h"
#include "IPv4.h"

#include <stdint.h>
#include <stdbool.h>


extern void print_string(const char *s);


/*
 * ============================================================
 * UDP CONSTANTS
 * ============================================================
 */

#define UDP_HEADER_SIZE 8

#define IPV4_PROTOCOL_UDP 17

/*
 * ============================================================
 * UDP STATE
 * ============================================================
 */

static bool udp_initialized = false;

static udp_listener_t udp_listeners[UDP_MAX_LISTENERS];

/*
 * ============================================================
 * SMALL DECIMAL PRINT HELPER
 * ============================================================
 *
 * We keep this inside UDP so UDP does not depend on ICMP's
 * printing functions.
 */

static void udp_print_decimal16(
    uint16_t value
)
{
    char buffer[6];

    uint16_t i = 0;


    if (value == 0)
    {
        print_string("0");
        return;
    }


    while (
        value > 0 &&
        i < sizeof(buffer) - 1
    )
    {
        buffer[i++] =
            (char)('0' + (value % 10));

        value /= 10;
    }


    while (i > 0)
    {
        char output[2];

        output[0] =
            buffer[--i];

        output[1] =
            '\0';

        print_string(
            output
        );
    }
}


/*
 * ============================================================
 * UDP INITIALIZATION
 * ============================================================
 */

void udp_init(void) {
    udp_initialized = true;
    for (uint16_t i = 0; i < UDP_MAX_LISTENERS; i++) {
        udp_listeners[i].active = false;
        udp_listeners[i].port = 0;
    }
    print_string("UDP: initialized.\n");
}


/*
 * ============================================================
 * UDP BIND
 * ============================================================
 */

/*
 * ============================================================
 * UDP BIND
 * ============================================================
 */

/*
 * ============================================================
 * UDP BIND
 * ============================================================
 */

bool udp_bind(uint16_t port)
{
    if (port == 0)
    {
        print_string(
            "UDP: cannot bind port 0.\n"
        );

        return false;
    }

    /*
     * Already bound?
     */

    if (udp_is_bound(port))
    {
        print_string(
            "UDP: port already bound.\n"
        );

        return false;
    }

    /*
     * Find an empty listener slot.
     */

    for (
        uint16_t i = 0;
        i < UDP_MAX_LISTENERS;
        i++
    )
    {
        if (!udp_listeners[i].active)
        {
            udp_listeners[i].active = true;
            udp_listeners[i].port = port;

            print_string(
                "UDP: port bound successfully: "
            );

            udp_print_decimal16(port);

            print_string(
                ".\n"
            );

            return true;
        }
    }

    print_string(
        "UDP: listener table full.\n"
    );

    return false;
}


/*
 * ============================================================
 * UDP UNBIND
 * ============================================================
 */

bool udp_unbind(uint16_t port)
{
    for (
        uint16_t i = 0;
        i < UDP_MAX_LISTENERS;
        i++
    )
    {
        if (
            udp_listeners[i].active &&
            udp_listeners[i].port == port
        )
        {
            udp_listeners[i].active = false;
            udp_listeners[i].port = 0;

            print_string(
                "UDP: port unbound: "
            );

            udp_print_decimal16(port);

            print_string(
                ".\n"
            );

            return true;
        }
    }

    return false;
}


/*
 * ============================================================
 * UDP IS BOUND
 * ============================================================
 */

bool udp_is_bound(uint16_t port)
{
    for (
        uint16_t i = 0;
        i < UDP_MAX_LISTENERS;
        i++
    )
    {
        if (
            udp_listeners[i].active &&
            udp_listeners[i].port == port
        )
        {
            return true;
        }
    }

    return false;
}


/*
 * ============================================================
 * UDP CHECKSUM
 * ============================================================
 *
 * UDP checksum uses the IPv4 pseudo-header:
 *
 *   source IP
 *   destination IP
 *   zero
 *   protocol
 *   UDP length
 *
 * followed by:
 *
 *   UDP header
 *   UDP payload
 */

uint16_t udp_checksum(
    ipv4_address_t source,
    ipv4_address_t destination,
    const uint8_t *data,
    uint16_t length
)
{
    uint32_t sum = 0;


    /*
     * ========================================================
     * SOURCE IP
     * ========================================================
     */

    sum +=
        ((uint16_t)source.bytes[0] << 8) |
        source.bytes[1];

    sum +=
        ((uint16_t)source.bytes[2] << 8) |
        source.bytes[3];


    /*
     * ========================================================
     * DESTINATION IP
     * ========================================================
     */

    sum +=
        ((uint16_t)destination.bytes[0] << 8) |
        destination.bytes[1];

    sum +=
        ((uint16_t)destination.bytes[2] << 8) |
        destination.bytes[3];


    /*
     * Zero byte + protocol.
     */

    sum +=
        IPV4_PROTOCOL_UDP;


    /*
     * UDP length.
     */

    sum +=
        length;


    /*
     * ========================================================
     * UDP DATA
     * ========================================================
     */

    for (
        uint16_t i = 0;
        i + 1 < length;
        i += 2
    )
    {
        sum +=
            ((uint16_t)data[i] << 8) |
            data[i + 1];
    }


    /*
     * Odd-length data.
     */

    if (
        length & 1
    )
    {
        sum +=
            ((uint16_t)data[length - 1] << 8);
    }


    /*
     * Fold carries.
     */

    while (
        sum >> 16
    )
    {
        sum =
            (sum & 0xFFFF) +
            (sum >> 16);
    }


    return (uint16_t)~sum;
}


/*
 * ============================================================
 * UDP SEND
 * ============================================================
 */

bool udp_send(
    uint16_t source_port,
    ipv4_address_t destination,
    uint16_t destination_port,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    /*
     * UDP length:
     *
     *     header + payload
     */

    uint16_t udp_length =
        UDP_HEADER_SIZE +
        payload_length;


    /*
     * Detect 16-bit overflow.
     */

    if (
        udp_length < UDP_HEADER_SIZE
    )
    {
        print_string(
            "UDP: packet length overflow.\n"
        );

        return false;
    }


    /*
     * ========================================================
     * PACKET BUFFER
     * ========================================================
     */

    uint8_t packet[2048];


    if (
        udp_length > sizeof(packet)
    )
    {
        print_string(
            "UDP: packet too large.\n"
        );

        return false;
    }


    /*
     * ========================================================
     * SOURCE PORT
     * ========================================================
     */

    packet[0] =
        (uint8_t)(source_port >> 8);

    packet[1] =
        (uint8_t)(source_port & 0xFF);


    /*
     * ========================================================
     * DESTINATION PORT
     * ========================================================
     */

    packet[2] =
        (uint8_t)(destination_port >> 8);

    packet[3] =
        (uint8_t)(destination_port & 0xFF);


    /*
     * ========================================================
     * UDP LENGTH
     * ========================================================
     */

    packet[4] =
        (uint8_t)(udp_length >> 8);

    packet[5] =
        (uint8_t)(udp_length & 0xFF);

    packet[6] = 0;
    packet[7] = 0;


    /*
     * ========================================================
     * PAYLOAD
     * ========================================================
     */

    for (
        uint16_t i = 0;
        i < payload_length;
        i++
    )
    {
        packet[
            UDP_HEADER_SIZE + i
        ] =
            payload[i];
    }

    uint16_t checksum =
        udp_checksum(
            ipv4_get_local_ip(),
            destination,
            packet,
            udp_length
        );


    packet[6] =
        (uint8_t)(checksum >> 8);

    packet[7] =
        (uint8_t)(checksum & 0xFF);

    /*
    * ========================================================
    * UDP LOOPBACK
    * ========================================================
    *
    * If the destination is our own IPv4 address,
    * deliver the UDP packet directly to the UDP
    * receive layer.
    */

    ipv4_address_t local_ip =
        ipv4_get_local_ip();


    bool is_loopback =
        destination.bytes[0] == local_ip.bytes[0] &&
        destination.bytes[1] == local_ip.bytes[1] &&
        destination.bytes[2] == local_ip.bytes[2] &&
        destination.bytes[3] == local_ip.bytes[3];


    if (is_loopback)
    {
        print_string(
            "UDP: loopback destination detected.\n"
        );

        if (
            !udp_receive(
                local_ip,
                destination,
                packet,
                udp_length
            )
        )
        {
            print_string(
                "UDP: loopback receive failed.\n"
            );

            return false;
        }

        print_string(
            "UDP: loopback delivery successful.\n"
        );

        return true;
    }


    /*
    * ========================================================
    * NORMAL IPv4 TRANSMISSION
    * ========================================================
    */

    if (
        !ipv4_send(
            destination,
            IPV4_PROTOCOL_UDP,
            packet,
            udp_length
        )
    )
    {
        print_string(
            "UDP: IPv4 transmission failed.\n"
        );

        return false;
    }


    print_string(
        "UDP: datagram transmitted.\n"
    );


    return true;
}


/*
 * ============================================================
 * UDP RECEIVE
 * ============================================================
 */

bool udp_receive(
    ipv4_address_t source,
    ipv4_address_t destination,
    const uint8_t *payload,
    uint16_t length
)
{
    /*
     * ========================================================
     * MINIMUM UDP PACKET
     * ========================================================
     */

    if (
        length < UDP_HEADER_SIZE
    )
    {
        print_string(
            "UDP: packet too short.\n"
        );

        return false;
    }


    /*
     * ========================================================
     * UDP HEADER
     * ========================================================
     */

    uint16_t source_port =
        ((uint16_t)payload[0] << 8) |
        payload[1];


    uint16_t destination_port =
        ((uint16_t)payload[2] << 8) |
        payload[3];

    /*
    * ============================================================
    * PORT LISTENER CHECK
    * ============================================================
    */

    if (!udp_is_bound(destination_port))
    {
        print_string(
            "UDP: destination port not bound.\n"
        );

        return false;
    }

    print_string(
        "UDP: destination port is bound.\n"
    );

    uint16_t udp_length =
        ((uint16_t)payload[4] << 8) |
        payload[5];


    uint16_t received_checksum =
        ((uint16_t)payload[6] << 8) |
        payload[7];


    /*
     * ========================================================
     * VALIDATE LENGTH
     * ========================================================
     */

    if (
        udp_length < UDP_HEADER_SIZE
    )
    {
        print_string(
            "UDP: invalid datagram length.\n"
        );

        return false;
    }


    if (
        udp_length > length
    )
    {
        print_string(
            "UDP: datagram exceeds IPv4 payload.\n"
        );

        return false;
    }

    /*
     * ========================================================
     * UDP CHECKSUM
     * ========================================================
     */

    if (
        received_checksum != 0
    )
    {
        uint16_t calculated_checksum =
            udp_checksum(
                source,
                destination,
                payload,
                udp_length
            );


        if (
            calculated_checksum != 0
        )
        {
            print_string(
                "UDP: checksum invalid.\n"
            );

            return false;
        }


        print_string(
            "UDP: checksum valid.\n"
        );
    }
    else
    {
        print_string(
            "UDP: checksum disabled.\n"
        );
    }


    /*
     * ========================================================
     * PACKET INFORMATION
     * ========================================================
     */

    print_string(
        "UDP: datagram received.\n"
    );


    print_string(
        "UDP: source = "
    );

    ipv4_print_address(
        source
    );

    print_string(
        ":"
    );

    udp_print_decimal16(
        source_port
    );

    print_string(
        "\n"
    );


    print_string(
        "UDP: destination = "
    );

    ipv4_print_address(
        destination
    );

    print_string(
        ":"
    );

    udp_print_decimal16(
        destination_port
    );

    print_string(
        "\n"
    );


    print_string(
        "UDP: length = "
    );

    udp_print_decimal16(
        udp_length
    );

    print_string(
        "\n"
    );


    /*
     * ========================================================
     * PAYLOAD
     * ========================================================
     */

    uint16_t payload_length =
        udp_length -
        UDP_HEADER_SIZE;


    print_string(
        "UDP: payload length = "
    );

    udp_print_decimal16(
        payload_length
    );

    print_string(
        "\n"
    );

    if (payload_length > 0) {
        print_string(
            "UDP: payload = "
        );

        for (
            uint16_t i = 0;
            i < payload_length;
            i++
        )
        {
            char c = (char)payload[
                UDP_HEADER_SIZE + i
            ];

            if (c >= 32 && c <= 126)
            {
                char text[2];

                text[0] = c;
                text[1] = '\0';

                print_string(
                    text
                );
            }
            else
            {
                print_string(
                    "."
                );
            }
        }

        print_string(
            "\n"
        );
    }


    return true;
}


bool udp_poll(void)
{
    if (!udp_initialized)
    {
        return false;
    }


    return ipv4_poll();
}