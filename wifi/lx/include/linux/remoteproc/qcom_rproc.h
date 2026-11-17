/*
 * linux/remoteproc/qcom_rproc.h for ath10k's snoc.c: to be told when the processor the firmware runs on restarts. The
 * Wi-Fi DSP is started and stopped by the kernel around the driver (drivers/wifi/hakios.wifi.c) and is not restarted
 * under it: the registration is taken and nothing is ever told.
 */
#ifndef LX_LINUX_QCOM_RPROC_H
#define LX_LINUX_QCOM_RPROC_H
#include <linux/notifier.h>
enum qcom_ssr_notify_type { QCOM_SSR_BEFORE_POWERUP, QCOM_SSR_AFTER_POWERUP, QCOM_SSR_BEFORE_SHUTDOWN, QCOM_SSR_AFTER_SHUTDOWN };
struct qcom_ssr_notify_data { const char *name; bool crashed; };
static inline void *qcom_register_ssr_notifier(const char *name, struct notifier_block *nb) { (void)name; return nb; }
static inline int qcom_unregister_ssr_notifier(void *notify, struct notifier_block *nb) { (void)notify; (void)nb; return 0; }
#endif
