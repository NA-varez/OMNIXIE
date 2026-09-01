// Host-side control panel for omnixie.c, talking to the CH32V003 over
// USB (rv003usb) rather than the WCH-Link SWIO debug-link mailbox 

//
//
// Protocol (must match omnixie.c):
//   8-byte buffer, byte[0] is always the HID report ID (0xaa).
//   Host -> device (hid_send_feature_report):
//     [1] = command: 0 = LED off, 1 = LED on, 2 = auto-blink
//     [2] = blink half-period, in 10ms ticks (only used for command 2)
//   Device -> host (hid_get_feature_report):
//     [1] = current LED state (0/1)
//     [2..5] = uptime in ms, little-endian
//     [6] = current command
//     [7] = current blink half-period, in 10ms ticks
//     [8..9] = PA1 ADC reading, little-endian, 10-bit (0-1023)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "hidapi.c"

#define CNFG_IMPLEMENTATION
#define CNFGOGL
#include "rawdraw_sf.h"

#define USB_VENDOR_ID  0x1209
#define USB_PRODUCT_ID 0xd003
#define REPORT_ID      0xaa
#define MAILBOX_LEN    10
#define ADC_HISTORY    200 // ~4s of samples at the 20ms poll rate below
#define ADC_VREF       3.3f

enum { CMD_LED_OFF = 0, CMD_LED_ON = 1, CMD_LED_BLINK = 2 };

static int mouse_x, mouse_y, mouse_down;

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { mouse_x = x; mouse_y = y; mouse_down = bDown; }
void HandleMotion( int x, int y, int mask ) { mouse_x = x; mouse_y = y; }
int HandleDestroy() { return 0; }

typedef struct { int x0, y0, x1, y1; const char * label; } Button;

// Used when checking whether the cursor is within the bounds of the button
static int InButton( const Button * b, int x, int y )
{
	return x >= b->x0 && x < b->x1 && y >= b->y0 && y < b->y1;
}

static void DrawButton( const Button * b, int active )
{
	// Change color if active
	CNFGColor( active ? 0x40a040ff : 0x606060ff ); 
	CNFGTackRectangle( b->x0, b->y0, b->x1, b->y1 );
	CNFGColor( 0xffffffff );
	CNFGPenX = b->x0 + 8;
	CNFGPenY = b->y0 + (b->y1 - b->y0)/2 - 4;
	CNFGDrawText( b->label, 2 );
}

static void SendCommand( hid_device * hd, uint8_t cmd, uint8_t param )
{
	uint8_t buf[MAILBOX_LEN] = {0};
	buf[0] = REPORT_ID;
	buf[1] = cmd;
	buf[2] = param;
	int r = hid_send_feature_report( hd, buf, sizeof(buf) ); // TX
	if( r != sizeof(buf) )
		fprintf( stderr, "Warning: command send failed (%d)\n", r );
}

int main()
{
	CNFGSetup( "OMNIXIE USB Control Panel", 480, 420 );

	Button btn_off  = { 20, 60, 140, 100, "LED OFF" };
	Button btn_on   = { 160, 60, 280, 100, "LED ON" };
	Button btn_blink = { 300, 60, 420, 100, "BLINK" };
	Button btn_slower = { 300, 120, 360, 160, "-" }; // - 10ms
	Button btn_faster = { 380, 120, 440, 160, "+" }; // + 10ms

	uint8_t command = 0xff; // sentinel: no report received yet, doesn't match any real CMD_*
	uint8_t led_state = 0, period_ticks = 0;
	uint32_t uptime_ms = 0;
	uint16_t adc_value = 0;
	uint16_t adc_history[ADC_HISTORY] = {0};
	int adc_history_head = 0;
	int connected = 0;
	hid_device * hd = NULL;
	int was_down = 0;
	int frames_since_open_attempt = 0;

	while( CNFGHandleInput() )
	{
		CNFGBGColor = 0x101020ff;
		CNFGClearFrame();

		if( !hd && frames_since_open_attempt-- <= 0 )
		{
			connected = 0;
			hd = hid_open( USB_VENDOR_ID, USB_PRODUCT_ID, L"OMNIXIE00001" );
			frames_since_open_attempt = 50; // ~1s at the 20ms frame pacing below
		}

		if( hd )
		{
			uint8_t buf[MAILBOX_LEN];
			memset( buf, 0, sizeof(buf) );
			buf[0] = REPORT_ID;
			int r = hid_get_feature_report( hd, buf, sizeof(buf) ); // RX from MCU
			if( r >= MAILBOX_LEN )
			{
				connected = 1;
				led_state = buf[1];
				uptime_ms = (uint32_t)buf[2] | ((uint32_t)buf[3]<<8) | ((uint32_t)buf[4]<<16) | ((uint32_t)buf[5]<<24);
				command = buf[6];
				period_ticks = buf[7];
				adc_value = (uint16_t)buf[8] | ((uint16_t)buf[9]<<8);
				adc_history[adc_history_head] = adc_value;
				adc_history_head = (adc_history_head + 1) % ADC_HISTORY;
			}
			else
			{
				hid_close( hd );
				hd = NULL;
				connected = 0;
			}
		}

		int click = mouse_down && !was_down;
		was_down = mouse_down;

		if( click && hd )
		{
			if( InButton( &btn_off, mouse_x, mouse_y ) )
				SendCommand( hd, CMD_LED_OFF, 0 );
			else if( InButton( &btn_on, mouse_x, mouse_y ) )
				SendCommand( hd, CMD_LED_ON, 0 );
			else if( InButton( &btn_blink, mouse_x, mouse_y ) )
				SendCommand( hd, CMD_LED_BLINK, period_ticks ? period_ticks : 25 );
			else if( InButton( &btn_slower, mouse_x, mouse_y ) )
			{
				uint8_t np = period_ticks > 5 ? period_ticks - 5 : 5;
				SendCommand( hd, CMD_LED_BLINK, np );
			}
			else if( InButton( &btn_faster, mouse_x, mouse_y ) )
			{
				uint8_t np = period_ticks < 250 ? period_ticks + 5 : 250;
				SendCommand( hd, CMD_LED_BLINK, np );
			}
		}

		CNFGColor( 0xffffffff );
		CNFGPenX = 20; CNFGPenY = 15;
		CNFGDrawText( connected ? "Connected over USB" : "Waiting for device (VID 1209 / PID d003)...", 3 );

		DrawButton( &btn_off,  connected && command == CMD_LED_OFF && !led_state );
		DrawButton( &btn_on,   connected && command == CMD_LED_OFF && led_state );
		DrawButton( &btn_blink, connected && command == CMD_LED_BLINK );
		DrawButton( &btn_slower, 0 );
		DrawButton( &btn_faster, 0 );

		// Live LED indicator, reflecting what the device just reported.
		CNFGColor( led_state ? 0xffcc00ff : 0x333333ff );
		{
			int cx = 60, cy = 220, rad = 25;
			RDPoint poly[16];
			int i;
			for( i = 0; i < 16; i++ )
			{
				double a = i * 2 * 3.14159265 / 16;
				poly[i].x = cx + (int)(rad * cosf(a));
				poly[i].y = cy + (int)(rad * sinf(a));
			}
			CNFGTackPoly( poly, 16 );
		}

		char status[256];
		snprintf( status, sizeof(status), "command: %s   period: %dms   uptime: %.1fs",
			command == CMD_LED_BLINK ? "AUTO" : (led_state ? "ON" : "OFF"),
			period_ticks * 10,
			uptime_ms / 1000.0 );
		CNFGColor( 0xccccccff );
		CNFGPenX = 20; CNFGPenY = 260;
		CNFGDrawText( status, 2 );

		{
			char adc_label[64];
			snprintf( adc_label, sizeof(adc_label), "ADC (PA1): %4d  (%.2fV)", adc_value, adc_value / 1023.0f * ADC_VREF );
			CNFGColor( 0xccccccff );
			CNFGPenX = 20; CNFGPenY = 300;
			CNFGDrawText( adc_label, 2 );

			int gx0 = 20, gy0 = 330, gx1 = 460, gy1 = 400;
			CNFGColor( 0x404040ff );
			CNFGDrawBox( gx0, gy0, gx1, gy1 );

			CNFGColor( 0x40c0ffff );
			int i;
			int px = 0, py = 0;
			for( i = 0; i < ADC_HISTORY; i++ )
			{
				uint16_t sample = adc_history[(adc_history_head + i) % ADC_HISTORY];
				int x = gx0 + i * (gx1 - gx0) / (ADC_HISTORY - 1);
				int y = gy1 - (int)( (sample / 1023.0f) * (gy1 - gy0) );
				if( i > 0 ) CNFGTackSegment( px, py, x, y );
				px = x; py = y;
			}
		}

		CNFGSwapBuffers();
		usleep( 20000 ); // ~50Hz poll; feature reports are control transfers, no need to hammer them
	}

	if( hd ) hid_close( hd );
	return 0;
}
