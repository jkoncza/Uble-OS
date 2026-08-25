#include <stdint.h>
#include <stdbool.h>

#include "Ethernet.h"
#include "E1000.h"

extern void print_string(const char *s);

/*
 * ================================================================
 * INTERNAL STATE
 * ================================================================
 */

static bool ethernet_initialized = false;

/*
 * ================================================================
 * BYTE ORDER
 * ================================================================
 *
 * EtherType is stored in network byte order.
 */

static uint16_t ethernet_read_u16(
    const uint8_t *buffer
)
{
    return
        ((uint16_t)buffer[0] << 8) |
        (uint16_t)buffer[1];
}

/*
 * ================================================================
 * MAC HELPERS
 * ================================================================
 */

bool ethernet_is_broadcast_mac(
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
        i < ETHERNET_MAC_SIZE;
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

bool ethernet_is_multicast_mac(
    const uint8_t *mac
)
{
    if (
        mac == 0
    )
    {
        return false;
    }

    /*
     * Ethernet multicast addresses have the low bit of the first
     * octet set.
     */
    return (
        (mac[0] & 0x01) != 0
    );
}

/*
 * ================================================================
 * INITIALIZATION
 * ================================================================
 */

void ethernet_init(void)
{
    ethernet_initialized =
        true;

    print_string(
        "Ethernet: initialized.\n"
    );
}

/*
 * ================================================================
 * RECEIVE
 * ================================================================
 *
 * IMPORTANT:
 *
 * This is now the ONLY layer above E1000 that should directly call
 * e1000_receive().
 *
 * ARP, IPv4, ICMP, UDP, etc. will eventually receive frames from
 * this layer rather than directly consuming the E1000 RX ring.
 */

bool ethernet_receive(
    ethernet_frame_t *frame
)
{
    if (
        !ethernet_initialized
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

    uint16_t length = 0;

    if (
        !e1000_receive(
            frame->data,
            ETHERNET_FRAME_SIZE,
            &length
        )
    )
    {
        return false;
    }

    /*
     * Ethernet header must be present.
     */
    if (
        length <
        ETHERNET_HEADER_SIZE
    )
    {
        return false;
    }

    /*
     * Don't allow a malformed E1000 length to escape the Ethernet
     * layer.
     */
    if (
        length >
        ETHERNET_FRAME_SIZE
    )
    {
        return false;
    }

    frame->length =
        length;

    return true;
}

/*
 * ================================================================
 * ETHERTYPE
 * ================================================================
 */

uint16_t ethernet_get_ethertype(
    const ethernet_frame_t *frame
)
{
    if (
        frame == 0
    )
    {
        return 0;
    }

    if (
        frame->length <
        ETHERNET_HEADER_SIZE
    )
    {
        return 0;
    }

    return ethernet_read_u16(
        &frame->data[12]
    );
}

/*
 * ================================================================
 * MAC ACCESSORS
 * ================================================================
 */

const uint8_t *ethernet_destination_mac(
    const ethernet_frame_t *frame
)
{
    if (
        frame == 0
    )
    {
        return 0;
    }

    if (
        frame->length <
        ETHERNET_HEADER_SIZE
    )
    {
        return 0;
    }

    /*
     * Ethernet bytes 0-5 = destination.
     */
    return &frame->data[0];
}

const uint8_t *ethernet_source_mac(
    const ethernet_frame_t *frame
)
{
    if (
        frame == 0
    )
    {
        return 0;
    }

    if (
        frame->length <
        ETHERNET_HEADER_SIZE
    )
    {
        return 0;
    }

    /*
     * Ethernet bytes 6-11 = source.
     */
    return &frame->data[6];
}

/*
 * ================================================================
 * DESTINATION FILTER
 * ================================================================
 */

bool ethernet_is_for_us(
    const ethernet_frame_t *frame
)
{
    if (
        frame == 0
    )
    {
        return false;
    }

    const uint8_t *destination =
        ethernet_destination_mac(
            frame
        );

    if (
        destination == 0
    )
    {
        return false;
    }

    /*
     * Broadcast is always accepted.
     */
    if (
        ethernet_is_broadcast_mac(
            destination
        )
    )
    {
        return true;
    }

    /*
     * Multicast is currently accepted at this layer.
     *
     * Higher-level protocols can decide whether they want the
     * particular multicast address.
     */
    if (
        ethernet_is_multicast_mac(
            destination
        )
    )
    {
        return true;
    }

    const uint8_t *local_mac =
        e1000_get_mac();

    if (
        local_mac == 0
    )
    {
        return false;
    }

    for (
        uint32_t i = 0;
        i < ETHERNET_MAC_SIZE;
        i++
    )
    {
        if (
            destination[i] !=
            local_mac[i]
        )
        {
            return false;
        }
    }

    return true;
}
