/*
 * The platform under ath10k's own bus for the WCN3990 (snoc.c), on the POCO X6 5G (garnet).
 *
 * snoc.c is a Linux platform driver: it is given a device that a device tree describes -- registers, twelve
 * interrupts, supply rails, a region of memory for the firmware -- and does the rest itself: its exchange with the
 * firmware (qmi.c), the copy engines (ce.c), the data path. Here is that device, and what the driver asks of a
 * platform, said from this phone's device tree (qcom,icnss@22800000):
 *
 *   registers      "membase", 0x22800000, 8 MB, mapped by the kernel
 *   interrupts     twelve, one a copy engine: lines of the GIC, taken by the kernel, which calls the handler Linux
 *                  would run on the interrupt from a thread; the driver's receive poll (NAPI) runs on another
 *   rails          the group snoc.c names is switched as one by the kernel (hakios_wifi_radio_rails(): the five of the
 *                  device tree at the voltages of the phone's own driver)
 *   firmware       its own memory (the MSA): reserved-memory wlan_fw_region@82a00000, 3 MB
 *   clocks, a power sequencer, a "wifi-firmware" node, calibration in the tree: none (include/linux/clk.h and its kin)
 *
 * and the entry points the kernel knows this object by (bsd/sys/hakios.lx.h): lx_prepare(), lx_start(), lx_stop(),
 * lx_transmit(). The 802.11 above the driver is lx_80211.c, the same that ath11k is under on the phone it was written
 * for.
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <net/mac80211.h>

#include "core.h"
#include "debug.h"
#include "snoc.h"

#include <lx_os.h>
#include "lx.h"

#define RADIO_REGISTERS		0x22800000ull	/* dt: "membase" of qcom,icnss@22800000 */
#define RADIO_REGISTERS_SIZE	0x00800000u
#define RADIO_MSA		0x82a00000ull	/* dt: reserved-memory wlan_fw_region@82a00000 */
#define RADIO_MSA_SIZE		0x00300000u
#define RADIO_IRQS		12		/* dt: the node's interrupts, one a copy engine */
#define IRQ_BASE		1		/* the numbers platform_get_irq() gives: 1 to 12 */

int lx_os_radio_rails(int on);		/* (bsd/sys/hakios.lx.h: the radio's supply rails, all of them) */
int lx_os_irq_attach_line(unsigned vector, unsigned spi, void (*function)(void *), void *argument);
void lx_os_irq_enable(unsigned vector);
void lx_os_irq_disable(unsigned vector);
void lx_os_irq_detach_all(void);
int lx_platform_driver_init(void);	/* (snoc.c: module_platform_driver(), include/linux/platform_device.h) */

static struct platform_driver *snoc;	/* the driver, once it has registered */
static struct platform_device radio;
static struct device_node radio_node;
static struct ath10k *volatile driver;	/* the driver's state, while it is up: between lx_start() and lx_stop() */

/* ---- what the driver says ---------------------------------------------------------------------------------- */

static void
driver_says(bool always, const char *kind, const char *format, va_list list)
{
	char line[400];
	size_t length;

	vsnprintf(line, sizeof(line), format, list);
	length = strlen(line);
	if (always) {
		lx_tell("ath10k%s: %s%s", kind, line, length != 0 && line[length - 1] == '\n' ? "" : "\n");
	} else {
		printf("ath10k%s: %s%s", kind, line, length != 0 && line[length - 1] == '\n' ? "" : "\n");
	}
}

void
ath10k_info(struct ath10k *ar, const char *format, ...)
{
	va_list list;

	va_start(list, format);
	driver_says(false, "", format, list);
	va_end(list);
}

void
ath10k_warn(struct ath10k *ar, const char *format, ...)
{
	va_list list;

	va_start(list, format);
	driver_says(true, " warns", format, list);
	va_end(list);
}

void
ath10k_err(struct ath10k *ar, const char *format, ...)
{
	va_list list;

	va_start(list, format);
	driver_says(true, " FAILS", format, list);
	va_end(list);
}

void
__ath10k_dbg(struct ath10k *ar, enum ath10k_debug_mask mask, const char *format, ...)
{
	va_list list;

	if ((ath10k_debug_mask & mask) == 0) {
		return;
	}
	va_start(list, format);
	driver_says(false, "", format, list);
	va_end(list);
}

void
ath10k_dbg_dump(struct ath10k *ar, enum ath10k_debug_mask mask, const char *msg, const char *prefix, const void *buf, size_t len)
{
}

/* ---- the device: registers, properties, memory --------------------------------------------------------------- */

int
platform_driver_register(struct platform_driver *driver_)
{
	snoc = driver_;
	return (0);
}

void
platform_driver_unregister(struct platform_driver *driver_)
{
	snoc = NULL;
}

const void *
device_get_match_data(const struct device *dev)
{
	return (snoc != NULL && snoc->driver.of_match_table != NULL ? snoc->driver.of_match_table[0].data : NULL);
}

struct resource *
platform_get_resource_byname(struct platform_device *pdev, unsigned int type, const char *name)
{
	static struct resource registers;

	if (type != IORESOURCE_MEM || strcmp(name, "membase") != 0) {
		return (NULL);
	}
	registers.start = RADIO_REGISTERS;
	registers.end = RADIO_REGISTERS + RADIO_REGISTERS_SIZE - 1;
	registers.flags = IORESOURCE_MEM;
	registers.name = "membase";
	return (&registers);
}

/* (calibration of the crystal that a tree may carry, "qcom,xo-cal-data": this one's does not) */
int
device_property_read_u32(struct device *dev, const char *name, u32 *out)
{
	return (-EINVAL);
}

const struct property *
of_find_property(const struct device_node *np, const char *name, int *lenp)
{
	return (NULL);
}

const void *
of_get_property(const struct device_node *np, const char *name, int *lenp)
{
	return (NULL);
}

struct device_node *
of_get_child_by_name(const struct device_node *np, const char *name)
{
	return (NULL);
}

int
of_property_read_string(const struct device_node *np, const char *name, const char **out)
{
	return (-EINVAL);
}

bool
of_machine_is_compatible(const char *compatible)
{
	return (false);
}

/* the node's memory-region: the firmware's own */
int
of_reserved_mem_region_to_resource(const struct device_node *np, unsigned int index, struct resource *res)
{
	if (index != 0) {
		return (-ENODEV);
	}
	memset(res, 0, sizeof(*res));
	res->start = RADIO_MSA;
	res->end = RADIO_MSA + RADIO_MSA_SIZE - 1;
	res->flags = IORESOURCE_MEM;
	return (0);
}

void *
devm_memremap(struct device *dev, resource_size_t offset, size_t size, unsigned long flags)
{
	void *mapped = lx_os_map_registers(offset, size);

	return (mapped != NULL ? mapped : ERR_PTR(-ENOMEM));
}

int
linuxkpi_device_get_mac_address(struct device *dev, char *address)
{
	return (lx_os_radio_address((unsigned char *)address) ? 0 : -ENOENT);
}

/* ---- memory that goes with the device (never given back before the driver is: the device is there for good) ---- */

void *
lkpi_devres_alloc(void (*release)(struct device *, void *), size_t size, gfp_t gfp)
{
	return (kzalloc(size, gfp));
}

void
lkpi_devres_add(struct device *dev, void *p)
{
}

void
lkpi_devm_kmalloc_release(struct device *dev, void *p)
{
}

int
linux_dma_tag_init(struct device *dev, u64 mask)
{
	return (0);
}

int
linux_dma_tag_init_coherent(struct device *dev, u64 mask)
{
	return (0);
}

void *
linuxkpi_dmam_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t flag)
{
	return (dma_alloc_coherent(dev, size, handle, flag));
}

/* ---- the rails ----------------------------------------------------------------------------------------------- */

int
devm_regulator_bulk_get(struct device *dev, int count, struct regulator_bulk_data *consumers)
{
	return (0);
}

int
regulator_bulk_get(struct device *dev, int count, struct regulator_bulk_data *consumers)
{
	return (0);
}

int
regulator_bulk_enable(int count, struct regulator_bulk_data *consumers)
{
	return (lx_os_radio_rails(1) ? 0 : -EIO);
}

int
regulator_bulk_disable(int count, struct regulator_bulk_data *consumers)
{
	lx_os_radio_rails(0);
	return (0);
}

/* ---- the interrupts, and the receive poll ------------------------------------------------------------------------ */

/*
 * The radio sleeps when it has nothing to do, and its registers cannot be read then: a read ends the kernel (16
 * November: fifteen seconds after the radio was up and idle). Linux reads them only when the radio has raised an
 * interrupt -- it is awake then -- so the handlers are called for a real interrupt and never to look. The twelve lines
 * are shared peripheral interrupts of the GIC, level-triggered (dt: "interrupts" of qcom,icnss@22800000, one a copy
 * engine); the kernel masks a line when it arrives, calls the handler on a thread, and lets the line through again when
 * the handler has silenced its cause (ath10k_snoc_per_engine_handler(): the copy engine's own interrupt enable).
 *
 * The handler asks for the driver's receive poll (NAPI), which runs on a thread of its own as soon as it is asked for.
 */
static const unsigned short radio_spi[RADIO_IRQS] = { 0x185, 0x19d, 0x1a0, 0x1a1, 0x1b2, 0x1b3, 0x1b6, 0x1b7, 0x1b8, 0x1c3,
						      0x1c5, 0x1c6 };
static struct radio_irq {
	irq_handler_t handler;
	void *argument;
	bool attached;
} irqs[RADIO_IRQS];
static struct {
	struct napi_struct *napi;
	int (*poll)(struct napi_struct *, int);
	bool enabled, scheduled, running;	/* (under the layer's lock) */
} napi_one;
static bool polling, polls_up;			/* the poll's thread: wanted, there */
static unsigned long long polls, interrupts;

int
platform_get_irq(struct platform_device *pdev, unsigned int index)
{
	return (index < RADIO_IRQS ? (int)(IRQ_BASE + index) : -ENXIO);
}

static void
radio_interrupts(void *argument)
{
	struct radio_irq *irq = argument;

	interrupts++;
	if (irq->handler != NULL) {
		irq->handler((int)(IRQ_BASE + (irq - irqs)), irq->argument);
	}
}

static void
radio_polls(void *argument)
{
	lx_os_lock();
	polls_up = true;
	lx_os_wakeup(&polls_up);
	while (polling) {
		int (*poll)(struct napi_struct *, int) = napi_one.poll;
		struct napi_struct *napi = napi_one.napi;
		int done;

		if (poll == NULL || !napi_one.enabled || !napi_one.scheduled) {
			lx_os_sleep(&napi_one, 200000000ull);
			continue;
		}
		napi_one.scheduled = false;
		napi_one.running = true;
		lx_os_unlock();
		done = poll(napi, 64);
		lx_os_lock();
		napi_one.running = false;
		if (done >= 64) {
			napi_one.scheduled = true;	/* (more is waiting: again at once) */
		}
		polls++;
		lx_os_wakeup(&napi_one.running);
	}
	polls_up = false;
	lx_os_wakeup(&polls_up);
	lx_os_unlock();
}

int
lkpi_request_irq(struct device *dev, unsigned int irq, irq_handler_t handler, irq_handler_t thread_handler, unsigned long flags,
    const char *name, void *argument)
{
	struct radio_irq *line;
	int error;

	if (irq < IRQ_BASE || irq >= IRQ_BASE + RADIO_IRQS) {
		return (-EINVAL);
	}
	line = &irqs[irq - IRQ_BASE];
	line->argument = argument;
	line->handler = handler;
	if (!line->attached) {
		if ((error = lx_os_irq_attach_line(irq - IRQ_BASE, radio_spi[irq - IRQ_BASE], radio_interrupts, line)) != 0) {
			lx_tell("lx: interrupt %u of the radio (SPI %u) is not to be had: %d\n", irq - IRQ_BASE, radio_spi[irq - IRQ_BASE],
			    error);
			line->handler = NULL;
			return (-error);
		}
		line->attached = true;
	}
	if ((flags & IRQF_NO_AUTOEN) == 0) {
		lx_os_irq_enable(irq - IRQ_BASE);
	}
	return (0);
}

void
lkpi_free_irq(unsigned int irq, void *argument)
{
	if (irq >= IRQ_BASE && irq < IRQ_BASE + RADIO_IRQS && irqs[irq - IRQ_BASE].attached) {
		lx_os_irq_disable(irq - IRQ_BASE);
		irqs[irq - IRQ_BASE].handler = NULL;
	}
}

int
lkpi_enable_irq(unsigned int irq)
{
	if (irq >= IRQ_BASE && irq < IRQ_BASE + RADIO_IRQS && irqs[irq - IRQ_BASE].attached) {
		lx_os_irq_enable(irq - IRQ_BASE);
	}
	return (0);
}

void
lkpi_disable_irq(unsigned int irq)
{
	if (irq >= IRQ_BASE && irq < IRQ_BASE + RADIO_IRQS && irqs[irq - IRQ_BASE].attached) {
		lx_os_irq_disable(irq - IRQ_BASE);
	}
}

int
enable_irq_wake(unsigned int irq)
{
	return (0);
}

int
disable_irq_wake(unsigned int irq)
{
	return (0);
}

struct net_device *
linuxkpi_alloc_netdev(size_t length, const char *name, uint32_t flags, void (*setup)(struct net_device *))
{
	return (kzalloc(sizeof(struct net_device) + length, GFP_KERNEL));
}

void
linuxkpi_free_netdev(struct net_device *dev)
{
	kfree(dev);
}

void
linuxkpi_netif_napi_add(struct net_device *dev, struct napi_struct *napi, int (*poll)(struct napi_struct *, int))
{
	lx_os_lock();
	napi_one.napi = napi;
	napi_one.scheduled = false;
	napi_one.enabled = false;
	napi_one.poll = poll;
	if (!polling) {
		polling = true;
		if (lx_os_thread("lx radio poll", radio_polls, NULL) != 0) {
			polling = false;
			lx_tell("lx: no thread for the radio's receive poll\n");
		}
	}
	lx_os_unlock();
}

void
linuxkpi_napi_synchronize(struct napi_struct *napi)
{
	lx_os_lock();
	while (napi_one.running) {
		lx_os_sleep(&napi_one.running, 10000000ull);
	}
	lx_os_unlock();
}

void
linuxkpi_netif_napi_del(struct napi_struct *napi)
{
	lx_os_lock();
	napi_one.enabled = false;
	napi_one.poll = NULL;
	lx_os_unlock();
	linuxkpi_napi_synchronize(napi);
}

void
linuxkpi_napi_enable(struct napi_struct *napi)
{
	lx_os_lock();
	napi_one.enabled = true;
	lx_os_wakeup(&napi_one);
	lx_os_unlock();
}

void
linuxkpi_napi_disable(struct napi_struct *napi)
{
	lx_os_lock();
	napi_one.enabled = false;
	lx_os_unlock();
	linuxkpi_napi_synchronize(napi);
}

bool
linuxkpi_napi_schedule(struct napi_struct *napi)
{
	lx_os_lock();
	napi_one.scheduled = true;
	lx_os_wakeup(&napi_one);
	lx_os_unlock();
	return (true);
}

bool
linuxkpi_napi_complete(struct napi_struct *napi)
{
	return (true);
}

struct sk_buff *
linuxkpi_skb_copy(const struct sk_buff *skb, gfp_t gfp)
{
	unsigned int headroom = skb_headroom(skb);
	struct sk_buff *copy = dev_alloc_skb(headroom + skb->len);

	if (copy == NULL) {
		return (NULL);
	}
	skb_reserve(copy, headroom);
	memcpy(skb_put(copy, skb->len), skb->data, skb->len);
	memcpy(copy->cb, skb->cb, sizeof(copy->cb));
	copy->priority = skb->priority;
	return (copy);
}

/* ---- the kernel's entry points --------------------------------------------------------------------------------- */

int
lx_prepare(void)
{
	return (lx_base_init() != 0 || lx_pool_init() != 0 ? ENOMEM : 0);
}

int
lx_start(void)
{
	unsigned long long began;
	struct ath10k *ar;
	bool up;
	int ret;

	if (lx_prepare() != 0) {
		lx_tell("lx: no thread, or no memory the radio can be given\n");
		return (ENOMEM);
	}
	ath10k_debug_mask = lx_os_option("wifi_quiet") ? 0 :
	    ATH10K_DBG_QMI | ATH10K_DBG_BOOT | ATH10K_DBG_WMI | ATH10K_DBG_REGULATORY | ATH10K_DBG_MAC |
	    (lx_os_option("wifi_loud") >= 2 ? ATH10K_DBG_SNOC : 0);	/* (2: every buffer through a copy engine) */
	radio.name = "wifi";
	radio.dev.of_node = &radio_node;
	radio.dev.kobj.name = "wifi";
	radio.dev.dma_priv = &radio;		/* (LinuxKPI's sign that the device can be given memory at all: lx_base.c's pool) */
	if ((ret = lx_platform_driver_init()) != 0 || snoc == NULL || snoc->probe == NULL) {
		lx_tell("lx: the driver does not register: %d\n", ret);
		return (EIO);
	}
	/*
	 * The phone's own driver switches the rails on when the firmware's service appears, before it says a word to it
	 * (icnss_driver_event_server_arrive(): icnss_hw_power_on()); ath10k does so only once the firmware has said
	 * "ready". On this phone the firmware was heard to say that with the rails on, so they are, from here.
	 */
	if (!lx_os_radio_rails(1)) {
		lx_tell("lx: the radio's rails do not come on\n");
		return (EIO);
	}
	printf("lx: the driver (\"%s\") has registered; its probe of the radio begins\n", snoc->driver.name);
	if ((ret = snoc->probe(&radio)) != 0) {
		lx_tell("lx: the driver's probe fails: %d\n", ret);
		return (-ret);
	}
	ar = platform_get_drvdata(&radio);

	/*
	 * The rest happens on the driver's own threads: its exchange with the firmware, and when the firmware has said
	 * "ready", core.c's registration (the radio powered and in mission mode, the copy engines, HTC, WMI, mac.c with
	 * what stands in for mac80211). It has got through when core.c says it is registered.
	 */
	began = lx_os_nanoseconds();
	while (!test_bit(ATH10K_FLAG_CORE_REGISTERED, &ar->dev_flags) && lx_os_nanoseconds() - began < 60000000000ull) {
		lx_pause_ms(10);
	}
	up = test_bit(ATH10K_FLAG_CORE_REGISTERED, &ar->dev_flags);
	lx_tell("lx: %llu ms after its probe the driver %s; state %d, flags %#lx; the radio has interrupted %llu times, %llu polls\n",
	    (lx_os_nanoseconds() - began) / 1000000ull, up ? "IS UP: registered with the 802.11 above it" :
	    "HAS NOT REGISTERED", (int)ar->state, ar->dev_flags, interrupts, polls);
	if (!up) {
		return (ETIMEDOUT);
	}
	driver = ar;
	if (lx_os_option("wifi_station")) {
		if ((ret = lx_station_offer()) == 0) {
			return (0);
		}
	} else if (lx_os_option("wifi_join")) {
		if ((ret = lx_station_join()) == 0) {
			if (lx_os_option("wifi_hold")) {
				lx_pause_ms(1000 * lx_os_option("wifi_hold"));
				lx_station_say();
			}
			return (0);
		}
	} else {
		ret = lx_os_option("wifi_scan") ? lx_station_scan() : 0;
	}
	lx_stop();
	return (ret);
}

void
lx_stop(void)
{
	if (driver == NULL) {
		return;
	}
	driver = NULL;
	lx_station_withdraw();
	lx_station_leave();
	if (snoc != NULL && snoc->remove != NULL) {
		snoc->remove(&radio);
	}
	lx_os_irq_detach_all();
	for (unsigned i = 0; i < RADIO_IRQS; i++) {
		irqs[i].attached = false;
	}
	lx_os_lock();
	polling = false;
	lx_os_wakeup(&napi_one);
	while (polls_up) {
		lx_os_sleep(&polls_up, 10000000ull);
	}
	lx_os_unlock();
	lx_tell("lx: the driver is down again; the radio interrupted %llu times, %llu polls\n", interrupts, polls);
}

int
lx_transmit(const unsigned char *frame, unsigned long length)
{
	return (driver != NULL ? lx_station_send(frame, length) : ENETDOWN);
}
