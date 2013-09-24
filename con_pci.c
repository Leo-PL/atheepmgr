/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pciaccess.h>

#include "edump.h"
#include "con_pci.h"

struct pci_priv {
	struct pci_device *pdev;
	pciaddr_t base_addr;
	pciaddr_t size;
	void *io_map;
};

static int is_supported_chipset(struct pci_device *pdev)
{
	if (pdev->vendor_id != ATHEROS_VENDOR_ID)
		return 0;

	if ((pdev->device_id != AR5416_DEVID_PCI) &&
	    (pdev->device_id != AR5416_DEVID_PCIE) &&
	    (pdev->device_id != AR9160_DEVID_PCI) &&
	    (pdev->device_id != AR9280_DEVID_PCI) &&
	    (pdev->device_id != AR9280_DEVID_PCIE) &&
	    (pdev->device_id != AR9285_DEVID_PCIE) &&
	    (pdev->device_id != AR9287_DEVID_PCI) &&
	    (pdev->device_id != AR9287_DEVID_PCIE) &&
	    (pdev->device_id != AR9300_DEVID_PCIE) &&
	    (pdev->device_id != AR9485_DEVID_PCIE) &&
	    (pdev->device_id != AR9580_DEVID_PCIE) &&
	    (pdev->device_id != AR9462_DEVID_PCIE) &&
	    (pdev->device_id != AR9565_DEVID_PCIE) &&
	    (pdev->device_id != AR1111_DEVID_PCIE)) {
		fprintf(stderr, "Device ID: 0x%x not supported\n", pdev->device_id);
		return 0;
	}

	printf("Found Device ID: 0x%04x\n", pdev->device_id);
	return 1;
}

static int pci_device_init(struct edump *edump, struct pci_device *pdev)
{
	struct pci_priv *ppd = edump->con_priv;
	int err;

	if (!pdev->regions[0].base_addr) {
		fprintf(stderr, "Invalid base address\n");
		return EINVAL;
	}

	ppd->pdev = pdev;
	ppd->base_addr = pdev->regions[0].base_addr;
	ppd->size = pdev->regions[0].size;
	pdev->user_data = (intptr_t)edump;

	err = pci_device_map_range(pdev, ppd->base_addr, ppd->size, 0,
				   &ppd->io_map);
	if (err) {
		fprintf(stderr, "%s\n", strerror(err));
		return err;
	}

	printf("Mapped IO region at: %p\n", ppd->io_map);

	return 0;
}

static void pci_device_cleanup(struct edump *edump)
{
	struct pci_priv *ppd = edump->con_priv;
	int err;

	printf("Freeing Mapped IO region at: %p\n", ppd->io_map);

	err = pci_device_unmap_range(ppd->pdev, ppd->io_map, ppd->size);
	if (err)
		fprintf(stderr, "%s\n", strerror(err));
}

static uint32_t pci_reg_read(struct edump *edump, uint32_t reg)
{
	struct pci_priv *ppd = edump->con_priv;

#if __BYTE_ORDER == __BIG_ENDIAN
	return bswap_32(*((volatile uint32_t *)(ppd->io_map + reg)));
#else
	return *((volatile uint32_t *)(ppd->io_map + reg));
#endif
}

static int pci_init(struct edump *edump, const char *arg_str)
{
	struct pci_slot_match slot[2];
	struct pci_device_iterator *iter;
	struct pci_device *pdev;
	int ret;

	memset(slot, 0x00, sizeof(slot));

	ret = sscanf(arg_str, "%x:%x:%x", &slot[0].domain, &slot[0].bus, &slot[0].dev);
	if (ret != 3) {
		fprintf(stderr, "Invalid PCI slot specification: %s\n",
			arg_str);
		return -EINVAL;
	}
	slot[0].func = PCI_MATCH_ANY;

	ret = pci_system_init();
	if (ret) {
		fprintf(stderr, "%s\n", strerror(ret));
		goto err;
	}

	iter = pci_slot_match_iterator_create(slot);
	if (iter == NULL) {
		fprintf(stderr, "Iter creation failed\n");
		ret = EINVAL;
		goto err;
	}

	pdev = pci_device_next(iter);
	pci_iterator_destroy(iter);

	if (NULL == pdev) {
		fprintf(stderr, "No suitable card found\n");
		ret = ENODEV;
		goto err;
	}

	ret = pci_device_probe(pdev);
	if (ret) {
		fprintf(stderr, "%s\n", strerror(ret));
		goto err;
	}

	if (!is_supported_chipset(pdev)) {
		ret = ENOTSUP;
		goto err;
	}

	ret = pci_device_init(edump, pdev);
	if (ret)
		goto err;

	return 0;

err:
	pci_system_cleanup();

	return -ret;
}

static void pci_clean(struct edump *edump)
{
	pci_device_cleanup(edump);
	pci_system_cleanup();
}

const struct connector con_pci = {
	.name = "PCI",
	.priv_data_sz = sizeof(struct pci_priv),
	.init = pci_init,
	.clean = pci_clean,
	.reg_read = pci_reg_read,
};
