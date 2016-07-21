/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * Copyright (c) 2013 Sergey Ryazanov <ryazanov.s.a@gmail.com>
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

#ifndef HW_H
#define HW_H

#define AR_SREV			0x4020
#define AR_SREV_ID		0x000000FF
#define AR_SREV_VERSION		0x000000F0
#define AR_SREV_VERSION_S	4
#define AR_SREV_REVISION	0x00000007
#define AR_SREV_VERSION2	0xFFFC0000
#define AR_SREV_VERSION2_S	18
#define AR_SREV_TYPE2		0x0003F000
#define AR_SREV_TYPE2_S		12
#define AR_SREV_REVISION2	0x00000F00
#define AR_SREV_REVISION2_S	8

#define AR5211_EEPROM_ADDR		0x6000

#define AR5211_EEPROM_DATA		0x6004

#define AR5211_EEPROM_CMD		0x6008
#define AR5211_EEPROM_CMD_READ		BIT(0)
#define AR5211_EEPROM_CMD_WRITE		BIT(1)
#define AR5211_EEPROM_CMD_RESET		BIT(2)

#define AR5211_EEPROM_STATUS			0x600c
#define AR5211_EEPROM_STATUS_READ_ERROR		BIT(0)
#define AR5211_EEPROM_STATUS_READ_COMPLETE	BIT(1)
#define AR5211_EEPROM_STATUS_WRITE_ERROR	BIT(2)
#define AR5211_EEPROM_STATUS_WRITE_COMPLETE	BIT(3)

#define AR5416_EEPROM_S				2
#define AR5416_EEPROM_OFFSET			0x2000

#define AR_EEPROM_STATUS_DATA		(AR_SREV_9340(aem) ? 0x40c8 : \
					 (AR_SREV_9300_20_OR_LATER(aem) ? \
					  0x4084 : 0x407c))
#define AR_EEPROM_STATUS_DATA_VAL		0x0000ffff
#define AR_EEPROM_STATUS_DATA_VAL_S		0
#define AR_EEPROM_STATUS_DATA_BUSY		0x00010000
#define AR_EEPROM_STATUS_DATA_BUSY_ACCESS	0x00020000
#define AR_EEPROM_STATUS_DATA_PROT_ACCESS	0x00040000
#define AR_EEPROM_STATUS_DATA_ABSENT_ACCESS	0x00080000

#define AR5XXX_GPIO_CTRL	0x4014

#define AR5XXX_GPIO_CTRL_DRV		0x3
#define AR5XXX_GPIO_CTRL_DRV_NO		0x0
#define AR5XXX_GPIO_CTRL_DRV_LOW	0x1
#define AR5XXX_GPIO_CTRL_DRV_HI		0x2
#define AR5XXX_GPIO_CTRL_DRV_ALL	0x3

#define AR5XXX_GPIO_OUT		0x4018

#define AR5XXX_GPIO_IN		0x401c

#define AR9XXX_GPIO_IN_OUT	(AR_SREV_9340(aem) ? 0x4028 : 0x4048)

#define AR5416_GPIO_IN_VAL	0x0FFFC000
#define AR5416_GPIO_IN_VAL_S	14
#define AR9280_GPIO_IN_VAL	0x000FFC00
#define AR9280_GPIO_IN_VAL_S	10
#define AR9285_GPIO_IN_VAL	0x00FFF000
#define AR9285_GPIO_IN_VAL_S	12
#define AR9287_GPIO_IN_VAL	0x003FF800
#define AR9287_GPIO_IN_VAL_S	11
#define AR9300_GPIO_IN_VAL	0x0001FFFF
#define AR9300_GPIO_IN_VAL_S	0

#define AR9XXX_GPIO_OE_OUT	(AR_SREV_9340(aem) ? 0x4030 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4050 : \
				  0x404c))

#define AR9XXX_GPIO_OE_OUT_DRV		0x3
#define AR9XXX_GPIO_OE_OUT_DRV_NO	0x0
#define AR9XXX_GPIO_OE_OUT_DRV_LOW	0x1
#define AR9XXX_GPIO_OE_OUT_DRV_HI	0x2
#define AR9XXX_GPIO_OE_OUT_DRV_ALL	0x3

#define AR9XXX_GPIO_OUTPUT_MUX1	(AR_SREV_9340(aem) ? 0x4048 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4068 : \
				  0x4060))
#define AR9XXX_GPIO_OUTPUT_MUX2	(AR_SREV_9340(aem) ? 0x404C : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x406C : \
				  0x4064))
#define AR9XXX_GPIO_OUTPUT_MUX3	(AR_SREV_9340(aem) ? 0x4050 : \
				 (AR_SREV_9300_20_OR_LATER(aem) ? 0x4070 : \
				  0x4068))

#define AR9XXX_GPIO_OUTPUT_MUX_MASK		0x1f
#define AR9XXX_GPIO_OUTPUT_MUX_OUTPUT		0x00
#define AR9XXX_GPIO_OUTPUT_MUX_TX_FRAME		0x03
#define AR9XXX_GPIO_OUTPUT_MUX_RX_CLEAR		0x04
#define AR9XXX_GPIO_OUTPUT_MUX_MAC_NETWORK	0x05
#define AR9XXX_GPIO_OUTPUT_MUX_MAC_POWER	0x06

#endif /* HW_H */
