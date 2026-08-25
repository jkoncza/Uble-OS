#include <stdint.h>
#include <stdbool.h>

#include "UDP.h"
#include "IPv4.h"


extern void print_string(
    const char *s
);


/*
 * ================================================================
 * UDP STATE
 * ================================================================
 */

static bool udp_initialized = false;

static udp_listener_t
    udp_listeners[UDP_MAX_LISTENERS];


/*
 * ================================================================
 * PRINT HELPERS
 * ================================================================
 */

static void udp_print_decimal16(
    uint16_t value
)
{
    char buffer[6];

    uint16_t count = 0;


    if (
        value == 0
    )
    {
        print_string(
            "0"
        );

        return;
    }


    while (
        value > 0 &&
        count < sizeof(buffer) - 1
    )
    {
        buffer[count++] =
            (char)(
                '0' +
                (value % 10)
            );

        value /= 10;
    }


    while (
        count > 0
    )
    {
        char output[2];

        output[0] =
            buffer[--count];

        output[1] =
            '\0';

        print_string(
            output
        );
    }
}


/*
 * ================================================================
 * ADDRESS COMPARISON
 * ================================================================
 */

static bool udp_ipv4_equal(
    ipv4_address_t a,
    ipv4_address_t b
)
{
    return
        a.bytes[0] == b.bytes[0] &&
        a.bytes[1] == b.bytes[1] &&
        a.bytes[2] == b.bytes[2] &&
        a.bytes[3] == b.bytes[3];
}


/*
 * ================================================================
 * BYTE ORDER
 * ================================================================
 */

static uint16_t udp_read_u16(
    const uint8_t *data
)
{
    return
        ((uint16_t)data[0] << 8) |
        (uint16_t)data[1];
}


static void udp_write_u16(
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
 * INITIALIZATION
 * ================================================================
 */

void udp_init(
    void
)
{
    udp_initialized =
        false;


    for (
        uint16_t i = 0;
        i < UDP_MAX_LISTENERS;
        i++
    )
    {
        udp_listeners[i].active =
            false;

        udp_listeners[i].port =
            0;
    }


    udp_initialized =
        true;


    print_string(
        "UDP: initialized.\n"
    );
}


/*
 * ================================================================
 * PORT BIND
 * ================================================================
 */

bool udp_bind(
    uint16_t port
)
{
    if (
        !udp_initialized
    )
    {
        return false;
    }


    /*
     * UDP port zero is not a valid listening port.
     */
    if (
        port == 0
    )
    {
        print_string(
            "UDP: cannot bind port 0.\n"
        );

        return false;
    }


    /*
     * Don't allow duplicate bindings.
     */
    if (
        udp_is_bound(
            port
        )
    )
    {
        print_string(
            "UDP: port already bound.\n"
        );

        return false;
    }


    for (
        uint16_t i = 0;
        i < UDP_MAX_LISTENERS;
        i++
    )
    {
        if (
            !udp_listeners[i].active
        )
        {
            udp_listeners[i].active =
                true;

            udp_listeners[i].port =
                port;


            print_string(
                "UDP: port bound successfully: "
            );

            udp_print_decimal16(
                port
            );

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
 * ================================================================
 * PORT UNBIND
 * ================================================================
 */

bool udp_unbind(
    uint16_t port
)
{
    if (
        !udp_initialized
    )
    {
        return false;
    }


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
            udp_listeners[i].active =
                false;

            udp_listeners[i].port =
                0;


            print_string(
                "UDP: port unbound: "
            );

            udp_print_decimal16(
                port
            );

            print_string(
                ".\n"
            );

            return true;
        }
    }


    return false;
}


/*
 * ================================================================
 * PORT LOOKUP
 * ================================================================
 */

bool udp_is_bound(
    uint16_t port
)
{
    if (
        !udp_initialized
    )
    {
        return false;
    }


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
 * ================================================================
 * UDP CHECKSUM
 * ================================================================
 *
 * IPv4 UDP checksum:
 *
 * pseudo-header:
 *
 *     source address
 *     destination address
 *     zero
 *     protocol
 *     UDP length
 *
 * followed by the complete UDP datagram.
 *
 * The caller must provide the UDP header with its checksum field
 * already populated when validating a received packet.
 */

uint16_t udp_checksum(
    ipv4_address_t source,
    ipv4_address_t destination,
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


    /*
     * ============================================================
     * SOURCE IP
     * ============================================================
     */

    sum +=
        ((uint16_t)source.bytes[0] << 8) |
        source.bytes[1];

    sum +=
        ((uint16_t)source.bytes[2] << 8) |
        source.bytes[3];


    /*
     * ============================================================
     * DESTINATION IP
     * ============================================================
     */

    sum +=
        ((uint16_t)destination.bytes[0] << 8) |
        destination.bytes[1];

    sum +=
        ((uint16_t)destination.bytes[2] << 8) |
        destination.bytes[3];


    /*
     * Zero + UDP protocol number.
     */
    sum +=
        UDP_PROTOCOL_NUMBER;


    /*
     * UDP length.
     */
    sum +=
        length;


    /*
     * ============================================================
     * UDP DATAGRAM
     * ============================================================
     */

    uint16_t i = 0;


    while (
        i + 1 < length
    )
    {
        sum +=
            ((uint16_t)data[i] << 8) |
            data[i + 1];

        i += 2;
    }


    /*
     * Odd byte is padded with zero.
     */
    if (
        i < length
    )
    {
        sum +=
            ((uint16_t)data[i] << 8);
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


    return
        (uint16_t)(~sum);
}


/*
 * ================================================================
 * UDP SEND
 * ================================================================
 */

bool udp_send(
    uint16_t source_port,
    ipv4_address_t destination,
    uint16_t destination_port,
    const uint8_t *payload,
    uint16_t payload_length
)
{
    if (
        !udp_initialized
    )
    {
        print_string(
            "UDP: not initialized.\n"
        );

        return false;
    }


    /*
     * Source port zero is not appropriate for this kernel's
     * current socket model.
     */
    if (
        source_port == 0
    )
    {
        print_string(
            "UDP: invalid source port.\n"
        );

        return false;
    }


    if (
        destination_port == 0
    )
    {
        print_string(
            "UDP: invalid destination port.\n"
        );

        return false;
    }


    if (
        payload_length > 0 &&
        payload == 0
    )
    {
        print_string(
            "UDP: NULL payload.\n"
        );

        return false;
    }


    /*
     * UDP must fit inside one IPv4 packet.
     */
    if (
        payload_length >
        UDP_MAX_PAYLOAD
    )
    {
        print_string(
            "UDP: payload exceeds IPv4 MTU.\n"
        );

        return false;
    }


    uint16_t udp_length =
        UDP_HEADER_SIZE +
        payload_length;


    /*
     * This cannot overflow because payload_length is already
     * limited to 1472.
     */
    if (
        udp_length < UDP_HEADER_SIZE
    )
    {
        return false;
    }


    /*
     * ============================================================
     * BUILD UDP DATAGRAM
     * ============================================================
     */

    uint8_t packet[
        UDP_HEADER_SIZE +
        UDP_MAX_PAYLOAD
    ];


    /*
     * Source port.
     */
    udp_write_u16(
        &packet[0],
        source_port
    );


    /*
     * Destination port.
     */
    udp_write_u16(
        &packet[2],
        destination_port
    );


    /*
     * UDP length.
     */
    udp_write_u16(
        &packet[4],
        udp_length
    );


    /*
     * Checksum initially zero.
     */
    packet[6] =
        0;

    packet[7] =
        0;


    /*
     * Copy payload.
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


    /*
     * ============================================================
     * CHECKSUM
     * ============================================================
     */

    uint16_t checksum =
        udp_checksum(
            ipv4_get_local_ip(),
            destination,
            packet,
            udp_length
        );


    /*
     * RFC UDP checksum has a special representation:
     *
     * a computed value of zero is transmitted as 0xFFFF.
     *
     * This is particularly important for IPv4 because zero means
     * checksum disabled.
     */
    if (
        checksum == 0
    )
    {
        checksum =
            0xFFFF;
    }


    udp_write_u16(
        &packet[6],
        checksum
    );


    /*
     * ============================================================
     * IPv4 TRANSMISSION
     * ============================================================
     */

    if (
        !ipv4_send(
            destination,
            UDP_PROTOCOL_NUMBER,
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
 * ================================================================
 * UDP RECEIVE
 * ================================================================
 *
 * Called by IPv4 after:
 *
 *     Ethernet validation
 *     IPv4 validation
 *     IPv4 checksum validation
 *     destination-IP validation
 *
 * The supplied buffer is:
 *
 *     UDP header + UDP payload
 */

bool udp_receive(
    ipv4_address_t source,
    ipv4_address_t destination,
    const uint8_t *packet,
    uint16_t length
)
{
    if (
        !udp_initialized
    )
    {
        return false;
    }


    if (
        packet == 0
    )
    {
        return false;
    }


    /*
     * Minimum UDP header.
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
     * ============================================================
     * UDP HEADER
     * ============================================================
     */

    uint16_t source_port =
        udp_read_u16(
            &packet[0]
        );


    uint16_t destination_port =
        udp_read_u16(
            &packet[2]
        );


    uint16_t udp_length =
        udp_read_u16(
            &packet[4]
        );


    uint16_t received_checksum =
        udp_read_u16(
            &packet[6]
        );


    /*
     * ============================================================
     * LENGTH VALIDATION
     * ============================================================
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


    /*
     * UDP length cannot extend beyond the IPv4 payload.
     */
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
     * There should never be a UDP packet larger than the IPv4
     * MTU-derived maximum in our current no-fragmentation stack.
     */
    if (
        udp_length >
        (
            UDP_HEADER_SIZE +
            UDP_MAX_PAYLOAD
        )
    )
    {
        print_string(
            "UDP: datagram exceeds supported size.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * PORT VALIDATION
     * ============================================================
     */

    /*
     * Port zero is not a valid destination for our socket layer.
     */
    if (
        destination_port == 0
    )
    {
        print_string(
            "UDP: invalid destination port.\n"
        );

        return false;
    }


    /*
     * We deliberately validate the complete datagram before
     * delivering it to a listener.
     */
    if (
        !udp_is_bound(
            destination_port
        )
    )
    {
        /*
         * In a future implementation this is where we could
         * generate ICMP "destination unreachable / port
         * unreachable".
         */
        print_string(
            "UDP: destination port not bound.\n"
        );

        return false;
    }


    /*
     * ============================================================
     * CHECKSUM
     * ============================================================
     *
     * For IPv4:
     *
     *     checksum == 0
     *
     * means the sender intentionally disabled UDP checksumming.
     *
     * Otherwise it MUST validate.
     */

    if (
        received_checksum != 0
    )
    {
        uint16_t calculated =
            udp_checksum(
                source,
                destination,
                packet,
                udp_length
            );


        if (
            calculated != 0
        )
        {
            print_string(
                "UDP: checksum invalid.\n"
            );

            return false;
        }
    }


    /*
     * ============================================================
     * INFORMATION
     * ============================================================
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
     * ============================================================
     * PAYLOAD
     * ============================================================
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


    if (
        payload_length > 0
    )
    {
        print_string(
            "UDP: payload = "
        );


        for (
            uint16_t i = 0;
            i < payload_length;
            i++
        )
        {
            char c =
                (char)packet[
                    UDP_HEADER_SIZE + i
                ];


            if (
                c >= 32 &&
                c <= 126
            )
            {
                char text[2];

                text[0] =
                    c;

                text[1] =
                    '\0';

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


    /*
     * UDP successfully consumed the datagram.
     */
    return true;
}

bool udp_poll(
    void
)
{
    if (
        !udp_initialized
    )
    {
        return false;
    }


    return ipv4_poll();
}
