/*
 * linux/qrtr.h for ath10k's qmi.c: the address of a QRTR port. The kernel's QMI helpers have it already
 * (bsd/sys/hakios.qmi.h, through linux/soc/qcom/qmi.h), and LinuxKPI's header would define it a second time.
 */
#ifndef LX_LINUX_QRTR_H
#define LX_LINUX_QRTR_H
#include <linux/soc/qcom/qmi.h>
#endif
