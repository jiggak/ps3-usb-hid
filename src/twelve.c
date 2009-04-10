/* Nes/Snes/Genesis/SMS/Atari to USB
 * Copyright (C) 2006-2007 Raphaël Assénat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The author may be contacted at raph@raphnet.net
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "gamepad.h"
#include "twelve.h"

#define REPORT_SIZE	4

static const char usbHidReportDescriptor[] PROGMEM = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       // USAGE (Game Pad)
    0xa1, 0x01,       // COLLECTION (Application)
    0x09, 0x01,       //   USAGE (Pointer)
    0xa1, 0x00,       //   COLLECTION (Physical)
    0x09, 0x30,       //     USAGE (X)
    0x09, 0x31,       //     USAGE (Y)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x02,       //     REPORT_COUNT (2)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0xc0,             //   END_COLLECTION
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (Button 1)
    0x29, 0x10,       //   USAGE_MAXIMUM (Button 16)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x10,       //   REPORT_COUNT (16)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0xc0              // END_COLLECTION
};


/*********** prototypes *************/
static void twelveInit(void);
static void twelveUpdate(void);
static char twelveChanged(void);
static void twelveBuildReport(unsigned char *reportBuffer);



// report matching the most recent bytes from the controller
static unsigned char last_read_controller_bytes[REPORT_SIZE];
// the most recently reported bytes
static unsigned char last_reported_controller_bytes[REPORT_SIZE];

static void readController(unsigned char bits[2])
{
	bits[0] = PINC;
	bits[1] = PINB;
}

static void twelveInit(void)
{
	unsigned char sreg;
	sreg = SREG;
	cli();


	/* 
	 * --- 10 button on multiuse2 pinout ---
	 * PC5: Up
	 * PC4: Down
	 * PC3: Left
	 * PC2: Right
	 *
	 * PC1: Button 0
	 * PC0: Button 1
	 * PB5: Button 2
	 * PB4: Button 3
	 * PB3: Button 4
	 * PB2: Button 5 (JP2)
	 * PB1: Button 6 (JP1)
	 * PB0: Button 7
	 */

	DDRB = 0; // all inputs
	PORTB |= 0xff; // all pullups enabled

	// all of portC input with pullups
	DDRC &= ~0x3F;
	PORTC |= 0x3F;

	twelveUpdate();

	SREG = sreg;
}



static void twelveUpdate(void)
{
	unsigned char data[2];
	int x=128,y=128;

	readController(data);

	/* Buttons are active low. Invert values. */
	data[0] = data[0] ^ 0xff;
	data[1] = data[1] ^ 0xff;

	if (data[0] & 0x20) { y = 0; } // up
	if (data[0] & 0x10) { y = 255; } //down
	if (data[0] & 0x08) { x = 0; }  // left
	if (data[0] & 0x04) { x = 255; } // right

	last_read_controller_bytes[0]=x;
	last_read_controller_bytes[1]=y;
	last_read_controller_bytes[2]=0;
	last_read_controller_bytes[3]=0;

	if (data[0] & 0x02) // btn 0
		last_read_controller_bytes[2] |= 0x01;
	if (data[0] & 0x01) // btn 1
		last_read_controller_bytes[2] |= 0x02;
	if (data[1] & 0x20) // btn 2
		last_read_controller_bytes[2] |= 0x04;
	if (data[1] & 0x10) // btn 3
		last_read_controller_bytes[2] |= 0x08;
	if (data[1] & 0x08) // btn 4
		last_read_controller_bytes[2] |= 0x10;
	if (data[1] & 0x04) // btn 5
		last_read_controller_bytes[2] |= 0x20;
	if (data[1] & 0x02) // btn 6
		last_read_controller_bytes[3] |= 0x01;
	if (data[1] & 0x01) // btn 7
		last_read_controller_bytes[3] |= 0x02;
}

static char twelveChanged(void)
{
	static int first = 1;
	if (first) { first = 0;  return 1; }
	
	return memcmp(last_read_controller_bytes, 
					last_reported_controller_bytes, REPORT_SIZE);
}

static void twelveBuildReport(unsigned char *reportBuffer)
{
	if (reportBuffer != NULL)
	{
		memcpy(reportBuffer, last_read_controller_bytes, REPORT_SIZE);
	}
	memcpy(last_reported_controller_bytes, 
			last_read_controller_bytes, 
			REPORT_SIZE);	
}

Gamepad twelveGamepad = {
	report_size: 		REPORT_SIZE,
	reportDescriptorSize:	sizeof(usbHidReportDescriptor),
	init: 			twelveInit,
	update: 		twelveUpdate,
	changed:		twelveChanged,
	buildReport:		twelveBuildReport
};

Gamepad *twelveGetGamepad(void)
{
	twelveGamepad.reportDescriptor = (void*)usbHidReportDescriptor;

	return &twelveGamepad;
}

