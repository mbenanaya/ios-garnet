/*
 * What ath10k uses of Linux beyond what ath11k does (lx_more.h) and LinuxKPI's headers (FreeBSD 15.1) do not declare.
 * Small things each. Included before every file of ath10k, after lx_prelude.h (tools/mk-ath10k.sh).
 */
#ifndef _LX_MORE10K_H_
#define _LX_MORE10K_H_

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/cleanup.h>

/* guard(spinlock_bh)(&lock): LinuxKPI has the irqsave one only */
DEFINE_LOCK_GUARD_1(spinlock_bh,
    spinlock_t,
    spin_lock_bh(_T->lock),
    spin_unlock_bh(_T->lock))

/* --- the device tree: byte properties (calibration data and variants a board's tree may carry: this one's has none) */
struct device_node;
static inline int
of_property_read_u8_array(const struct device_node *np, const char *name, u8 *out, size_t count)
{
	(void)np; (void)name; (void)out; (void)count;
	return -EINVAL;
}

static inline int
of_property_read_u8(const struct device_node *np, const char *name, u8 *out)
{
	(void)np; (void)name; (void)out;
	return -EINVAL;
}

static inline int
of_property_read_string_index(const struct device_node *np, const char *name, int index, const char **out)
{
	(void)np; (void)name; (void)index; (void)out;
	return -EINVAL;
}

/* --- non-volatile memory cells (calibration kept in a PC's EEPROM): there are none */
struct nvmem_cell;
static inline struct nvmem_cell *
devm_nvmem_cell_get(struct device *dev, const char *name)
{
	(void)dev; (void)name;
	return ERR_PTR(-ENOENT);
}

static inline void *
nvmem_cell_read(struct nvmem_cell *cell, size_t *length)
{
	(void)cell; (void)length;
	return ERR_PTR(-ENOENT);
}

/* --- the platform device the radio is (snoc.c): its registers, its interrupts, its properties. What they are on this
 * phone is the glue's to say (src10k/lx_snoc.c), from the phone's device tree */
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#ifndef IRQF_NO_AUTOEN
#define IRQF_NO_AUTOEN		0x00080000	/* request_irq() leaves the interrupt disabled until enable_irq() */

/*
 * --- the regulatory domain. LinuxKPI's wiphy_apply_custom_regulatory() does nothing and its regulatory_hint() refuses
 * once a domain is set; ath/regd.c and mac.c get the layer's own (lx_80211.c, lx_more10k.c). The header is taken first
 * so that its definitions keep their names.
 */
#include <net/cfg80211.h>
void lx_custom_regulatory(struct wiphy *wiphy, const struct linuxkpi_ieee80211_regdomain *regd);
int lx_regulatory_hint(struct wiphy *wiphy, const uint8_t *alpha2);
#define wiphy_apply_custom_regulatory(wiphy, regd)	lx_custom_regulatory(wiphy, regd)
#define regulatory_hint(wiphy, alpha2)			lx_regulatory_hint(wiphy, alpha2)

#endif
struct resource *platform_get_resource_byname(struct platform_device *pdev, unsigned int type, const char *name);
int platform_get_irq(struct platform_device *pdev, unsigned int index);
int device_property_read_u32(struct device *dev, const char *name, u32 *out);
/* the receive poll on a thread of its own: it is on whatever calls it here */
static inline void netif_threaded_enable(struct net_device *dev) { (void)dev; }

/* the firmware's own memory, mapped for the driver to clear it (snoc.c: ath10k_setup_msa_resources()) */
#ifndef MEMREMAP_WT
#define MEMREMAP_WT		(1 << 1)
#endif
void *devm_memremap(struct device *dev, resource_size_t offset, size_t size, unsigned long flags);
/* which of the driver's variants this device is: the one entry of its table (snoc.c: ath10k_snoc_dt_match) */
const void *device_get_match_data(const struct device *dev);
/* a device of its own for the firmware under a "wifi-firmware" node: this phone has no such node (iommu.h) */
struct fwnode_handle;
struct platform_device_info {
	struct device *parent;
	struct fwnode_handle *fwnode;
	const char *name;
	u64 dma_mask;
};
static inline struct platform_device *platform_device_register_full(const struct platform_device_info *info) { (void)info; return ERR_PTR(-ENODEV); }
static inline int of_dma_configure(struct device *dev, struct device_node *np, bool force) { (void)dev; (void)np; (void)force; return -ENODEV; }

#endif
