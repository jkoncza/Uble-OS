#include <stdint.h>
#include <stdbool.h>

#include "E1000.h"
#include "PCI.h"
#include "memory.h"

extern void print_string(const char *s);

/*
 * ================================================================
 * E1000 REGISTER DEFINITIONS
 * ================================================================
 */

/* Device control/status */
#define E1000_CTRL       0x0000
#define E1000_STATUS     0x0008
#define E1000_EERD       0x0014

/* Interrupt registers */
#define E1000_ICR        0x00C0
#define E1000_IMS        0x00D0
#define E1000_IMC        0x00D8

/* Receive */
#define E1000_RCTL       0x0100
#define E1000_RDBAL      0x2800
#define E1000_RDBAH      0x2804
#define E1000_RDLEN      0x2808
#define E1000_RDH        0x2810
#define E1000_RDT        0x2818

/* Transmit */
#define E1000_TCTL       0x0400
#define E1000_TIPG       0x0410
#define E1000_TDBAL      0x3800
#define E1000_TDBAH      0x3804
#define E1000_TDLEN      0x3808
#define E1000_TDH        0x3810
#define E1000_TDT        0x3818

/* Receive address */
#define E1000_RAL        0x5400
#define E1000_RAH        0x5404

/* Multicast table */
#define E1000_MTA        0x5200

/*
 * ================================================================
 * CTRL / STATUS BITS
 * ================================================================
 */

#define E1000_CTRL_FD    0x00000001u
#define E1000_CTRL_SLU   0x00000040u
#define E1000_CTRL_RST   0x04000000u

#define E1000_STATUS_FD  0x00000001u
#define E1000_STATUS_LU  0x00000002u

/*
 * RAH address-valid bit.
 */
#define E1000_RAH_AV     0x80000000u

/*
 * ================================================================
 * RECEIVE CONTROL BITS
 * ================================================================
 */

#define E1000_RCTL_EN       (1u << 1)
#define E1000_RCTL_SBP      (1u << 2)
#define E1000_RCTL_UPE      (1u << 3)
#define E1000_RCTL_MPE      (1u << 4)
#define E1000_RCTL_LPE      (1u << 5)
#define E1000_RCTL_BAM      (1u << 15)
#define E1000_RCTL_SECRC   (1u << 26)

/*
 * BSIZE = 00 selects 2048-byte receive buffers.
 */
#define E1000_RCTL_BSIZE_2048 0x00000000u

/*
 * ================================================================
 * TRANSMIT CONTROL
 * ================================================================
 */

#define E1000_TCTL_EN       (1u << 1)
#define E1000_TCTL_PSP      (1u << 3)

/*
 * ================================================================
 * TX DESCRIPTOR COMMAND / STATUS
 * ================================================================
 */

#define E1000_TXD_CMD_EOP  0x01
#define E1000_TXD_CMD_IFCS 0x02
#define E1000_TXD_CMD_RS   0x08

#define E1000_TXD_STAT_DD  0x01

/*
 * ================================================================
 * RX DESCRIPTOR STATUS / ERROR
 * ================================================================
 */

#define E1000_RX_STATUS_DD  0x01
#define E1000_RX_STATUS_EOP 0x02

#define E1000_RX_ERROR_CE   0x01
#define E1000_RX_ERROR_SE   0x02
#define E1000_RX_ERROR_SEQ  0x04
#define E1000_RX_ERROR_CXE  0x08
#define E1000_RX_ERROR_TCPE 0x20
#define E1000_RX_ERROR_IPE  0x40
#define E1000_RX_ERROR_RXE  0x80

/*
 * ================================================================
 * I/O BAR
 * ================================================================
 *
 * The QEMU 82540EM exposes the legacy register I/O BAR at BAR2.
 *
 * PCI support remains unchanged for this first driver revision.
 * MMIO support can be added later without changing the packet
 * interface.
 */

#define E1000_IO_ADDR 0x00
#define E1000_IO_DATA 0x04

/*
 * ================================================================
 * RING CONFIGURATION
 * ================================================================
 */

#define E1000_RX_DESC_COUNT 32
#define E1000_TX_DESC_COUNT 32

#define E1000_RX_BUFFER_SIZE 2048
#define E1000_TX_BUFFER_SIZE 2048

/*
 * ================================================================
 * RX DESCRIPTOR
 * ================================================================
 *
 * Legacy E1000 receive descriptor:
 *
 *   uint64_t buffer_address
 *   uint16_t length
 *   uint16_t checksum
 *   uint8_t  status
 *   uint8_t  errors
 *   uint16_t special
 *
 * Total: 16 bytes.
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

typedef struct
{
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
 * DRIVER STATE
 * ================================================================
 */

static bool e1000_found = false;

static uint16_t e1000_io_base = 0;

static uint8_t e1000_mac[6];

/*
 * ================================================================
 * RX STATE
 * ================================================================
 */

static e1000_rx_descriptor_t *e1000_rx_ring = 0;

static uint64_t e1000_rx_ring_physical = 0;

static uint8_t *e1000_rx_buffers[
    E1000_RX_DESC_COUNT
];

static uint64_t e1000_rx_buffer_physical[
    E1000_RX_DESC_COUNT
];

/*
 * Software index of the next RX descriptor that software
 * expects the hardware to have filled.
 */
static uint32_t e1000_rx_index = 0;

/*
 * ================================================================
 * TX STATE
 * ================================================================
 */

static e1000_tx_descriptor_t *e1000_tx_ring = 0;

static uint64_t e1000_tx_ring_physical = 0;

static uint8_t *e1000_tx_buffers[
    E1000_TX_DESC_COUNT
];

static uint64_t e1000_tx_buffer_physical[
    E1000_TX_DESC_COUNT
];

/*
 * tx_tail:
 *
 * The next descriptor software will submit.
 */
static uint32_t e1000_tx_tail = 0;

/*
 * tx_head:
 *
 * The oldest descriptor still outstanding.
 *
 * When tx_head == tx_tail the software queue is empty.
 */
static uint32_t e1000_tx_head = 0;

/*
 * ================================================================
 * MEMORY ORDERING
 * ================================================================
 *
 * Descriptor/buffer writes must happen before notifying the NIC
 * through TDT/RDT.
 *
 * This compiler barrier prevents the compiler from reordering
 * memory accesses across the ownership transition.
 */

static inline void e1000_memory_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

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
    for (
        uint8_t i = 0;
        i < 6;
        i++
    )
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
 * RX DESCRIPTOR RECYCLE
 * ================================================================
 */

static void e1000_recycle_rx_descriptor(
    uint32_t index
)
{
    e1000_rx_descriptor_t *descriptor =
        &e1000_rx_ring[index];

    /*
     * Clear fields that the hardware writes.
     */
    descriptor->length = 0;
    descriptor->checksum = 0;
    descriptor->status = 0;
    descriptor->errors = 0;
    descriptor->special = 0;

    /*
     * Make descriptor changes visible before giving ownership
     * back to the hardware.
     */
    e1000_memory_barrier();

    /*
     * RDT identifies the last descriptor made available to
     * the hardware.
     */
    e1000_write_reg(
        E1000_RDT,
        index
    );

    /*
     * Advance our software receive position.
     */
    e1000_rx_index++;

    if (
        e1000_rx_index >=
        E1000_RX_DESC_COUNT
    )
    {
        e1000_rx_index = 0;
    }
}

/*
 * ================================================================
 * CONTROLLER RESET
 * ================================================================
 */

static bool e1000_reset(void)
{
    uint32_t ctrl;

    print_string(
        "E1000: resetting controller...\n"
    );

    /*
     * Disable receiver and transmitter before reset.
     */
    e1000_write_reg(
        E1000_RCTL,
        0
    );

    e1000_write_reg(
        E1000_TCTL,
        0
    );

    /*
     * Mask interrupts for this polling-based driver.
     */
    e1000_write_reg(
        E1000_IMC,
        0xFFFFFFFFu
    );

    /*
     * Request hardware reset.
     */
    ctrl =
        e1000_read_reg(
            E1000_CTRL
        );

    ctrl |= E1000_CTRL_RST;

    e1000_write_reg(
        E1000_CTRL,
        ctrl
    );

    /*
     * Poll for reset completion.
     *
     * This is intentionally bounded so a broken device cannot
     * hang kernel initialization forever.
     */
    for (
        volatile uint32_t i = 0;
        i < 1000000u;
        i++
    )
    {
        ctrl =
            e1000_read_reg(
                E1000_CTRL
            );

        if (
            (ctrl &
             E1000_CTRL_RST) == 0
        )
        {
            print_string(
                "E1000: reset complete.\n"
            );

            return true;
        }
    }

    print_string(
        "E1000: ERROR: controller reset timed out.\n"
    );

    return false;
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

    e1000_rx_ring =
        (e1000_rx_descriptor_t *)
        memory_alloc_page(
            &ring_physical
        );

    if (
        e1000_rx_ring == 0
    )
    {
        print_string(
            "E1000: ERROR: RX ring allocation failed.\n"
        );

        return false;
    }

    e1000_rx_ring_physical =
        ring_physical;

    /*
     * Clear the entire descriptor page.
     */
    for (
        uint32_t i = 0;
        i < MEMORY_PAGE_SIZE;
        i++
    )
    {
        ((uint8_t *)e1000_rx_ring)[i] =
            0;
    }

    /*
     * Allocate one page per RX descriptor.
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
            e1000_rx_buffers[i] == 0
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
         * Clear the buffer.
         */
        for (
            uint32_t j = 0;
            j < E1000_RX_BUFFER_SIZE;
            j++
        )
        {
            e1000_rx_buffers[i][j] =
                0;
        }

        /*
         * Give the physical buffer address to hardware.
         */
        e1000_rx_ring[i].buffer_address =
            physical;

        e1000_rx_ring[i].length = 0;
        e1000_rx_ring[i].checksum = 0;

        /*
         * DD must initially be clear.
         * Hardware owns these descriptors after RDT is set.
         */
        e1000_rx_ring[i].status = 0;
        e1000_rx_ring[i].errors = 0;
        e1000_rx_ring[i].special = 0;
    }

    e1000_rx_index = 0;

    /*
     * Program RX descriptor ring physical address.
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

    e1000_write_reg(
        E1000_RDLEN,
        E1000_RX_DESC_COUNT *
        sizeof(e1000_rx_descriptor_t)
    );

    /*
     * Hardware starts at descriptor zero.
     */
    e1000_write_reg(
        E1000_RDH,
        0
    );

    /*
     * Give hardware all descriptors.
     */
    e1000_write_reg(
        E1000_RDT,
        E1000_RX_DESC_COUNT - 1
    );

    /*
     * Clear multicast table.
     *
     * We do not implement multicast filtering yet.
     */
    for (
        uint32_t offset = 0;
        offset < 0x200;
        offset += 4
    )
    {
        e1000_write_reg(
            E1000_MTA + offset,
            0
        );
    }

    /*
     * Receiver configuration:
     *
     * EN      = enable
     * BAM     = accept broadcasts
     * SECRC   = strip Ethernet CRC from received frames
     * BSIZE   = 2048-byte buffers
     *
     * We deliberately do not enable LPE because this driver
     * currently handles one normal Ethernet frame per descriptor.
     */
    uint32_t rctl =
        E1000_RCTL_EN |
        E1000_RCTL_BAM |
        E1000_RCTL_SECRC |
        E1000_RCTL_BSIZE_2048;

    e1000_write_reg(
        E1000_RCTL,
        rctl
    );

    e1000_memory_barrier();

    print_string(
        "E1000: RX initialized.\n"
    );

    print_string(
        "E1000: RX ring physical = 0x"
    );

    e1000_print_hex64(
        e1000_rx_ring_physical
    );

    print_string("\n");

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

    print_string(
        "E1000: allocating TX descriptor ring...\n"
    );

    e1000_tx_ring =
        (e1000_tx_descriptor_t *)
        memory_alloc_page(
            &ring_physical
        );

    if (
        e1000_tx_ring == 0
    )
    {
        print_string(
            "E1000: ERROR: TX ring allocation failed.\n"
        );

        return false;
    }

    e1000_tx_ring_physical =
        ring_physical;

    /*
     * Clear descriptor page.
     */
    for (
        uint32_t i = 0;
        i < MEMORY_PAGE_SIZE;
        i++
    )
    {
        ((uint8_t *)e1000_tx_ring)[i] =
            0;
    }

    /*
     * Allocate one DMA buffer per descriptor.
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

        e1000_tx_buffer_physical[i] =
            physical;

        /*
         * Clear DMA buffer.
         */
        for (
            uint32_t j = 0;
            j < E1000_TX_BUFFER_SIZE;
            j++
        )
        {
            e1000_tx_buffers[i][j] =
                0;
        }

        e1000_tx_ring[i].buffer_address =
            physical;

        e1000_tx_ring[i].length = 0;
        e1000_tx_ring[i].cso = 0;
        e1000_tx_ring[i].command = 0;

        /*
         * DD means software owns this descriptor and it is
         * available for reuse.
         */
        e1000_tx_ring[i].status =
            E1000_TXD_STAT_DD;

        e1000_tx_ring[i].css = 0;
        e1000_tx_ring[i].special = 0;
    }

    e1000_tx_head = 0;
    e1000_tx_tail = 0;

    e1000_memory_barrier();

    /*
     * Program TX ring.
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

    e1000_write_reg(
        E1000_TDLEN,
        E1000_TX_DESC_COUNT *
        sizeof(e1000_tx_descriptor_t)
    );

    /*
     * Hardware starts at descriptor zero.
     */
    e1000_write_reg(
        E1000_TDH,
        0
    );

    e1000_write_reg(
        E1000_TDT,
        0
    );

    /*
     * TCTL:
     *
     * EN  = transmitter enable
     * PSP = pad short packets
     * CT  = collision threshold
     * COLD = collision distance
     */
    uint32_t tctl =
        E1000_TCTL_EN |
        E1000_TCTL_PSP |
        (0x10u << 4) |
        (0x40u << 12);

    e1000_write_reg(
        E1000_TCTL,
        tctl
    );

    /*
     * Standard inter-packet gap.
     */
    e1000_write_reg(
        E1000_TIPG,
        0x0060200A
    );

    print_string(
        "E1000: TX initialized.\n"
    );

    print_string(
        "E1000: TX ring physical = 0x"
    );

    e1000_print_hex64(
        e1000_tx_ring_physical
    );

    print_string("\n");

    return true;
}

/*
 * ================================================================
 * TX COMPLETION
 * ================================================================
 *
 * This fixes a major bug in the original implementation.
 *
 * The old driver always checked descriptor zero.
 *
 * This version tracks the oldest outstanding descriptor with
 * e1000_tx_head and reclaims completed descriptors in order.
 */

bool e1000_tx_complete(void)
{
    bool reclaimed = false;

    if (
        !e1000_found ||
        e1000_tx_ring == 0
    )
    {
        return false;
    }

    /*
     * Walk from oldest outstanding descriptor to the software
     * tail.
     */
    while (
        e1000_tx_head !=
        e1000_tx_tail
    )
    {
        e1000_tx_descriptor_t *descriptor =
            &e1000_tx_ring[
                e1000_tx_head
            ];

        /*
         * Hardware still owns this descriptor.
         */
        if (
            (descriptor->status &
             E1000_TXD_STAT_DD) == 0
        )
        {
            break;
        }

        /*
         * Hardware is finished. Software can reuse it.
         */
        descriptor->length = 0;
        descriptor->cso = 0;
        descriptor->command = 0;
        descriptor->css = 0;
        descriptor->special = 0;

        /*
         * Keep DD set because the descriptor is now available
         * to software.
         */
        descriptor->status =
            E1000_TXD_STAT_DD;

        e1000_tx_head++;

        if (
            e1000_tx_head >=
            E1000_TX_DESC_COUNT
        )
        {
            e1000_tx_head = 0;
        }

        reclaimed = true;
    }

    return reclaimed;
}

/*
 * ================================================================
 * SEND ETHERNET FRAME
 * ================================================================
 */

bool e1000_send(
    const uint8_t *data,
    uint16_t length
)
{
    uint32_t index;

    e1000_tx_descriptor_t *descriptor;

    if (
        !e1000_found ||
        e1000_tx_ring == 0 ||
        data == 0
    )
    {
        return false;
    }

    /*
     * Ethernet frame without FCS must be at least 60 bytes.
     * The NIC generates the FCS itself.
     */
    if (length < 60)
    {
        print_string(
            "E1000: TX frame too small.\n"
        );

        return false;
    }

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

    /*
     * Reclaim any descriptors hardware has already completed.
     */
    e1000_tx_complete();

    index =
        e1000_tx_tail;

    descriptor =
        &e1000_tx_ring[index];

    /*
     * If the next descriptor is not available, the ring is full.
     */
    if (
        (descriptor->status &
         E1000_TXD_STAT_DD) == 0
    )
    {
        print_string(
            "E1000: TX ring full.\n"
        );

        return false;
    }

    /*
     * Copy packet into the DMA buffer.
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
     * Populate descriptor.
     */
    descriptor->length =
        length;

    descriptor->cso = 0;

    descriptor->css = 0;

    descriptor->special = 0;

    descriptor->command =
        E1000_TXD_CMD_EOP |
        E1000_TXD_CMD_IFCS |
        E1000_TXD_CMD_RS;

    /*
     * Clear DD.
     *
     * This transfers ownership from software to hardware.
     */
    descriptor->status = 0;

    /*
     * Make packet and descriptor writes visible before updating
     * the hardware tail register.
     */
    e1000_memory_barrier();

    /*
     * Advance software tail.
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
     * Notify hardware.
     */
    e1000_write_reg(
        E1000_TDT,
        e1000_tx_tail
    );

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
    uint32_t index;

    e1000_rx_descriptor_t *descriptor;

    uint16_t packet_length;

    if (
        !e1000_found ||
        e1000_rx_ring == 0 ||
        buffer == 0 ||
        length == 0
    )
    {
        return false;
    }

    index =
        e1000_rx_index;

    descriptor =
        &e1000_rx_ring[index];

    /*
     * Hardware has not filled this descriptor yet.
     */
    if (
        (descriptor->status &
         E1000_RX_STATUS_DD) == 0
    )
    {
        return false;
    }

    packet_length =
        descriptor->length;

    /*
     * Reject invalid packet lengths.
     */
    if (
        packet_length == 0 ||
        packet_length >
        E1000_RX_BUFFER_SIZE
    )
    {
        print_string(
            "E1000: dropping invalid RX packet length.\n"
        );

        e1000_recycle_rx_descriptor(
            index
        );

        return false;
    }

    /*
     * Reject hardware-reported receive errors.
     */
    if (
        descriptor->errors != 0
    )
    {
        print_string(
            "E1000: dropping RX packet with hardware errors.\n"
        );

        e1000_recycle_rx_descriptor(
            index
        );

        return false;
    }

    /*
     * This driver currently expects one complete Ethernet frame
     * in one descriptor.
     */
    if (
        (descriptor->status &
         E1000_RX_STATUS_EOP) == 0
    )
    {
        print_string(
            "E1000: RX packet spans multiple descriptors; dropping.\n"
        );

        e1000_recycle_rx_descriptor(
            index
        );

        return false;
    }

    /*
     * Check caller's destination buffer.
     */
    if (
        packet_length >
        buffer_size
    )
    {
        print_string(
            "E1000: RX destination buffer too small.\n"
        );

        e1000_recycle_rx_descriptor(
            index
        );

        return false;
    }

    /*
     * Copy DMA buffer into caller's buffer.
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

    *length =
        packet_length;

    /*
     * Return descriptor to hardware.
     */
    e1000_recycle_rx_descriptor(
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

    uint32_t ctrl;
    uint32_t status;

    /*
     * Reset state in case initialization is called again.
     */
    e1000_found = false;

    e1000_io_base = 0;

    e1000_rx_ring = 0;
    e1000_tx_ring = 0;

    e1000_rx_ring_physical = 0;
    e1000_tx_ring_physical = 0;

    e1000_rx_index = 0;

    e1000_tx_head = 0;
    e1000_tx_tail = 0;

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        e1000_mac[i] = 0;
    }

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
     * Find the QEMU/Intel 82540EM PCI device.
     */
    if (
        !pci_find_device(
            E1000_VENDOR_ID,
            E1000_DEVICE_ID,
            &device
        )
    )
    {
        print_string(
            "E1000: Intel 82540EM not found.\n"
        );

        return false;
    }

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
     * The current PCI layer exposes BARs as 32-bit values.
     *
     * For the Intel 82540EM used by Uble/QEMU, BAR2 is the
     * legacy I/O register BAR.
     */
    bar =
        pci_get_bar(
            &device,
            2
        );

    print_string(
        "E1000: BAR2 = 0x"
    );

    e1000_print_hex32(
        bar
    );

    print_string("\n");

    if (
        (bar & 1u) == 0
    )
    {
        print_string(
            "E1000: ERROR: BAR2 is not an I/O BAR.\n"
        );

        return false;
    }

    /*
     * Mask the I/O BAR attribute bits.
     */
    e1000_io_base =
        (uint16_t)(
            bar &
            0xFFFFFFFCu
        );

    if (
        e1000_io_base == 0
    )
    {
        print_string(
            "E1000: ERROR: invalid I/O base.\n"
        );

        return false;
    }

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
     * bit 0 = I/O space
     * bit 1 = memory space
     * bit 2 = bus mastering
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

    /*
     * Confirm bus mastering is enabled.
     */
    command =
        pci_config_read16(
            device.bus,
            device.device,
            device.function,
            0x04
        );

    if (
        (command & 0x0004) == 0
    )
    {
        print_string(
            "E1000: ERROR: PCI bus mastering is disabled.\n"
        );

        return false;
    }

    print_string(
        "E1000: PCI I/O + memory + bus mastering enabled.\n"
    );

    /*
     * Reset controller before configuring rings.
     */
    if (
        !e1000_reset()
    )
    {
        return false;
    }

    /*
     * Set software link configuration.
     *
     * SLU = set link up
     * FD  = full duplex
     */
    ctrl =
        e1000_read_reg(
            E1000_CTRL
        );

    ctrl |=
        E1000_CTRL_SLU |
        E1000_CTRL_FD;

    e1000_write_reg(
        E1000_CTRL,
        ctrl
    );

    /*
     * Read current hardware status.
     */
    status =
        e1000_read_reg(
            E1000_STATUS
        );

    print_string(
        "E1000: CTRL = 0x"
    );

    e1000_print_hex32(
        ctrl
    );

    print_string("\n");

    print_string(
        "E1000: STATUS = 0x"
    );

    e1000_print_hex32(
        status
    );

    print_string("\n");

    if (
        status &
        E1000_STATUS_LU
    )
    {
        print_string(
            "E1000: link is UP.\n"
        );
    }
    else
    {
        /*
         * Link state is not an initialization failure.
         * The virtual/physical link can become active later.
         */
        print_string(
            "E1000: link is currently DOWN.\n"
        );
    }

    /*
     * Read factory-programmed MAC address.
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
     * Reject obviously invalid MAC addresses.
     */
    bool mac_all_zero =
        true;

    bool mac_all_ff =
        true;

    for (
        uint32_t i = 0;
        i < 6;
        i++
    )
    {
        if (
            e1000_mac[i] != 0
        )
        {
            mac_all_zero = false;
        }

        if (
            e1000_mac[i] != 0xFF
        )
        {
            mac_all_ff = false;
        }
    }

    if (
        mac_all_zero ||
        mac_all_ff
    )
    {
        print_string(
            "E1000: ERROR: invalid MAC address.\n"
        );

        return false;
    }

    /*
     * Explicitly program receive address slot 0.
     *
     * RAH bit 31 marks the entry valid.
     */
    e1000_write_reg(
        E1000_RAL,
        ral
    );

    e1000_write_reg(
        E1000_RAH,
        (rah & 0x0000FFFFu) |
        E1000_RAH_AV
    );

    /*
     * Initialize RX before enabling traffic.
     */
    if (
        !e1000_init_rx()
    )
    {
        print_string(
            "E1000: ERROR: RX initialization failed.\n"
        );

        return false;
    }

    /*
     * Initialize TX.
     */
    if (
        !e1000_init_tx()
    )
    {
        print_string(
            "E1000: ERROR: TX initialization failed.\n"
        );

        return false;
    }

    /*
     * Re-apply link configuration after ring initialization.
     */
    ctrl =
        e1000_read_reg(
            E1000_CTRL
        );

    ctrl |=
        E1000_CTRL_SLU |
        E1000_CTRL_FD;

    e1000_write_reg(
        E1000_CTRL,
        ctrl
    );

    status =
        e1000_read_reg(
            E1000_STATUS
        );

    if (
        status &
        E1000_STATUS_LU
    )
    {
        print_string(
            "E1000: link is UP.\n"
        );
    }
    else
    {
        print_string(
            "E1000: link is currently DOWN.\n"
        );
    }

    e1000_found = true;

    print_string(
        "E1000: initialization successful.\n"
    );

    print_string(
        "========================================================\n\n"
    );

    return true;
}

bool e1000_present(void)
{
    return e1000_found;
}

const uint8_t *e1000_get_mac(void)
{
    return e1000_mac;
}
