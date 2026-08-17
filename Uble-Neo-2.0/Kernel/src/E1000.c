#include <stdint.h>
#include <stdbool.h>

#include "E1000.h"
#include "PCI.h"
#include "memory.h"

#define E1000_RX_STATUS_DD   0x01
#define E1000_RX_STATUS_EOP  0x02
/*
 * ================================================================
 * TX REGISTERS
 * ================================================================
 */

#define E1000_CTRL    0x0000
#define E1000_STATUS  0x0008

#define E1000_CTRL_FD   0x00000001
#define E1000_CTRL_SLU  0x00000040

#define E1000_STATUS_FD 0x00000001
#define E1000_STATUS_LU 0x00000002

#define E1000_TCTL   0x0400
#define E1000_TIPG   0x0410

#define E1000_TDBAL  0x3800
#define E1000_TDBAH  0x3804
#define E1000_TDLEN  0x3808
#define E1000_TDH    0x3810
#define E1000_TDT    0x3818

/*
 * ================================================================
 * TX DESCRIPTOR BITS
 * ================================================================
 */

#define E1000_TXD_CMD_EOP   0x01
#define E1000_TXD_CMD_IFCS  0x02
#define E1000_TXD_CMD_RS    0x08

#define E1000_TXD_STAT_DD   0x01
extern void print_string(const char *s);

/*
 * ================================================================
 * E1000 REGISTERS
 * ================================================================
 */

/*
 * Receive Descriptor Base Address Low
 */
#define E1000_RDBAL 0x2800

/*
 * Receive Descriptor Base Address High
 */
#define E1000_RDBAH 0x2804

/*
 * Receive Descriptor Length
 */
#define E1000_RDLEN 0x2808

/*
 * Receive Descriptor Head
 */
#define E1000_RDH 0x2810

/*
 * Receive Descriptor Tail
 */
#define E1000_RDT 0x2818

/*
 * Receive Control
 */
#define E1000_RCTL 0x0100

/*
 * Receive Address registers.
 */
#define E1000_RAL 0x5400
#define E1000_RAH 0x5404


/*
 * ================================================================
 * RCTL BITS
 * ================================================================
 */

#define E1000_RCTL_EN      (1u << 1)
#define E1000_RCTL_SBP     (1u << 2)
#define E1000_RCTL_UPE     (1u << 3)
#define E1000_RCTL_MPE     (1u << 4)
#define E1000_RCTL_LPE     (1u << 5)
#define E1000_RCTL_BAM     (1u << 15)

#define E1000_RCTL_BSIZE_2048  0x00000000u


/*
 * ================================================================
 * I/O BAR
 * ================================================================
 */

#define E1000_IO_ADDR 0x00
#define E1000_IO_DATA 0x04


/*
 * ================================================================
 * RX CONFIGURATION
 * ================================================================
 */

/*
 * 32 receive descriptors.
 *
 * 32 × 16 bytes = 512 bytes.
 */
#define E1000_RX_DESC_COUNT 32

/*
 * Each packet buffer is one 4 KiB page.
 */
#define E1000_RX_BUFFER_SIZE 4096


/*
 * ================================================================
 * RX DESCRIPTOR
 * ================================================================
 *
 * Legacy E1000 receive descriptor:
 *
 * 0x00 - buffer address
 * 0x08 - length / checksum
 * 0x0C - status / errors / VLAN
 *
 * Total = 16 bytes.
 */
typedef struct
{
    uint64_t buffer_address;

    uint16_t length;

    uint16_t checksum;

    uint8_t status;

    uint8_t errors;

    uint16_t special;

} __attribute__((packed)) e1000_rx_descriptor_t;


/*
 * ================================================================
 * TX DESCRIPTOR
 * ================================================================
 */

typedef struct {
    uint64_t buffer_address;
    uint16_t length;
    uint8_t cso;
    uint8_t command;
    uint8_t status;
    uint8_t css;
    uint16_t special;

} __attribute__((packed)) e1000_tx_descriptor_t;

/*
 * ================================================================
 * TX STATE
 * ================================================================
 */

#define E1000_TX_DESC_COUNT 32

#define E1000_TX_BUFFER_SIZE 2048

static e1000_tx_descriptor_t *e1000_tx_ring = 0;

static uint64_t e1000_tx_ring_physical = 0;

static uint8_t *e1000_tx_buffers[
    E1000_TX_DESC_COUNT
];

static uint64_t e1000_tx_buffer_physical[
    E1000_TX_DESC_COUNT
];

static uint32_t e1000_tx_tail = 0;

/*
 * ================================================================
 * DRIVER STATE
 * ================================================================
 */

static bool e1000_found = false;

static uint16_t e1000_io_base = 0;

static uint8_t e1000_mac[6];

static uint32_t e1000_rx_index = 0;
/*
 * RX descriptor ring.
 *
 * CPU accesses this through the HHDM virtual address.
 */
static e1000_rx_descriptor_t *e1000_rx_ring = 0;

/*
 * Physical address of descriptor ring.
 *
 * The E1000 uses THIS address for DMA.
 */
static uint64_t e1000_rx_ring_physical = 0;

/*
 * Physical addresses of packet buffers.
 */
static uint64_t e1000_rx_buffer_physical[
    E1000_RX_DESC_COUNT
];

/*
 * Virtual addresses of packet buffers.
 */
static uint8_t *e1000_rx_buffers[
    E1000_RX_DESC_COUNT
];


/*
 * ================================================================
 * PORT I/O
 * ================================================================
 */

static inline void e1000_outl(
    uint16_t port,
    uint32_t value
)
{
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(value), "dN"(port)
    );
}


static inline uint32_t e1000_inl(
    uint16_t port
)
{
    uint32_t value;

    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "dN"(port)
    );

    return value;
}


/*
 * ================================================================
 * REGISTER ACCESS
 * ================================================================
 */

static uint32_t e1000_read_reg(
    uint16_t reg
)
{
    e1000_outl(
        (uint16_t)(
            e1000_io_base +
            E1000_IO_ADDR
        ),
        reg
    );

    return e1000_inl(
        (uint16_t)(
            e1000_io_base +
            E1000_IO_DATA
        )
    );
}


static void e1000_write_reg(
    uint16_t reg,
    uint32_t value
)
{
    e1000_outl(
        (uint16_t)(
            e1000_io_base +
            E1000_IO_ADDR
        ),
        reg
    );

    e1000_outl(
        (uint16_t)(
            e1000_io_base +
            E1000_IO_DATA
        ),
        value
    );
}


/*
 * ================================================================
 * PRINTING
 * ================================================================
 */

void e1000_print_hex8(
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

    text[2] = '\0';

    print_string(text);
}


void e1000_print_hex16(
    uint16_t value
)
{
    static const char hex[] =
        "0123456789ABCDEF";

    char text[5];

    text[0] =
        hex[(value >> 12) & 0x0F];

    text[1] =
        hex[(value >> 8) & 0x0F];

    text[2] =
        hex[(value >> 4) & 0x0F];

    text[3] =
        hex[value & 0x0F];

    text[4] = '\0';

    print_string(text);
}


void e1000_print_hex32(
    uint32_t value
)
{
    static const char hex[] =
        "0123456789ABCDEF";

    char text[9];

    text[0] =
        hex[(value >> 28) & 0x0F];

    text[1] =
        hex[(value >> 24) & 0x0F];

    text[2] =
        hex[(value >> 20) & 0x0F];

    text[3] =
        hex[(value >> 16) & 0x0F];

    text[4] =
        hex[(value >> 12) & 0x0F];

    text[5] =
        hex[(value >> 8) & 0x0F];

    text[6] =
        hex[(value >> 4) & 0x0F];

    text[7] =
        hex[value & 0x0F];

    text[8] = '\0';

    print_string(text);
}


static void e1000_print_hex64(
    uint64_t value
)
{
    static const char hex[] =
        "0123456789ABCDEF";

    char text[17];

    text[0] =
        hex[(value >> 60) & 0x0F];

    text[1] =
        hex[(value >> 56) & 0x0F];

    text[2] =
        hex[(value >> 52) & 0x0F];

    text[3] =
        hex[(value >> 48) & 0x0F];

    text[4] =
        hex[(value >> 44) & 0x0F];

    text[5] =
        hex[(value >> 40) & 0x0F];

    text[6] =
        hex[(value >> 36) & 0x0F];

    text[7] =
        hex[(value >> 32) & 0x0F];

    text[8] =
        hex[(value >> 28) & 0x0F];

    text[9] =
        hex[(value >> 24) & 0x0F];

    text[10] =
        hex[(value >> 20) & 0x0F];

    text[11] =
        hex[(value >> 16) & 0x0F];

    text[12] =
        hex[(value >> 12) & 0x0F];

    text[13] =
        hex[(value >> 8) & 0x0F];

    text[14] =
        hex[(value >> 4) & 0x0F];

    text[15] =
        hex[value & 0x0F];

    text[16] = '\0';

    print_string(text);
}


static void e1000_print_mac(void)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        e1000_print_hex8(
            e1000_mac[i]
        );

        if (i != 5)
            print_string(":");
    }
}


/*
 * ================================================================
 * RX INITIALIZATION
 * ================================================================
 */

static bool e1000_init_rx(void)
{
    uint64_t ring_physical;

    print_string(
        "E1000: allocating RX descriptor ring...\n"
    );

    /*
     * Allocate one 4 KiB page.
     *
     * We only need 512 bytes for 32 descriptors,
     * but a full page is convenient and DMA-safe.
     */
    e1000_rx_ring =
        (e1000_rx_descriptor_t *)
        memory_alloc_page(
            &ring_physical
        );

    if (e1000_rx_ring == 0)
    {
        print_string(
            "E1000: ERROR: RX ring allocation failed.\n"
        );

        return false;
    }

    e1000_rx_ring_physical =
        ring_physical;

    /*
     * Clear the entire page.
     */
    for (
        uint32_t i = 0;
        i < MEMORY_PAGE_SIZE;
        i++
    )
    {
        ((uint8_t *)e1000_rx_ring)[i] = 0;
    }

    print_string(
        "E1000: RX ring virtual = 0x"
    );

    e1000_print_hex64(
        (uint64_t)e1000_rx_ring
    );

    print_string("\n");

    print_string(
        "E1000: RX ring physical = 0x"
    );

    e1000_print_hex64(
        e1000_rx_ring_physical
    );

    print_string("\n");


    /*
     * Allocate one 4 KiB page for each RX descriptor.
     */
    for (
        uint32_t i = 0;
        i < E1000_RX_DESC_COUNT;
        i++
    )
    {
        uint64_t physical;

        e1000_rx_buffers[i] =
            (uint8_t *)
            memory_alloc_page(
                &physical
            );

        if (
            e1000_rx_buffers[i] ==
            0
        )
        {
            print_string(
                "E1000: ERROR: RX buffer allocation failed.\n"
            );

            return false;
        }

        e1000_rx_buffer_physical[i] =
            physical;

        /*
         * Clear the packet buffer.
         */
        for (
            uint32_t j = 0;
            j < E1000_RX_BUFFER_SIZE;
            j++
        )
        {
            e1000_rx_buffers[i][j] = 0;
        }

        /*
         * Give the physical address to the NIC.
         */
        e1000_rx_ring[i].buffer_address =
            physical;

        /*
         * Clear descriptor state.
         */
        e1000_rx_ring[i].length = 0;
        e1000_rx_ring[i].checksum = 0;
        e1000_rx_ring[i].status = 0;
        e1000_rx_ring[i].errors = 0;
        e1000_rx_ring[i].special = 0;
    }

    print_string(
        "E1000: allocated "
    );

    e1000_print_hex8(
        E1000_RX_DESC_COUNT
    );

    print_string(
        " RX buffers.\n"
    );


    /*
     * Tell the E1000 where the descriptor ring lives.
     *
     * The 82540EM uses 64-bit descriptor addresses,
     * split between RDBAL and RDBAH.
     */
    e1000_write_reg(
        E1000_RDBAL,
        (uint32_t)(
            e1000_rx_ring_physical &
            0xFFFFFFFFULL
        )
    );

    e1000_write_reg(
        E1000_RDBAH,
        (uint32_t)(
            e1000_rx_ring_physical >>
            32
        )
    );

    /*
     * Ring size in bytes.
     *
     * 32 × 16 = 512.
     */
    e1000_write_reg(
        E1000_RDLEN,
        E1000_RX_DESC_COUNT *
        sizeof(e1000_rx_descriptor_t)
    );

    /*
     * Start at descriptor zero.
     */
    e1000_write_reg(
        E1000_RDH,
        0
    );

    /*
     * Give the hardware all 32 descriptors.
     *
     * RDT points to the last available descriptor.
     */
    e1000_write_reg(
        E1000_RDT,
        E1000_RX_DESC_COUNT - 1
    );


    /*
     * Configure receiver.
     *
     * EN  = enable receiver
     * BAM = accept broadcast packets
     * LPE = accept long packets
     *
     * BSIZE = 2048 bytes.
     */
    uint32_t rctl =
        E1000_RCTL_EN |
        E1000_RCTL_BAM |
        E1000_RCTL_LPE |
        E1000_RCTL_BSIZE_2048;

    e1000_write_reg(
        E1000_RCTL,
        rctl
    );

    print_string(
        "E1000: RX descriptor ring configured.\n"
    );

    print_string(
        "E1000: RDBAL = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_RDBAL)
    );

    print_string("\n");

    print_string(
        "E1000: RDBAH = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_RDBAH)
    );

    print_string("\n");

    print_string(
        "E1000: RDLEN = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_RDLEN)
    );

    print_string("\n");

    print_string(
        "E1000: RDH = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_RDH)
    );

    print_string("\n");

    print_string(
        "E1000: RDT = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_RDT)
    );

    print_string("\n");

    print_string(
        "E1000: RX enabled.\n"
    );

    return true;
}

/*
 * ================================================================
 * TX INITIALIZATION
 * ================================================================
 */

static bool e1000_init_tx(void)
{
    uint64_t ring_physical;

    print_string("E1000: allocating TX descriptor ring...\n");
    /*
     * 32 descriptors × 16 bytes = 512 bytes.
     *
     * Allocate a complete page.
     */
    e1000_tx_ring =
        (e1000_tx_descriptor_t *)
        memory_alloc_page(
            &ring_physical
        );

    if (e1000_tx_ring == 0)
    {
        print_string(
            "E1000: ERROR: TX ring allocation failed.\n"
        );

        return false;
    }

    e1000_tx_ring_physical =
        ring_physical;

    /*
     * Clear the page.
     */
    for (
        uint32_t i = 0;
        i < MEMORY_PAGE_SIZE;
        i++
    )
    {
        ((uint8_t *)e1000_tx_ring)[i] = 0;
    }

    print_string(
        "E1000: TX ring virtual = 0x"
    );

    e1000_print_hex64(
        (uint64_t)e1000_tx_ring
    );

    print_string("\n");

    print_string(
        "E1000: TX ring physical = 0x"
    );

    e1000_print_hex64(
        e1000_tx_ring_physical
    );

    print_string("\n");


    /*
     * Allocate packet buffers.
     */
    for (
        uint32_t i = 0;
        i < E1000_TX_DESC_COUNT;
        i++
    )
    {
        uint64_t physical;

        e1000_tx_buffers[i] =
            (uint8_t *)
            memory_alloc_page(
                &physical
            );

        if (
            e1000_tx_buffers[i] == 0
        )
        {
            print_string(
                "E1000: ERROR: TX buffer allocation failed.\n"
            );

            return false;
        }

        e1000_tx_buffer_physical[i] = physical;

        if (i == 0) {
            print_string("E1000: TX buffer[0] physical = 0x");
            e1000_print_hex64(physical);
            print_string("\n");
        }

        /*
         * Clear packet buffer.
         */
        for (
            uint32_t j = 0;
            j < E1000_TX_BUFFER_SIZE;
            j++
        )
        {
            e1000_tx_buffers[i][j] = 0;
        }

        /*
         * Give physical buffer address to NIC.
         */
        e1000_tx_ring[i].buffer_address =
            physical;

        if (i == 0) {
            print_string("E1000: TX descriptor[0] buffer = 0x");
            e1000_print_hex64(e1000_tx_ring[i].buffer_address);
            print_string("\n");
        }

        e1000_tx_ring[i].length = 0;
        e1000_tx_ring[i].cso = 0;
        e1000_tx_ring[i].command = 0;

        /*
         * Mark descriptor as available initially.
         *
         * This is important because our send code will
         * check DD before reusing a descriptor.
         */
        e1000_tx_ring[i].status =
            E1000_TXD_STAT_DD;

        e1000_tx_ring[i].css = 0;
        e1000_tx_ring[i].special = 0;
    }


    /*
     * Hardware starts with descriptor zero.
     */
    e1000_write_reg(
        E1000_TDBAL,
        (uint32_t)(
            e1000_tx_ring_physical &
            0xFFFFFFFFULL
        )
    );

    e1000_write_reg(
        E1000_TDBAH,
        (uint32_t)(
            e1000_tx_ring_physical >>
            32
        )
    );

    /*
     * 32 × 16 = 512 bytes.
     */
    e1000_write_reg(
        E1000_TDLEN,
        E1000_TX_DESC_COUNT *
        sizeof(e1000_tx_descriptor_t)
    );

    e1000_write_reg(
        E1000_TDH,
        0
    );

    e1000_write_reg(
        E1000_TDT,
        0
    );


    uint32_t tctl = 0x0000000A | (0x10u << 4) | (0x40u << 12);

    e1000_write_reg(E1000_TCTL,tctl);

    print_string("E1000: TCTL = 0x");

    e1000_print_hex32(e1000_read_reg(E1000_TCTL));

    print_string("\n");

    /*
     * Standard inter-packet gap configuration.
     */
    e1000_write_reg(
        E1000_TIPG,
        0x0060200A
    );


    e1000_tx_tail = 0;


    print_string(
        "E1000: TX descriptor ring configured.\n"
    );

    print_string(
        "E1000: TDBAL = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_TDBAL)
    );

    print_string("\n");

    print_string(
        "E1000: TDBAH = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_TDBAH)
    );

    print_string("\n");

    print_string(
        "E1000: TDLEN = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_TDLEN)
    );

    print_string("\n");

    print_string(
        "E1000: TDH = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_TDH)
    );

    print_string("\n");

    print_string(
        "E1000: TDT = 0x"
    );

    e1000_print_hex32(
        e1000_read_reg(E1000_TDT)
    );

    print_string("\n");

    print_string(
        "E1000: TX enabled.\n"
    );

    uint32_t ctrl;
    uint32_t status;

    ctrl = e1000_read_reg(E1000_CTRL);

    status = e1000_read_reg(E1000_STATUS);

    print_string("E1000: CTRL = 0x");

    e1000_print_hex32(ctrl);

    print_string("\n");

    print_string("E1000: STATUS = 0x");

    e1000_print_hex32(status);

    print_string("\n");

    if (status & E1000_STATUS_LU) {
        print_string("E1000: link is UP.\n");
    }
    else
    {
        print_string("E1000: link is DOWN.\n");
    }
    ctrl |= E1000_CTRL_SLU | E1000_CTRL_FD;

    e1000_write_reg(E1000_CTRL,ctrl);
    print_string("E1000: CTRL after link setup = 0x");

    e1000_print_hex32(e1000_read_reg(E1000_CTRL));

    status =
    e1000_read_reg(E1000_STATUS);

print_string(
    "E1000: STATUS after link setup = 0x"
);

e1000_print_hex32(status);

print_string("\n");

if (status & E1000_STATUS_LU)
{
    print_string(
        "E1000: link is UP.\n"
    );
}
else
{
    print_string(
        "E1000: link is STILL DOWN.\n"
    );
}

    print_string("\n");
    return true;
}

/*
 * ================================================================
 * SEND ETHERNET FRAME
 * ================================================================
 */

bool e1000_send(const uint8_t *data,uint16_t length) {
    e1000_tx_descriptor_t *descriptor;

    uint32_t index;

    /*
     * Ethernet frames are at least 60 bytes before
     * the NIC-generated FCS.
     */
    if (length < 60)
    {
        print_string(
            "E1000: TX frame too small.\n"
        );

        return false;
    }

    /*
     * Don't exceed our buffer.
     */
    if (
        length >
        E1000_TX_BUFFER_SIZE
    )
    {
        print_string(
            "E1000: TX frame too large.\n"
        );

        return false;
    }

    index =
        e1000_tx_tail;

    descriptor =
        &e1000_tx_ring[index];


    /*
     * Make sure the previous transmission using
     * this descriptor has finished.
     */
    if (
        (descriptor->status &
         E1000_TXD_STAT_DD) == 0
    )
    {
        print_string(
            "E1000: TX descriptor busy.\n"
        );

        return false;
    }


    /*
     * Copy packet into DMA buffer.
     */
    for (
        uint16_t i = 0;
        i < length;
        i++
    )
    {
        e1000_tx_buffers[index][i] =
            data[i];
    }


    /*
     * Set descriptor.
     */
    descriptor->length =
        length;

    descriptor->cso =
        0;

    descriptor->css =
        0;

    descriptor->special =
        0;

    /*
     * EOP:
     * end of packet.
     *
     * IFCS:
     * hardware generates Ethernet FCS.
     *
     * RS:
     * hardware sets DD when finished.
     */
    descriptor->command =
        E1000_TXD_CMD_EOP |
        E1000_TXD_CMD_IFCS |
        E1000_TXD_CMD_RS;

    /*
     * Hardware has NOT finished this descriptor yet.
     */
    descriptor->status = 0;


    /*
     * Move tail to the next descriptor.
     */
    e1000_tx_tail++;

    if (
        e1000_tx_tail >=
        E1000_TX_DESC_COUNT
    )
    {
        e1000_tx_tail = 0;
    }


    /*
     * Tell E1000 about the new descriptor.
     */
    e1000_write_reg(E1000_TDT,e1000_tx_tail);

    print_string("E1000: TDT now = 0x");

    e1000_print_hex32(e1000_read_reg(E1000_TDT));

    print_string("\n");

    print_string(
        "E1000: packet queued for TX.\n"
    );

    print_string(
        "E1000: descriptor = "
    );

    e1000_print_hex32(index);

    print_string("\n");

    print_string(
        "E1000: length = "
    );
    e1000_print_hex16(length);
    print_string(" bytes\n");
    return true;
}

/*
 * ================================================================
 * TX COMPLETION
 * ================================================================
 */

bool e1000_tx_complete(void)
{
    static bool printed = false;

    uint32_t tdh;
    uint32_t tdt;

    e1000_tx_descriptor_t *descriptor;

    uint32_t index;

    /*
     * Read what the hardware thinks the TX head/tail are.
     */
    tdh = e1000_read_reg(E1000_TDH);
    tdt = e1000_read_reg(E1000_TDT);

    if (!printed)
    {
        print_string(
            "E1000: TX hardware TDH = 0x"
        );

        e1000_print_hex32(tdh);

        print_string("\n");

        print_string(
            "E1000: TX hardware TDT = 0x"
        );

        e1000_print_hex32(tdt);

        print_string("\n");

        printed = true;
    }

    /*
     * First descriptor is the one we submitted.
     */
    index = 0;

    descriptor =
        &e1000_tx_ring[index];

    /*
     * Has hardware completed it?
     */
    if (
        (descriptor->status &
         E1000_TXD_STAT_DD) == 0
    )
    {
        return false;
    }

    return true;
}

/*
 * ================================================================
 * RECEIVE PACKET
 * ================================================================
 */

bool e1000_receive(
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *length
)
{
    e1000_rx_descriptor_t *descriptor;

    uint32_t index;

    uint16_t packet_length;

    /*
     * Current descriptor we're expecting the
     * E1000 to fill.
     */
    index =
        e1000_rx_index;

    descriptor =
        &e1000_rx_ring[index];

    /*
     * Has the E1000 received a packet here?
     */
    if (
        (descriptor->status &
         E1000_RX_STATUS_DD) == 0
    )
    {
        return false;
    }

    /*
     * Get packet length.
     */
    packet_length =
        descriptor->length;

    /*
     * Make sure the caller's buffer is large enough.
     */
    if (
        packet_length >
        buffer_size
    )
    {
        print_string(
            "E1000: RX packet too large.\n"
        );

        /*
         * We still need to recycle this descriptor.
         */
        descriptor->status = 0;

        e1000_write_reg(
            E1000_RDT,
            index
        );

        e1000_rx_index++;

        if (
            e1000_rx_index >=
            E1000_RX_DESC_COUNT
        )
        {
            e1000_rx_index = 0;
        }

        return false;
    }

    /*
     * We currently support one descriptor per packet.
     *
     * If EOP isn't set, the packet spans multiple
     * descriptors. We'll handle packet assembly later.
     */
    if (
        (descriptor->status &
         E1000_RX_STATUS_EOP) == 0
    )
    {
        print_string(
            "E1000: RX packet spans multiple descriptors.\n"
        );

        /*
         * Drop this fragment for now.
         */
        descriptor->status = 0;

        e1000_write_reg(
            E1000_RDT,
            index
        );

        e1000_rx_index++;

        if (
            e1000_rx_index >=
            E1000_RX_DESC_COUNT
        )
        {
            e1000_rx_index = 0;
        }

        return false;
    }

    /*
     * Copy packet from the DMA buffer into the
     * caller's buffer.
     */
    for (
        uint16_t i = 0;
        i < packet_length;
        i++
    )
    {
        buffer[i] =
            e1000_rx_buffers[index][i];
    }

    /*
     * Tell caller how many bytes were received.
     */
    *length =
        packet_length;

    /*
     * Descriptor is now free again.
     */
    descriptor->status = 0;

    descriptor->errors = 0;

    /*
     * Advance software RX index.
     */
    e1000_rx_index++;

    if (
        e1000_rx_index >=
        E1000_RX_DESC_COUNT
    )
    {
        e1000_rx_index = 0;
    }

    /*
     * Give the descriptor back to the E1000.
     *
     * IMPORTANT:
     *
     * We give back the descriptor we just consumed.
     */
    e1000_write_reg(
        E1000_RDT,
        index
    );

    return true;
}

/*
 * ================================================================
 * INITIALIZATION
 * ================================================================
 */

bool e1000_init(void)
{
    pci_device_t device;

    uint32_t bar;

    uint16_t command;

    uint32_t ral;
    uint32_t rah;


    print_string("\n");

    print_string(
        "========================================================\n"
    );

    print_string(
        "                     E1000 DRIVER\n"
    );

    print_string(
        "========================================================\n"
    );

    print_string(
        "E1000: searching for Intel 82540EM...\n"
    );


    /*
     * Find Intel 82540EM.
     */
    if (!pci_find_device(
        E1000_VENDOR_ID,
        E1000_DEVICE_ID,
        &device
    ))
    {
        print_string(
            "E1000: Intel 82540EM not found.\n"
        );

        return false;
    }

    e1000_found = true;


    print_string(
        "E1000: found at PCI "
    );

    e1000_print_hex8(
        device.bus
    );

    print_string(":");

    e1000_print_hex8(
        device.device
    );

    print_string(".");

    e1000_print_hex8(
        device.function
    );

    print_string("\n");


    /*
     * BAR2 is the I/O BAR we discovered.
     */
    bar =
        pci_get_bar(
            &device,
            2
        );

    print_string("E1000: BAR2 = 0x");

    e1000_print_hex32(bar);

    print_string("\n");


    if ((bar & 1u) == 0)
    {
        print_string(
            "E1000: ERROR: BAR2 is not an I/O BAR.\n"
        );

        return false;
    }


    /*
     * 0xC171 -> 0xC170
     */
    e1000_io_base =
        (uint16_t)(
            bar &
            0xFFFFFFFCu
        );


    print_string(
        "E1000: I/O base = 0x"
    );

    e1000_print_hex16(
        e1000_io_base
    );

    print_string("\n");


    /*
     * Enable PCI:
     *
     * bit 0 = I/O
     * bit 1 = Memory
     * bit 2 = Bus Master
     */
    command =
        pci_config_read16(
            device.bus,
            device.device,
            device.function,
            0x04
        );

    command |= 0x0007;


    pci_config_write16(
        device.bus,
        device.device,
        device.function,
        0x04,
        command
    );


    print_string(
        "E1000: PCI I/O + memory + bus mastering enabled.\n"
    );


    /*
     * Read MAC.
     */
    print_string(
        "E1000: reading MAC registers...\n"
    );


    ral =
        e1000_read_reg(
            E1000_RAL
        );

    rah =
        e1000_read_reg(
            E1000_RAH
        );


    print_string(
        "E1000: RAL = 0x"
    );

    e1000_print_hex32(ral);

    print_string("\n");


    print_string(
        "E1000: RAH = 0x"
    );

    e1000_print_hex32(rah);

    print_string("\n");


    /*
     * Decode MAC.
     */
    e1000_mac[0] =
        (uint8_t)(
            ral & 0xFF
        );

    e1000_mac[1] =
        (uint8_t)(
            (ral >> 8) & 0xFF
        );

    e1000_mac[2] =
        (uint8_t)(
            (ral >> 16) & 0xFF
        );

    e1000_mac[3] =
        (uint8_t)(
            (ral >> 24) & 0xFF
        );

    e1000_mac[4] =
        (uint8_t)(
            rah & 0xFF
        );

    e1000_mac[5] =
        (uint8_t)(
            (rah >> 8) & 0xFF
        );


    print_string(
        "E1000: MAC = "
    );

    e1000_print_mac();

    print_string("\n");


    /*
     * Check for invalid MAC.
     */
    if (
        e1000_mac[0] == 0 &&
        e1000_mac[1] == 0 &&
        e1000_mac[2] == 0 &&
        e1000_mac[3] == 0 &&
        e1000_mac[4] == 0 &&
        e1000_mac[5] == 0
    )
    {
        print_string(
            "E1000: ERROR: MAC is all zeroes.\n"
        );

        return false;
    }


    /*
     * Initialize receive DMA.
     */
    if (!e1000_init_rx()) {
        print_string("E1000: ERROR: RX initialization failed.\n");
        return false;
    }

    if (!e1000_init_tx()) {
        print_string("E1000: ERROR: TX initialization failed.\n");
        return false;
    }

    print_string(
        "E1000: initialization successful.\n"
    );

    print_string(
        "========================================================\n\n"
    );

    return true;
}


/*
 * ================================================================
 * PUBLIC API
 * ================================================================
 */

bool e1000_present(void)
{
    return e1000_found;
}


const uint8_t *e1000_get_mac(void)
{
    return e1000_mac;
}