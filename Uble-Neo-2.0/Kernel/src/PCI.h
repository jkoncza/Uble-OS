#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
} pci_device_t;

void pci_init(void);


bool pci_has_devices(void);

uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
);

uint16_t pci_config_read16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
);

uint8_t pci_config_read8(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
);

void pci_config_write16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
);

bool pci_find_device(
    uint16_t vendor_id,
    uint16_t device_id,
    pci_device_t *result
);

uint32_t pci_get_bar(
    const pci_device_t *device,
    uint8_t bar
);

#endif