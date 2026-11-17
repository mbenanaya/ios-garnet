/*
 * linux/firmware/qcom/qcom_scm.h, as much of it as ath10k's qmi.c uses: handing the firmware's own memory (the MSA)
 * over to the modem through TrustZone. That is for a radio whose firmware runs in the modem. On the phone this is
 * built for (garnet) the firmware runs on the Wi-Fi DSP, the MSA is a fixed region of the device tree, and the phone's
 * own driver (icnss2) asks TrustZone for nothing about it; the firmware said "MSA ready" and "firmware ready" with
 * nothing handed over (16 November). So the request is answered as done, and nothing is asked.
 */
#ifndef LX_QCOM_SCM_H
#define LX_QCOM_SCM_H

#include <linux/types.h>

#define QCOM_SCM_VMID_HLOS              0x3
#define QCOM_SCM_VMID_MSS_MSA           0xf
#define QCOM_SCM_VMID_WLAN              0x18
#define QCOM_SCM_VMID_WLAN_CE           0x19
#define QCOM_SCM_PERM_READ              0x4
#define QCOM_SCM_PERM_WRITE             0x2
#define QCOM_SCM_PERM_EXEC              0x1
#define QCOM_SCM_PERM_RW                (QCOM_SCM_PERM_READ | QCOM_SCM_PERM_WRITE)
#define QCOM_SCM_PERM_RWX               (QCOM_SCM_PERM_RW | QCOM_SCM_PERM_EXEC)

struct qcom_scm_vmperm {
	int vmid;
	int perm;
};

static inline int
qcom_scm_assign_mem(phys_addr_t mem_addr, size_t mem_sz, u64 *src, const struct qcom_scm_vmperm *newvm, unsigned int dest_cnt)
{
	(void)mem_addr; (void)mem_sz; (void)newvm; (void)dest_cnt;
	if (src != NULL && dest_cnt != 0) {
		u64 owners = 0;
		for (unsigned int i = 0; i < dest_cnt; i++) {
			owners |= 1ull << newvm[i].vmid;
		}
		*src = owners;          /* (as the call leaves it: the set of new owners, for the next call's "from") */
	}
	return 0;
}

#endif
