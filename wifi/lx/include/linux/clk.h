/*
 * linux/clk.h for ath10k's snoc.c: the radio's reference clocks, which its device tree node may name. This phone's node
 * (qcom,icnss@22800000) names none, and the phone's own driver has none to switch on: asked for, there are none, and
 * switching none on succeeds.
 */
#ifndef LX_LINUX_CLK_H
#define LX_LINUX_CLK_H
#include <linux/types.h>
struct device;
struct clk;
struct clk_bulk_data { const char *id; struct clk *clk; };
static inline int devm_clk_bulk_get_optional(struct device *dev, int count, struct clk_bulk_data *clks) { (void)dev; (void)count; (void)clks; return 0; }
static inline int clk_bulk_prepare_enable(int count, const struct clk_bulk_data *clks) { (void)count; (void)clks; return 0; }
static inline void clk_bulk_disable_unprepare(int count, const struct clk_bulk_data *clks) { (void)count; (void)clks; }
#endif
