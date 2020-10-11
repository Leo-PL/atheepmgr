/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013,2017 Sergey Ryazanov <ryazanov.s.a@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "atheepmgr.h"
#include "eep_common.h"

const char * const sDeviceType[] = {
	"UNKNOWN [0] ",
	"Cardbus     ",
	"PCI         ",
	"MiniPCI     ",
	"Access Point",
	"PCIExpress  ",
	"UNKNOWN [6] ",
	"UNKNOWN [7] ",
};

const char * const sAccessType[] = {
	"ReadWrite", "WriteOnly", "ReadOnly", "NoAccess"
};

const char * const eep_rates_cck[AR5416_NUM_TARGET_POWER_RATES_LEG] = {
	"1 mbps", "2 mbps", "5.5 mbps", "11 mbps"
};

const char * const eep_rates_ofdm[AR5416_NUM_TARGET_POWER_RATES_LEG] = {
	"6-24 mbps", "36 mbps", "48 mbps", "54 mbps"
};

const char * const eep_rates_ht[AR5416_NUM_TARGET_POWER_RATES_HT] = {
	"MCS 0/8", "MCS 1/9", "MCS 2/10", "MCS 3/11",
	"MCS 4/12", "MCS 5/13", "MCS 6/14", "MCS 7/15"
};

const char * const eep_ctldomains[] = {
	 "Unknown (0)",          "FCC",  "Unknown (2)",         "ETSI",
	         "MKK",  "Unknown (5)",  "Unknown (6)",  "Unknown (7)",
	 "Unknown (8)",  "Unknown (9)", "Unknown (10)", "Unknown (11)",
	"Unknown (12)", "Unknown (13)",    "SD no ctl",       "No ctl"
};

const char * const eep_ctlmodes[] = {
	   "5GHz OFDM",     "2GHz CCK",    "2GHz OFDM",   "5GHz Turbo",
	  "2GHz Turbo",    "2GHz HT20",    "5GHz HT20",    "2GHz HT40",
	   "5GHz HT40",  "Unknown (9)", "Unknown (10)", "Unknown (11)",
	"Unknown (12)", "Unknown (13)", "Unknown (14)", "Unknown (15)"
};

/**
 * Detect possible EEPROM I/O byteswapping and toggle I/O byteswap compensation
 * if need it to consistently load EEPROM data.
 *
 * NB: all offsets are in 16-bits words
 */
bool __ar5416_toggle_byteswap(struct atheepmgr *aem, uint32_t eepmisc_off,
			      uint32_t binbuildnum_off)
{
	uint16_t word;
	int magic_is_be;

	/* First check whether magic is Little-endian or not */
	if (!EEP_READ(AR5416_EEPROM_MAGIC_OFFSET, &word)) {
		printf("Toggle EEPROM I/O byteswap compensation\n");
		return false;
	}
	magic_is_be = word != AR5416_EEPROM_MAGIC;	/* Constant is LE */

	/**
	 * Now read {opCapFlags,eepMisc} pair of fields that lay in the same
	 * EEPROM word. opCapFlags first bit indicates 5GHz support, while
	 * eepMisc first bit indicates BigEndian EEPROM data. Due to possible
	 * byteswap we are unable to distinguish between these two fields.
	 *
	 * So we have only two certain cases: if first bit in both octets are
	 * set, then we have a Big-endian EEPROM data with activated 5GHz
	 * support, and in opposite if bits in both octets are reset, then we
	 * have a Little-endian EEPROM with disabled 5GHz support. In both cases
	 * I/O byteswap could be detected using only magic field value: as soon
	 * as we know an EEPROM format, magic format mismatch indicates the I/O
	 * byteswap.
	 *
	 * If a first bit set only in one octet, then we are in an abiguity
	 * case:
	 *
	 *  EEP        Host   I/O  word &
	 * type  5GHz  type  swap  0x0101
	 * ------------------------------
	 *  LE     1    LE     N   0x0001
	 *  LE     1    BE     Y   0x0001
	 *  BE     0    LE     Y   0x0001
	 *  BE     0    BE     N   0x0001
	 *  LE     1    LE     Y   0x0100
	 *  LE     1    BE     N   0x0100
	 *  BE     0    LE     N   0x0100
	 *  BE     0    BE     Y   0x0100
	 *
	 *  And we will need some more heuristic to solve it (see below).
	 */
	if (!EEP_READ(eepmisc_off, &word)) {
		fprintf(stderr, "EEPROM misc field read failed\n");
		return false;
	}

	word &= 0x0101;	/* Clear all except 5GHz and BigEndian bits */
	if (word == 0x0000) {/* Clearly not Big-endian EEPROM */
		if (!magic_is_be)
			goto skip_eeprom_io_swap;
		if (aem->verbose > 1)
			printf("Got byteswapped Little-endian EEPROM data\n");
		goto toggle_eeprom_io_swap;
	} else if (word == 0x0101) {/* Clearly Big-endian EEPROM */
		if (magic_is_be)
			goto skip_eeprom_io_swap;
		if (aem->verbose > 1)
			printf("Got byteswapped Big-endian EEPROM data\n");
		goto toggle_eeprom_io_swap;
	}

	if (aem->verbose > 1)
		printf("Data is possibly byteswapped\n");

	/**
	 * Calibration software (ART) version in each seen AR5416/AR92xx EEPROMs
	 * is a 32-bits value that has a following format:
	 *
	 *    .----------- Major version (always zero)
	 *    | .--------- Minor version (always non-zero)
	 *    | | .------- Build (always non-zero)
	 *    | | | .----- Unused (always zero)
	 *    | | | |
	 * 0xMMmmrr00
	 *
	 * For example: 0x00091500 is a cal. software v0.9.21. For Little-endian
	 * EEPROM this version will be saved as 0x00 0x15 0x09 0x00, and for
	 * Big-endian EEPROM this will be saved as 0x00 0x09 0x15 0x00. As you
	 * can see both formats have a common part: first byte is always zero,
	 * while a second one is always non-zero. But if data will be
	 * byteswapped in each 16-bits word, then we will have an opposite
	 * situation: first byte become a non-zero, while the second one become
	 * zero.
	 *
	 * Checking each octet of first 16-bits part of version field is our
	 * endian-agnostic way to detect the byteswapping.
	 */

	if (!EEP_READ(binbuildnum_off, &word)) {
		fprintf(stderr, "Calibration software build read failed\n");
		return false;
	}

	word = le16toh(word);	/* Just to make a byteorder predictable */

	/* First we check for byteswapped case */
	if ((word & 0xff00) == 0 && (word & 0x00ff) != 0)
		goto toggle_eeprom_io_swap;

	/* Now check for non-byteswapped case */
	if ((word & 0xff00) != 0 && (word & 0x00ff) == 0) {
		if (aem->verbose > 1)
			printf("Looks like there are no byteswapping\n");
		goto skip_eeprom_io_swap;
	}

	/* We have some weird software version, giving up */
	if (aem->verbose > 1)
		printf("Unable to detect byteswap, giving up\n");

	if (!magic_is_be)	/* Prefer the Little-endian format */
		goto skip_eeprom_io_swap;

toggle_eeprom_io_swap:
	if (aem->verbose)
		printf("Toggle EEPROM I/O byteswap compensation\n");
	aem->eep_io_swap = !aem->eep_io_swap;

skip_eeprom_io_swap:
	return true;
}

/**
 * NB: size is in 16-bits words
 */
void ar5416_dump_eep_init(const struct ar5416_eep_init *ini, size_t size)
{
	int i, maxregsnum;

	printf("%-20s : 0x%04X\n", "Magic", ini->magic);
	for (i = 0; i < 8; ++i)
		printf("Region%d access       : %s\n", i,
		       sAccessType[(ini->prot >> (i * 2)) & 0x3]);
	printf("%-20s : 0x%04X\n", "Regs init data ptr", ini->iptr);
	printf("\n");

	EEP_PRINT_SUBSECT_NAME("Register(s) initialization data");

	maxregsnum = (2 * size - offsetof(typeof(*ini), regs)) /
		     sizeof(ini->regs[0]);
	for (i = 0; i < maxregsnum; ++i) {
		if (ini->regs[i].addr == 0xffff)
			break;
		printf("  %04X: %04X%04X\n", ini->regs[i].addr,
		       ini->regs[i].val_high, ini->regs[i].val_low);
	}

	printf("\n");
}

void ar5416_dump_target_power(const struct ar5416_cal_target_power *caldata,
			      int maxchans, const char * const rates[],
			      int nrates, int is_2g)
{
#define MARGIN		"    "
#define TP_ITEM_SIZE	(sizeof(struct ar5416_cal_target_power) + \
			 nrates * sizeof(uint8_t))
#define TP_NEXT_CHAN(__tp)	((void *)((uint8_t *)(__tp) + TP_ITEM_SIZE))

	const struct ar5416_cal_target_power *tp;
	int nchans, i, j;

	printf(MARGIN "%10s, MHz:", "Freq");
	tp = caldata;
	for (j = 0; j < maxchans; ++j, tp = TP_NEXT_CHAN(tp)) {
		if (tp->bChannel == AR5416_BCHAN_UNUSED)
			break;
		printf("  %4u", FBIN2FREQ(tp->bChannel, is_2g));
	}
	nchans = j;
	printf("\n");
	printf(MARGIN "----------------");
	for (j = 0; j < nchans; ++j)
		printf("  ----");
	printf("\n");

	for (i = 0; i < nrates; ++i) {
		printf(MARGIN "%10s, dBm:", rates[i]);
		tp = caldata;
		for (j = 0; j < nchans; ++j, tp = TP_NEXT_CHAN(tp))
			printf("  %4.1f", (double)tp->tPow2x[i] / 2);
		printf("\n");
	}

#undef TP_NEXT_CHAN
#undef TP_ITEM_SIZE
#undef MARGIN
}

void ar5416_dump_ctl_edges(const struct ar5416_cal_ctl_edges *edges,
			   int maxradios, int maxedges, int is_2g)
{
	const struct ar5416_cal_ctl_edges *e;
	int edge, rnum, open;

	for (rnum = 0; rnum < maxradios; ++rnum) {
		printf("\n");
		if (maxradios > 1)
			printf("    %d radio(s) Tx:\n", rnum + 1);
		printf("           Edges, MHz:");
		for (edge = 0, open = 1; edge < maxedges; ++edge) {
			e = &edges[rnum * maxedges + edge];
			if (!e->bChannel)
				break;
			printf(" %c%4u%c",
			       !CTL_EDGE_FLAGS(e->ctl) && open ? '[' : ' ',
			       FBIN2FREQ(e->bChannel, is_2g),
			       !CTL_EDGE_FLAGS(e->ctl) && !open ? ']' : ' ');
			if (!CTL_EDGE_FLAGS(e->ctl))
				open = !open;
		}
		printf("\n");
		printf("      MaxTxPower, dBm:");
		for (edge = 0; edge < maxedges; ++edge) {
			e = &edges[rnum * maxedges + edge];
			if (!e->bChannel)
				break;
			printf("  %4.1f ", (double)CTL_EDGE_POWER(e->ctl) / 2);
		}
		printf("\n");
	}
}

void ar5416_dump_ctl(const uint8_t *index,
		     const struct ar5416_cal_ctl_edges *data,
		     int maxctl, int maxchains, int maxradios, int maxedges)
{
	int i;
	uint8_t ctl;

	for (i = 0; i < maxctl; ++i) {
		if (!index[i])
			break;
		ctl = index[i];
		printf("  %s %s:\n", eep_ctldomains[ctl >> 4],
		       eep_ctlmodes[ctl & 0x0f]);

		ar5416_dump_ctl_edges(data + i * (maxchains * maxedges),
				      maxradios, maxedges,
				      eep_ctlmodes[ctl & 0x0f][0] == '2'/*:)*/);

		printf("\n");
	}
}

uint16_t eep_calc_csum(const uint16_t *buf, size_t len)
{
	uint16_t csum = 0;
	size_t i;

	for (i = 0; i < len; i++)
		csum ^= *buf++;

	return csum;
}
