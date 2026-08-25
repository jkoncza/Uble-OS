#include <stdint.h>
#include <stdbool.h>

#include <limine.h>

#include "memory.h"
#define MEMORY_MIN_PHYSICAL 0x00100000ULL
extern void print_string(const char *s);

/*
 * ================================================================
 * LIMINE REQUESTS
 * ================================================================
 *
 * We need:
 *
 * 1. The physical memory map.
 * 2. The Higher Half Direct Map offset.
 *
 * Limine will fill these responses before entering the kernel.
 */

/*
 * Memory map request.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memory_map_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = 0
};

/*
 * Higher Half Direct Map request.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = 0
};


/*
 * ================================================================
 * MEMORY MANAGER STATE
 * ================================================================
 */

/*
 * Whether memory_init() succeeded.
 */
static bool memory_initialized = false;

/*
 * HHDM virtual-address offset.
 *
 * Physical address:
 *
 *     0x0000000012345000
 *
 * HHDM offset:
 *
 *     0xFFFF800000000000
 *
 * Kernel virtual address:
 *
 *     0xFFFF9234512345000
 *
 * Conceptually:
 *
 *     virtual = physical + hhdm_offset
 */
static uint64_t memory_hhdm_offset = 0;

/*
 * Total usable physical memory reported by Limine.
 */
static uint64_t memory_total_usable = 0;

/*
 * Amount allocated by our simple allocator.
 */
static uint64_t memory_total_allocated = 0;

/*
 * Current region being used by the simple allocator.
 */
static uint64_t memory_current_region = 0;

/*
 * Current physical address inside that region.
 */
static uint64_t memory_current_address = 0;

/*
 * End of current usable region.
 */
static uint64_t memory_current_region_end = 0;


/*
 * ================================================================
 * HEX PRINT HELPERS
 * ================================================================
 */

static void memory_print_hex64(uint64_t value)
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


/*
 * ================================================================
 * ALIGNMENT
 * ================================================================
 */

/*
 * Align an address upward to a 4 KiB boundary.
 */
static uint64_t memory_align_up(
    uint64_t value
)
{
    return (
        value +
        (MEMORY_PAGE_SIZE - 1)
    ) &
    ~(MEMORY_PAGE_SIZE - 1);
}


/*
 * ================================================================
 * FIND NEXT USABLE REGION
 * ================================================================
 */

static bool memory_find_next_region(void)
{
    struct limine_memmap_response *response;

    response =
        memory_map_request.response;

    if (response == 0)
        return false;

    while (
        memory_current_region <
        response->entry_count
    )
    {
        struct limine_memmap_entry *entry;

        entry =
            response->entries[
                memory_current_region
            ];

        memory_current_region++;

        if (
            entry == 0 ||
            entry->type != LIMINE_MEMMAP_USABLE
        )
        {
            continue;
        }

        if (entry->length == 0)
            continue;

        /*
         * Calculate the end of this usable region.
         */
        uint64_t region_end =
            entry->base +
            entry->length;

        /*
         * Don't ever allocate below 1 MiB.
         */
        uint64_t start =
            entry->base;

        if (
            start <
            MEMORY_MIN_PHYSICAL
        )
        {
            start =
                MEMORY_MIN_PHYSICAL;
        }

        /*
         * Align to a 4 KiB page.
         */
        memory_current_address =
            memory_align_up(start);

        memory_current_region_end =
            region_end;

        /*
         * Make sure the resulting address is
         * actually inside the region.
         */
        if (
            memory_current_address >=
            memory_current_region_end
        )
        {
            continue;
        }

        return true;
    }

    return false;
}


/*
 * ================================================================
 * INITIALIZATION
 * ================================================================
 */

bool memory_init(void)
{
    struct limine_memmap_response *response;

    print_string("\n");
    print_string("========================================================\n");
    print_string("                  MEMORY MANAGER\n");
    print_string("========================================================\n");

    print_string(
        "MEM: requesting Limine memory map...\n"
    );

    response =
        memory_map_request.response;

    if (response == 0)
    {
        print_string(
            "MEM: ERROR: Limine memory map unavailable.\n"
        );

        return false;
    }

    /*
     * We also require HHDM.
     */
    if (hhdm_request.response == 0)
    {
        print_string(
            "MEM: ERROR: Limine HHDM unavailable.\n"
        );

        return false;
    }

    memory_hhdm_offset =
        hhdm_request.response->offset;

    print_string("MEM: HHDM offset = 0x");

    memory_print_hex64(
        memory_hhdm_offset
    );

    print_string("\n");

    /*
     * Walk every memory-map entry and count usable RAM.
     */
    memory_total_usable = 0;

    for (
        uint64_t i = 0;
        i < response->entry_count;
        i++
    )
    {
        struct limine_memmap_entry *entry;

        entry =
            response->entries[i];

        if (entry == 0)
            continue;

        if (
            entry->type ==
            LIMINE_MEMMAP_USABLE
        )
        {
            memory_total_usable +=
                entry->length;
        }
    }

    print_string(
        "MEM: usable physical memory = 0x"
    );

    memory_print_hex64(
        memory_total_usable
    );

    print_string(" bytes\n");

    /*
     * Start looking for allocatable memory.
     */
    memory_current_region = 0;
    memory_current_address = 0;
    memory_current_region_end = 0;

    if (!memory_find_next_region())
    {
        print_string(
            "MEM: ERROR: no usable memory regions.\n"
        );

        return false;
    }

    memory_total_allocated = 0;

    memory_initialized = true;

    print_string(
        "MEM: physical page allocator initialized.\n"
    );

    print_string(
        "MEM: page size = 4096 bytes.\n"
    );

    print_string(
        "========================================================\n\n"
    );

    return true;
}


/*
 * ================================================================
 * PHYSICAL → VIRTUAL
 * ================================================================
 */

void *memory_phys_to_virt(
    uint64_t physical_address
)
{
    if (!memory_initialized)
        return 0;

    return (
        void *)(physical_address +
                memory_hhdm_offset);
}


/*
 * ================================================================
 * VIRTUAL → PHYSICAL
 * ================================================================
 */

uint64_t memory_virt_to_phys(
    const void *virtual_address
)
{
    uint64_t virtual_address_value;

    if (!memory_initialized)
        return 0;

    virtual_address_value =
        (uint64_t)virtual_address;

    return (
        virtual_address_value -
        memory_hhdm_offset
    );
}

void *memory_alloc_pages(
    uint64_t page_count,
    uint64_t *physical_address
)
{
    uint64_t bytes;
    uint64_t allocation_address;
    uint64_t allocation_end;

    if (!memory_initialized)
        return 0;

    if (page_count == 0)
        return 0;


    if (memory_current_address < MEMORY_MIN_PHYSICAL)
    {
        memory_current_address = MEMORY_MIN_PHYSICAL;
        memory_current_address = memory_align_up(memory_current_address);
    }

    bytes =
        page_count *
        MEMORY_PAGE_SIZE;

    if (
        bytes / MEMORY_PAGE_SIZE !=
        page_count
    )
    {
        return 0;
    }

    while (
        memory_current_address +
        bytes >
        memory_current_region_end
    )
    {
        if (!memory_find_next_region())
            return 0;
    }

    allocation_address =
        memory_current_address;

    allocation_end =
        allocation_address +
        bytes;

    memory_current_address =
        allocation_end;

    memory_total_allocated += bytes;

    if (physical_address != 0)
    {
        *physical_address =
            allocation_address;
    }

    return memory_phys_to_virt(
        allocation_address
    );
}


/*
 * Allocate exactly one page.
 */
void *memory_alloc_page(
    uint64_t *physical_address
)
{
    return memory_alloc_pages(
        1,
        physical_address
    );
}


uint64_t memory_get_total_usable(void)
{
    return memory_total_usable;
}


uint64_t memory_get_allocated(void)
{
    return memory_total_allocated;
}


bool memory_is_initialized(void)
{
    return memory_initialized;
}