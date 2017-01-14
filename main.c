/*
 * Basic firmware for an RGB LED notifier clone of the Riso Kaguku
 * Webmail Notifier.
 *
 * Copyright 2016 Jonathan McDowell <noodles@earth.li>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"
#include "libs-device/osccal.h"

#define GREEN_BIT  1 /* Bit 0 on port B */
#define RED_BIT    2 /* Bit 1 on port B */
#define BLUE_BIT   4 /* Bit 2 on port B */
#define ALL_BITS (RED_BIT | GREEN_BIT | BLUE_BIT)
#define CMD_SET_SERIAL 0xfa

int serno_str[] = {
	USB_STRING_DESCRIPTOR_HEADER(8),
	'U', 'N', 'S', 'E', 'T', 'X', 'X', 'X',
};

PROGMEM const char usbDescriptorConfiguration[CONFIG_DESCRIPTOR_SIZE] = {    /* USB configuration descriptor */
	9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
	USBDESCR_CONFIG,    /* descriptor type */
	18 + 7 + 7 + 9, 0,
	/* total length of data returned (including inlined descriptors) */
	1,          /* number of interfaces in this configuration */
	1,          /* index of this configuration */
	0,          /* configuration name string index */
	(1 << 7) | USBATTR_REMOTEWAKE,      /* attributes */
	USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */
	/* interface descriptor follows inline: */
	9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE, /* descriptor type */
	0,          /* index of this interface */
	0,          /* alternate setting for this interface */
	2,          /* endpoints excl 0: number of endpoint descriptors to follow */
	USB_CFG_INTERFACE_CLASS,
	USB_CFG_INTERFACE_SUBCLASS,
	USB_CFG_INTERFACE_PROTOCOL,
	0,          /* string index for interface */
	9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
	USBDESCR_HID,   /* descriptor type: HID */
	0x01, 0x01, /* BCD representation of HID version */
	0x00,       /* target country code */
	0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
	0x22,       /* descriptor type: report */
	USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, 0,  /* total length of report descriptor */
	/* endpoint descriptor for endpoint 1 */
	7,          /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	(char)0x81, /* IN endpoint number 1 */
	0x03,       /* attrib: Interrupt endpoint */
	8, 0,       /* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL, /* in ms */
	/* endpoint descriptor for endpoint 2 */
	7,          /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	2,			/* OUT endpoint number 2 */
	0x03,       /* attrib: Interrupt endpoint */
	5, 0,       /* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL, /* in ms */
};

PROGMEM const char usbHidReportDescriptor[22] = {
	0x06, 0x00, 0xff,		/* USAGE PAGE (Generic Desktop) */
	0x09, 0x01,			/* USAGE (Vendor Usage 1) */
	0xa1, 0x01,			/* COLLECTION (Application) */
	0x15, 0x00,			/*   LOGICAL_MINIMUM (0) */
	0x26, 0xff, 0x00,		/*   LOGICAL_MAXIMUM (255) */
	0x75, 0x08,			/*   REPORT_SIZE (8 bits) */
	0x95, 0x08,			/*   REPORT_COUNT (8 elements) */
	0x09, 0x00,			/*   USAGE (Undefined) */
	0xb2, 0x02, 0x01,		/*   FEATURE (Data, Var, Abs, Buf) */
	0xc0				/* END_COLLECTION */
};

inline char hexdigit(int i)
{
	return (i < 10) ? ('0' + i) : ('A' - 10 + i);
}

void fetch_serno(void)
{
	uint32_t serno;

	eeprom_read_block(&serno, 0, 4);
	if (serno == 0xffffffff) {
		serno_str[1] = 'U';
		serno_str[2] = 'N';
		serno_str[3] = 'S';
		serno_str[4] = 'E';
		serno_str[5] = 'T';
		serno_str[6] = 'X';
		serno_str[7] = 'X';
		serno_str[8] = 'X';
	} else {
		serno_str[1] = hexdigit((serno >> 28));
		serno_str[2] = hexdigit((serno >> 24) & 0xF);
		serno_str[3] = hexdigit((serno >> 20) & 0xF);
		serno_str[4] = hexdigit((serno >> 16) & 0xF);
		serno_str[5] = hexdigit((serno >> 12) & 0xF);
		serno_str[6] = hexdigit((serno >>  8) & 0xF);
		serno_str[7] = hexdigit((serno >>  4) & 0xF);
		serno_str[8] = hexdigit( serno        & 0xF);
	}
}

void update_serno(uint32_t serno)
{
	eeprom_write_block(&serno, 0x00, 4);

	serno_str[1] = hexdigit((serno >> 28));
	serno_str[2] = hexdigit((serno >> 24) & 0xF);
	serno_str[3] = hexdigit((serno >> 20) & 0xF);
	serno_str[4] = hexdigit((serno >> 16) & 0xF);
	serno_str[5] = hexdigit((serno >> 12) & 0xF);
	serno_str[6] = hexdigit((serno >>  8) & 0xF);
	serno_str[7] = hexdigit((serno >>  4) & 0xF);
	serno_str[8] = hexdigit( serno        & 0xF);
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
		if ((rq->bRequest == USBRQ_HID_GET_REPORT) ||
				(rq->bRequest == USBRQ_HID_SET_REPORT)) {
			return 0xFF;
		}
	}

	return 0;
}

usbMsgLen_t usbFunctionDescriptor(usbRequest_t *rq)
{
	if (rq->wValue.bytes[1] == USBDESCR_STRING &&
			rq->wValue.bytes[0] == 3) {
		usbMsgPtr = (usbMsgPtr_t) serno_str;
		return sizeof(serno_str);
	}

	return 0;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
	uint8_t colour_map[8] = { 0, 2, 1, 5, 3, 6, 4, 7 };
	uint8_t status;

	if (len != 0) {
		status = 0;
		if (PORTB & RED_BIT)
			status |= 1;
		if (PORTB & GREEN_BIT)
			status |= 2;
		if (PORTB & BLUE_BIT)
			status |= 4;
		/* Map RGB back to Riso Kagaku colour code. */
		data[0] = colour_map[status];
		return len;
	}

	return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
	/*
	 * The Riso Kagaku has an internal colour table, map it back to an
	 * RGB bitmap.
	 */
	uint8_t colour_map[8] = { 0, 2, 1, 4, 6, 3, 5, 7 };
	uint8_t rgb_val, port_val;

	if (data[0] < 8) {
		rgb_val = colour_map[data[0]];

		port_val = PORTB & ~ALL_BITS;

		if (rgb_val & 1)
			port_val |= RED_BIT;
		if (rgb_val & 2)
			port_val |= GREEN_BIT;
		if (rgb_val & 4)
			port_val |= BLUE_BIT;

		PORTB = port_val;
	} else if (data[0] == CMD_SET_SERIAL) {
		update_serno(*(uint32_t *) &data[1]);
	}

	return len;
}

void usbFunctionWriteOut(uchar *data, uchar len)
{
	/*
	 * The Riso Kagaku has an internal colour table, map it back to an
	 * RGB bitmap.
	 */
	uint8_t colour_map[8] = { 0, 2, 1, 4, 6, 3, 5, 7 };
	uint8_t rgb_val, port_val;

	if (!len)
		return;

	if (data[0] < 8) {
		rgb_val = colour_map[data[0]];

		port_val = PORTB & ~ALL_BITS;

		if (rgb_val & 1)
			port_val |= RED_BIT;
		if (rgb_val & 2)
			port_val |= GREEN_BIT;
		if (rgb_val & 4)
			port_val |= BLUE_BIT;

		PORTB = port_val;
	} else if (len == 5 && data[0] == CMD_SET_SERIAL) {
		update_serno(*(uint32_t *) &data[1]);
	}
}

int __attribute__((noreturn)) main(void)
{
	unsigned char i;

	wdt_enable(WDTO_1S);

	fetch_serno();

	usbInit();
	usbDeviceDisconnect();

	i = 0;
	while (--i) {
		wdt_reset();
		_delay_ms(1);
	}

	usbDeviceConnect();

	/* Set the LED bits to output mode */
	DDRB |= ALL_BITS;
	/* Turn them off */
	PORTB &= ~ALL_BITS;

	sei(); /* We're ready to go; enable interrupts */

	while (1) {
		wdt_reset();
		usbPoll();
	}
}
