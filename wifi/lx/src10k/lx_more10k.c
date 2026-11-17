/*
 * What ath10k asks of the 802.11 above it and of the system that ath11k does not, and the layer did not have.
 *
 * Transmit queues. ath10k is written for mac80211's per-station queues ("TXQ": the stack keeps the frames, the driver
 * pulls them when the firmware has room) as well as for frames handed to it one at a time (its "tx" operation, which
 * takes any frame). The station of lx_80211.c hands frames over one at a time, as it does with ath11k, so the queues
 * are always empty: asked for the next one, there is none, and stopping or waking them has nothing to act on.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/time.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>

#include <lx_os.h>
#include "lx.h"

/* the six bytes in front of an EtherType in an 802.11 data frame (RFC 1042) */
const uint8_t rfc1042_header[6] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

void
linuxkpi_ieee80211_txq_schedule_start(struct ieee80211_hw *hw, uint8_t ac)
{
}

struct ieee80211_txq *
linuxkpi_ieee80211_next_txq(struct ieee80211_hw *hw, uint8_t ac)
{
	return (NULL);
}

void
linuxkpi_ieee80211_schedule_txq(struct ieee80211_hw *hw, struct ieee80211_txq *txq, bool force)
{
}

struct sk_buff *
linuxkpi_ieee80211_tx_dequeue(struct ieee80211_hw *hw, struct ieee80211_txq *txq)
{
	return (NULL);
}

void
linuxkpi_ieee80211_txq_get_depth(struct ieee80211_txq *txq, unsigned long *frames, unsigned long *bytes)
{
	if (frames != NULL) {
		*frames = 0;
	}
	if (bytes != NULL) {
		*bytes = 0;
	}
}

/* (one of mac80211's queues stopped or woken: there is one sender here, lx_80211.c, and it is told) */
void
linuxkpi_ieee80211_stop_queue(struct ieee80211_hw *hw, int queue)
{
	linuxkpi_ieee80211_stop_queues(hw);
}

void
linuxkpi_ieee80211_wake_queue(struct ieee80211_hw *hw, int queue)
{
	linuxkpi_ieee80211_wake_queues(hw);
}

/* the traffic identifier of a QoS data frame: the low four bits of its QoS control, which follows the addresses */
uint8_t
linuxkpi_ieee80211_get_tid(struct ieee80211_hdr *hdr, bool nonqos_ok)
{
	const uint8_t *frame = (const uint8_t *)hdr;
	uint16_t control = (uint16_t)(frame[0] | frame[1] << 8);
	bool four = (control & 0x0300) == 0x0300;		/* to and from the distribution system: a fourth address */

	if ((control & 0x008c) != 0x0088) {			/* not a QoS data frame */
		return (0);
	}
	return (frame[four ? 30 : 24] & 0x0f);
}

/* (a rate as a number for reports nobody here reads) */
uint32_t
linuxkpi_cfg80211_calculate_bitrate(struct rate_info *rate)
{
	return (0);
}

struct linuxkpi_ieee80211_regdomain *
lkpi_get_linuxkpi_ieee80211_regdomain(size_t rules)
{
	return (kzalloc(sizeof(struct linuxkpi_ieee80211_regdomain) + rules * sizeof(struct ieee80211_reg_rule), GFP_KERNEL));
}

/*
 * The driver names the country of its board's data ("CN" on this phone, whatever country it was sold in) for Linux to
 * look its rules up in a database and narrow or widen the driver's own domain. There is no database here: the driver's
 * own domain stands (ath/regd.c's for a radio that roams: channels 1 to 11 at 2.4 GHz, and 5 GHz listened on but not
 * called on until an access point is heard there -- a station may still join one).
 */
int
lx_regulatory_hint(struct wiphy *wiphy, const uint8_t *alpha2)
{
	printf("lx: the driver names the country \"%c%c\" for its rules: there is no database of them, its own domain stands\n",
	    alpha2[0], alpha2[1]);
	return (0);
}

/* ---- the system's ------------------------------------------------------------------------------------------- */

void
DELAY(int usec)
{
	unsigned long long until = lx_os_nanoseconds() + (unsigned long long)(usec > 0 ? usec : 0) * 1000ull;

	while (lx_os_nanoseconds() < until) {
	}
}

void
nanouptime(struct timespec *ts)
{
	unsigned long long now = lx_os_nanoseconds();

	ts->tv_sec = (time_t)(now / 1000000000ull);
	ts->tv_nsec = (long)(now % 1000000000ull);
}

/* sleep that long (a time in FreeBSD's 32.32 seconds); never less than a millisecond */
int
pause_sbt(const char *wmesg, sbintime_t sbt, sbintime_t pr, int flags)
{
	unsigned long long ms = ((unsigned long long)sbt * 1000ull) >> 32;

	lx_pause_ms(ms != 0 ? (unsigned)ms : 1);
	return (0);
}

int
device_printf(device_t dev, const char *format, ...)
{
	char line[300];
	va_list list;

	va_start(list, format);
	vsnprintf(line, sizeof(line), format, list);
	va_end(list);
	printf("ath10k: %s", line);
	return (0);
}
