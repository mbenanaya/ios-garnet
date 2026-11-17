/*
 * linux/soc/qcom/smem.h for ath10k's qmi.c, which writes the firmware's build name into the SoC's table of image
 * versions in the shared memory (ath10k_qmi_add_wlan_ver_smem()): a note for crash dumps, which nothing here reads.
 * The item is said not to be there, and the driver goes on without it (it says so in a line).
 */
#ifndef LX_QCOM_SMEM_H
#define LX_QCOM_SMEM_H

#include <linux/types.h>
#include <linux/err.h>

#define QCOM_SMEM_HOST_ANY      -1

static inline void *
qcom_smem_get(unsigned host, unsigned item, size_t *size)
{
	(void)host; (void)item; (void)size;
	return ERR_PTR(-ENOENT);
}

#endif
