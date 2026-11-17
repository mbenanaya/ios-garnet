/*
 * linux/pwrseq/consumer.h for ath10k's snoc.c: a power sequencer for radios that share an enable pin with Bluetooth
 * through one. This radio's supplies are switched by regulators alone (regulator/consumer.h): there is no sequencer,
 * and switching none on succeeds.
 */
#ifndef LX_LINUX_PWRSEQ_CONSUMER_H
#define LX_LINUX_PWRSEQ_CONSUMER_H
#include <linux/err.h>
struct device;
struct pwrseq_desc;
static inline struct pwrseq_desc *devm_pwrseq_get(struct device *dev, const char *target) { (void)dev; (void)target; return ERR_PTR(-EPROBE_DEFER); }
static inline int pwrseq_power_on(struct pwrseq_desc *desc) { (void)desc; return 0; }
static inline int pwrseq_power_off(struct pwrseq_desc *desc) { (void)desc; return 0; }
#endif
