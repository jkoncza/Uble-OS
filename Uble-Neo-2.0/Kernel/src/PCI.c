#include <stdint.h>
#include <stdbool.h>

#include "PCI.h"

static inline void pci_outl(uint16_t port, uint32_t value)
{
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a"(value), "dN"(port)
    );
}

static inline uint32_t pci_inl(uint16_t port)
{
    uint32_t value;

    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "dN"(port)
    );

    return value;
}

extern void print_string(const char *s);

static uint32_t pci_config_address(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    return
        0x80000000u |
        ((uint32_t)bus      << 16) |
        ((uint32_t)device   << 11) |
        ((uint32_t)function << 8)  |
        ((uint32_t)offset   & 0xFC);
}

/*
 * Read one 32-bit PCI configuration register.
 */
uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    uint32_t address = pci_config_address(
        bus,
        device,
        function,
        offset
    );

    pci_outl(0xCF8, address);

    return pci_inl(0xCFC);
}

/*
 * Read a 16-bit value from a PCI configuration register.
 */
uint16_t pci_config_read16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    uint32_t value = pci_config_read32(
        bus,
        device,
        function,
        offset
    );

    if (offset & 2)
        return (uint16_t)(value >> 16);

    return (uint16_t)(value & 0xFFFF);
}

/*
 * Read an 8-bit value from a PCI configuration register.
 */
uint8_t pci_config_read8(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    uint32_t value = pci_config_read32(
        bus,
        device,
        function,
        offset
    );

    uint8_t shift = (uint8_t)((offset & 3) * 8);

    return (uint8_t)((value >> shift) & 0xFF);
}

static void pci_print_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char text[3];

    text[0] = hex[(value >> 4) & 0x0F];
    text[1] = hex[value & 0x0F];
    text[2] = '\0';

    print_string(text);
}

static void pci_print_hex16(uint16_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    char text[5];

    text[0] = hex[(value >> 12) & 0x0F];
    text[1] = hex[(value >> 8) & 0x0F];
    text[2] = hex[(value >> 4) & 0x0F];
    text[3] = hex[value & 0x0F];
    text[4] = '\0';

    print_string(text);
}

static bool found_device = false;

/*
 * Print one PCI device.
 */
static void pci_print_device(
    uint8_t bus,
    uint8_t device,
    uint8_t function
)
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t header_type;

    vendor_id = pci_config_read16(
        bus,
        device,
        function,
        0x00
    );

    /*
     * 0xFFFF means there is no device at this
     * bus/device/function combination.
     */
    if (vendor_id == 0xFFFF)
        return;

    device_id = pci_config_read16(
        bus,
        device,
        function,
        0x02
    );

    class_code = pci_config_read8(
        bus,
        device,
        function,
        0x0B
    );

    subclass = pci_config_read8(
        bus,
        device,
        function,
        0x0A
    );

    prog_if = pci_config_read8(
        bus,
        device,
        function,
        0x09
    );

    header_type = pci_config_read8(
        bus,
        device,
        function,
        0x0E
    );

    found_device = true;

    print_string("PCI: ");

    print_string("bus=");
    pci_print_hex8(bus);

    print_string(" device=");
    pci_print_hex8(device);

    print_string(" function=");
    pci_print_hex8(function);

    print_string("\n");

    print_string("     vendor=0x");
    pci_print_hex16(vendor_id);

    print_string(" device=0x");
    pci_print_hex16(device_id);

    print_string("\n");

    print_string("     class=0x");
    pci_print_hex8(class_code);

    print_string(" subclass=0x");
    pci_print_hex8(subclass);

    print_string(" prog_if=0x");
    pci_print_hex8(prog_if);

    print_string("\n");

    print_string("     header=0x");
    pci_print_hex8(header_type);

    print_string("\n");
}

/*
 * Scan one PCI function.
 */
static void pci_scan_function(
    uint8_t bus,
    uint8_t device,
    uint8_t function
)
{
    uint16_t vendor_id;

    vendor_id = pci_config_read16(
        bus,
        device,
        function,
        0x00
    );

    if (vendor_id == 0xFFFF)
        return;

    pci_print_device(
        bus,
        device,
        function
    );
}

static void pci_scan_device(
    uint8_t bus,
    uint8_t device
)
{
    uint16_t vendor_id;
    uint8_t header_type;

    vendor_id = pci_config_read16(
        bus,
        device,
        0,
        0x00
    );

    if (vendor_id == 0xFFFF)
        return;

    pci_scan_function(
        bus,
        device,
        0
    );

    header_type = pci_config_read8(
        bus,
        device,
        0,
        0x0E
    );

    if (header_type & 0x80)
    {
        for (uint8_t function = 1; function < 8; function++)
        {
            pci_scan_function(
                bus,
                device,
                function
            );
        }
    }
}

static void pci_scan_bus(uint8_t bus)
{
    for (uint8_t device = 0; device < 32; device++)
    {
        pci_scan_device(
            bus,
            device
        );
    }
}

void pci_init(void)
{
    found_device = false;

    print_string("\n");
    print_string("========================================================\n");
    print_string("                     PCI ENUMERATION\n");
    print_string("========================================================\n");

    print_string("PCI: scanning buses...\n");

    for (uint16_t bus = 0; bus < 256; bus++)
    {
        pci_scan_bus((uint8_t)bus);
    }

    if (found_device)
    {
        print_string("PCI: device enumeration complete.\n");
    }
    else
    {
        print_string("PCI: no devices found.\n");
    }

    print_string("========================================================\n\n");
}

bool pci_has_devices(void) {
    return found_device;
}

void pci_config_write16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
) {
    uint32_t address = pci_config_address(
        bus,
        device,
        function,
        offset
    );
    uint32_t current;
    pci_outl(0xCF8, address);
    current = pci_inl(0xCFC);
    if (offset & 2) {
        current &= 0x0000FFFFu;
        current |= ((uint32_t)value << 16);
    } else {
        current &= 0xFFFF0000u;
        current |= (uint32_t)value;
    }
    pci_outl(0xCF8, address);
    pci_outl(0xCFC, current);
}

bool pci_find_device(
    uint16_t vendor_id,
    uint16_t device_id,
    pci_device_t *result
)
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            for (uint8_t function = 0; function < 8; function++)
            {
                uint16_t found_vendor;
                uint16_t found_device;

                found_vendor = pci_config_read16(
                    (uint8_t)bus,
                    device,
                    function,
                    0x00
                );

                if (found_vendor == 0xFFFF)
                    continue;

                found_device = pci_config_read16(
                    (uint8_t)bus,
                    device,
                    function,
                    0x02
                );

                if (
                    found_vendor == vendor_id &&
                    found_device == device_id
                )
                {
                    if (result != 0)
                    {
                        result->bus = (uint8_t)bus;
                        result->device = device;
                        result->function = function;
                        result->vendor_id = found_vendor;
                        result->device_id = found_device;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

uint32_t pci_get_bar(
    const pci_device_t *device,
    uint8_t bar
)
{
    if (bar > 5)
        return 0;

    return pci_config_read32(
        device->bus,
        device->device,
        device->function,
        (uint8_t)(0x10 + (bar * 4))
    );
}