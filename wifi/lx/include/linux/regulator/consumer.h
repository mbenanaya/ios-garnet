/*
 * linux/regulator/consumer.h for ath10k's snoc.c: the radio's supply rails, taken and switched as a group. The rails are
 * the kernel's to switch (drivers/wifi/hakios.wifi.c: hakios_wifi_radio_rails(), the five of the phone's device tree at
 * the voltages of the phone's own driver); the group's names are taken as they come, and on and off are passed on
 * (src10k/lx_snoc.c).
 */
#ifndef LX_LINUX_REGULATOR_CONSUMER_H
#define LX_LINUX_REGULATOR_CONSUMER_H
#include <linux/types.h>
struct device;
struct regulator;
struct regulator_bulk_data { const char *supply; struct regulator *consumer; int init_load_uA; int ret; };
int devm_regulator_bulk_get(struct device *dev, int count, struct regulator_bulk_data *consumers);
int regulator_bulk_get(struct device *dev, int count, struct regulator_bulk_data *consumers);
int regulator_bulk_enable(int count, struct regulator_bulk_data *consumers);
int regulator_bulk_disable(int count, struct regulator_bulk_data *consumers);
#endif
