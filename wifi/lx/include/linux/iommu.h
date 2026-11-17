/*
 * linux/iommu.h for ath10k's snoc.c: a translation domain of its own for the firmware's memory, where the device tree
 * has a "wifi-firmware" node under the radio's (a firmware in the modem on some laptops). This phone's has none, so the
 * driver never comes to these; they are here for it to compile, and refuse.
 */
#ifndef LX_LINUX_IOMMU_H
#define LX_LINUX_IOMMU_H
#include <linux/types.h>
#include <linux/err.h>
#define IOMMU_READ      (1 << 0)
#define IOMMU_WRITE     (1 << 1)
struct device;
struct iommu_domain;
static inline struct iommu_domain *iommu_paging_domain_alloc(struct device *dev) { (void)dev; return ERR_PTR(-ENODEV); }
static inline void iommu_domain_free(struct iommu_domain *domain) { (void)domain; }
static inline int iommu_attach_device(struct iommu_domain *domain, struct device *dev) { (void)domain; (void)dev; return -ENODEV; }
static inline void iommu_detach_device(struct iommu_domain *domain, struct device *dev) { (void)domain; (void)dev; }
static inline int iommu_map(struct iommu_domain *domain, unsigned long iova, phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{ (void)domain; (void)iova; (void)paddr; (void)size; (void)prot; (void)gfp; return -ENODEV; }
static inline size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size) { (void)domain; (void)iova; (void)size; return 0; }
#endif
