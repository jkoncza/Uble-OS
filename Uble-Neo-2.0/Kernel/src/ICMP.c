#include <stdint.h>
#include <stdbool.h>

#include "ICMP.h"
#include "IPv4.h"

extern void print_string(const char *s);

static bool icmp_initialized = false;
static uint16_t icmp_pending_identifier = 0;
static uint16_t icmp_pending_sequence = 0;
static bool icmp_waiting_for_reply = false;

static void icmp_print_hex8(
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
 * PRINT DECIMAL
 * ================================================================
 */

void icmp_print_decimal16(
    uint16_t value
)
{
    char text[6];

    uint32_t divisor;
    uint32_t started = 0;
    uint32_t position = 0;

    divisor = 10000;

    while (divisor != 0)
    {
        uint32_t digit =
            value / divisor;

        value =
            value % divisor;

        if (
            digit != 0 ||
            started ||
            divisor == 1
        )
        {
            text[position++] =
                '0' + digit;

            started = 1;
        }

        divisor /= 10;
    }

    text[position] =
        '\0';

    print_string(text);
}


/*
 * ================================================================
 * CHECKSUM
 * ================================================================
 */

uint16_t icmp_checksum(
    const uint8_t *data,
    uint16_t length
)
{
    uint32_t sum = 0;

    uint16_t i = 0;


    while (
        i + 1 < length
    )
    {
        uint16_t word =
            ((uint16_t)data[i] << 8) |
            data[i + 1];

        sum += word;

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


    if (
        i < length
    )
    {
        sum +=
            ((uint16_t)data[i] << 8);

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
 * INITIALIZATION
 * ================================================================
 */

void icmp_init(void)
{
    icmp_initialized =
        true;

    print_string(
        "ICMP: initialized.\n"
    );
}


/*
 * ================================================================
 * SEND ECHO REQUEST
 * ================================================================
 */

bool icmp_send_echo_request(
    ipv4_address_t destination,
    uint16_t identifier,
    uint16_t sequence
)
{
    if (
        !icmp_initialized
    )
    {
        print_string(
            "ICMP: ERROR: not initialized.\n"
        );

        return false;
    }


    /*
     * ICMP Echo Request:
     *
     * 0  = Type
     * 1  = Code
     * 2-3 = Checksum
     * 4-5 = Identifier
     * 6-7 = Sequence
     *
     * 8+ = optional data
     */

    uint8_t packet[32] = { 0 };


    /*
     * Type = 8
     *
     * Echo Request.
     */
    packet[0] =
        ICMP_TYPE_ECHO_REQUEST;


    /*
     * Code = 0.
     */
    packet[1] =
        ICMP_CODE_ECHO;


    /*
     * Checksum initially zero.
     */
    packet[2] =
        0;

    packet[3] =
        0;


    /*
     * Identifier.
     */
    packet[4] =
        (uint8_t)(identifier >> 8);

    packet[5] =
        (uint8_t)(identifier & 0xFF);


    /*
     * Sequence.
     */
    packet[6] =
        (uint8_t)(sequence >> 8);

    packet[7] =
        (uint8_t)(sequence & 0xFF);


    /*
     * Simple test payload.
     */
    packet[8] =
        'U';

    packet[9] =
        'B';

    packet[10] =
        'L';

    packet[11] =
        'E';

    packet[12] =
        '-';

    packet[13] =
        'P';

    packet[14] =
        'I';

    packet[15] =
        'N';

    packet[16] =
        'G';


    /*
     * ICMP packet length.
     */
    uint16_t packet_length =
        17;


    /*
     * Calculate ICMP checksum.
     */
    uint16_t checksum =
        icmp_checksum(
            packet,
            packet_length
        );


    packet[2] =
        (uint8_t)(checksum >> 8);

    packet[3] =
        (uint8_t)(checksum & 0xFF);

    icmp_pending_identifier =
        identifier;

    icmp_pending_sequence =
        sequence;

    icmp_waiting_for_reply =
        true;

    print_string(
        "ICMP: sending Echo Request...\n"
    );


    print_string(
        "ICMP: destination = "
    );

    ipv4_print_address(
        destination
    );

    print_string(
        "\n"
    );


    print_string(
        "ICMP: identifier = "
    );

    icmp_print_decimal16(
        identifier
    );

    print_string(
        "\n"
    );


    print_string(
        "ICMP: sequence = "
    );

    icmp_print_decimal16(
        sequence
    );

    print_string(
        "\n"
    );


    print_string(
        "ICMP: checksum = 0x"
    );

    icmp_print_hex8(
        (uint8_t)(checksum >> 8)
    );

    icmp_print_hex8(
        (uint8_t)(checksum & 0xFF)
    );

    print_string(
        "\n"
    );


    /*
     * Protocol 1 = ICMP.
     */
    if (
        !ipv4_send(
            destination,
            1,
            packet,
            packet_length
        )
    )
    {
        print_string(
            "ICMP: Echo Request FAILED.\n"
        );

        return false;
    }


    print_string(
        "ICMP: Echo Request submitted.\n"
    );


    return true;
}


/*
 * ================================================================
 * RECEIVE ICMP
 * ================================================================
 */

bool icmp_receive(
    ipv4_address_t source,
    ipv4_address_t destination,
    const uint8_t *payload,
    uint16_t length
)
{
    if (
        length < 8
    )
    {
        print_string(
            "ICMP: packet too short.\n"
        );

        return false;
    }


    /*
     * Verify ICMP checksum.
     */
    if (
        icmp_checksum(
            payload,
            length
        ) != 0
    )
    {
        print_string(
            "ICMP: checksum invalid.\n"
        );

        return false;
    }


    uint8_t type =
        payload[0];

    uint8_t code =
        payload[1];


    /*
     * ============================================================
     * ECHO REPLY
     * ============================================================
     */

    if (
        type == ICMP_TYPE_ECHO_REPLY &&
        code == ICMP_CODE_ECHO
    )
    {
        uint16_t identifier =
            ((uint16_t)payload[4] << 8) |
            payload[5];

        uint16_t sequence =
            ((uint16_t)payload[6] << 8) |
            payload[7];

        if (!icmp_waiting_for_reply) {
            print_string(
                "ICMP: unexpected Echo Reply.\n"
            );

            return false;
        }


        if (
            identifier !=
            icmp_pending_identifier
        )
        {
            print_string(
                "ICMP: Echo Reply identifier mismatch.\n"
            );

            return false;
        }


        if (
            sequence !=
            icmp_pending_sequence
        )
        {
            print_string(
                "ICMP: Echo Reply sequence mismatch.\n"
            );

            return false;
        }

        print_string(
            "ICMP: ECHO REPLY received!\n"
        );


        print_string(
            "ICMP: source = "
        );

        ipv4_print_address(
            source
        );

        print_string(
            "\n"
        );


        print_string(
            "ICMP: destination = "
        );

        ipv4_print_address(
            destination
        );

        print_string(
            "\n"
        );


        print_string(
            "ICMP: identifier = "
        );

        icmp_print_decimal16(
            identifier
        );

        print_string(
            "\n"
        );


        print_string(
            "ICMP: sequence = "
        );

        icmp_print_decimal16(
            sequence
        );

        print_string(
            "\n"
        );


        print_string(
            "ICMP: checksum valid.\n"
        );

        icmp_waiting_for_reply = false;

        return true;
    }


    if (
        type == ICMP_TYPE_ECHO_REQUEST &&
        code == ICMP_CODE_ECHO
    )
    {
        print_string(
            "ICMP: ECHO REQUEST received.\n"
        );

        /*
        * ============================================================
        * BUILD ECHO REPLY
        * ============================================================
        */

        uint8_t reply[1500];

        if (length > sizeof(reply))
        {
            print_string(
                "ICMP: Echo Request too large.\n"
            );

            return false;
        }

        /*
        * Copy the complete ICMP request.
        */
        for (
            uint16_t i = 0;
            i < length;
            i++
        )
        {
            reply[i] = payload[i];
        }

        /*
        * Change:
        *
        *   Echo Request = 8
        *
        * to:
        *
        *   Echo Reply = 0
        */
        reply[0] = ICMP_TYPE_ECHO_REPLY;

        /*
        * Code stays zero.
        */
        reply[1] = ICMP_CODE_ECHO;

        /*
        * Clear the old checksum.
        */
        reply[2] = 0;
        reply[3] = 0;

        /*
        * Calculate the new checksum.
        */
        uint16_t checksum =
            icmp_checksum(
                reply,
                length
            );

        reply[2] =
            (uint8_t)(checksum >> 8);

        reply[3] =
            (uint8_t)(checksum & 0xFF);

        /*
        * Send the Echo Reply back to the sender.
        */
        if (
            !ipv4_send(
                source,
                IPV4_PROTOCOL_ICMP,
                reply,
                length
            )
        )
        {
            print_string(
                "ICMP: Echo Reply transmission FAILED.\n"
            );

            return false;
        }

        print_string(
            "ICMP: ECHO REPLY sent.\n"
        );

        return true;
    }

    print_string(
        "ICMP: unknown message type = 0x"
    );

    icmp_print_hex8(
        type
    );

    print_string(
        "\n"
    );

    return false;
}