/*
 * linux/platform_device.h for ath10k's snoc.c, which is a platform driver: a device the device tree describes, with
 * registers and interrupts of its own. LinuxKPI's header has the device but a driver without "probe", and registering
 * one fails. Here the driver has what Linux's has, and registering it hands it to the glue (src10k/lx_snoc.c), which
 * makes the one device there is -- the phone's radio -- and probes it when the kernel says so.
 * (For anything but ath10k: LinuxKPI's header, as before.)
 */
#ifndef CONFIG_ATH10K_SNOC
#include_next <linux/platform_device.h>
#else
#ifndef LX_LINUX_PLATFORM_DEVICE_H
#define LX_LINUX_PLATFORM_DEVICE_H

#include <linux/kernel.h>
#include <linux/device.h>

struct platform_device {
	const char *name;
	int id;
	bool id_auto;
	struct device dev;
};

struct platform_driver {
	int (*probe)(struct platform_device *);
	void (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	struct device_driver driver;
};

#define dev_is_platform(dev)	(true)
#define to_platform_device(d)	container_of((d), struct platform_device, dev)

static inline void platform_set_drvdata(struct platform_device *pdev, void *data) { dev_set_drvdata(&pdev->dev, data); }
static inline void *platform_get_drvdata(const struct platform_device *pdev) { return dev_get_drvdata(&pdev->dev); }
int platform_driver_register(struct platform_driver *driver);
void platform_driver_unregister(struct platform_driver *driver);
static inline void platform_device_unregister(struct platform_device *pdev) { (void)pdev; }

/* a module that is one platform driver: registered when the object is prepared (lx_snoc.c calls it) */
#undef module_platform_driver
#define module_platform_driver(drv) \
	int lx_platform_driver_init(void); \
	int lx_platform_driver_init(void) { return platform_driver_register(&(drv)); }

#endif
#endif
