// LED blink demo, now driven live over USB (rv003usb)

#include "ch32fun.h"
#include <string.h>
#include "rv003usb.h"

#define PIN_LED 		PD6
#define PIN_ADC_INPUT	PA1
#define ADC_INPUT		1

enum {
	CMD_LED_OFF      = 0,
	CMD_LED_ON       = 1,
	CMD_LED_BLINK 	 = 2,
};

// Report ID 0xaa, 10 bytes total (1 ID byte + 9 data bytes), matching
// HID_REPORT_COUNT(9) in usb_config.h. This one report is reused for both
// hid_send_feature_report (host->device command) and
// hid_get_feature_report (device->host status).
#define MAILBOX_LEN 10
#define MAILBOX_REPORT_ID 0xaa

static volatile uint8_t mailbox_in[MAILBOX_LEN];  // host -> device, filled by usb_handle_user_data
static uint8_t mailbox_out[MAILBOX_LEN];          // device -> host, filled on demand in usb_handle_hid_get_report_start

static volatile uint8_t command = CMD_LED_BLINK;
static volatile uint8_t blink_half_period_ticks = 25; // *10ms => 250ms, matches the original blink example
static volatile uint8_t led_state = 0;
static volatile uint32_t uptime_ms = 0;
static volatile uint16_t adc_value = 0; // last PA1 reading, 10-bit (0-1023)

static void SetLed( int on )
{
	led_state = on ? 1 : 0;
	funDigitalWrite( PIN_LED, on ? FUN_HIGH : FUN_LOW );
}

static void ApplyCommand( const uint8_t * cmd )
{
	uint8_t new_command = cmd[1];
	if( new_command == CMD_LED_BLINK )
	{
		if( cmd[2] ) blink_half_period_ticks = cmd[2];
		command = CMD_LED_BLINK;
	}
	else
	{
		command = CMD_LED_OFF; // parked; SetLed below drives the actual pin
		SetLed( new_command == CMD_LED_ON );
	}
}

int main()
{
	SystemInit();

	funGpioInitAll(); // only enables the GPIO port clocks; each pin still needs its own funPinMode() below

	funPinMode( PIN_LED, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funPinMode( PIN_ADC_INPUT, GPIO_CFGLR_IN_ANALOG );
	funAnalogInit();

	Delay_Ms(100); // Ensures USB re-enumeration after bootloader or reset; spec demands >2.5us (TDDIS)
	usb_setup();

	uint32_t last_toggle = 0;

	while(1)
	{
		Delay_Ms(10);
		uptime_ms += 10;
		adc_value = funAnalogRead( ADC_INPUT );

		// Auto-blink: flip the LED once per half-period, giving a full
		// blink period of 2 * blink_half_period_ticks * 10ms.
		if( command == CMD_LED_BLINK &&
		    uptime_ms - last_toggle >= (uint32_t)blink_half_period_ticks * 10 )
		{
			last_toggle = uptime_ms;
			SetLed( !led_state );
		}
	}
}

// We don't use the interrupt-IN endpoint for anything; everything rides on
// control (feature report) transfers, so just ack it if the host ever polls it.
// The HID class descriptor still requires this endpoint to exist, so this
// handler exists purely to satisfy the protocol.
void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp )
		usb_send_empty( sendtok );
}

// hid_get_feature_report: build the live status snapshot on demand.
// TX
void usb_handle_hid_get_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	uint32_t now = uptime_ms;

	mailbox_out[0] = MAILBOX_REPORT_ID;
	mailbox_out[1] = led_state;
	mailbox_out[2] = now & 0xff;
	mailbox_out[3] = (now>>8) & 0xff;
	mailbox_out[4] = (now>>16) & 0xff;
	mailbox_out[5] = (now>>24) & 0xff;
	mailbox_out[6] = command;
	mailbox_out[7] = blink_half_period_ticks;
	mailbox_out[8] = adc_value & 0xff;
	mailbox_out[9] = (adc_value>>8) & 0xff;

	// Clamp to MAILBOX_LEN: reqLen is host-supplied (wLength) and could ask
	// for more than mailbox_out actually holds, which would read out of bounds.
	if( reqLen > MAILBOX_LEN ) reqLen = MAILBOX_LEN;
	e->opaque = mailbox_out;
	e->max_len = reqLen;
}

// hid_send_feature_report: just declare the expected length; the actual
// bytes land via usb_handle_user_data below (mirrors demo_hidapi's scratchpad).
void usb_handle_hid_set_report_start( struct usb_endpoint * e, int reqLen, uint32_t lValueLSBIndexMSB )
{
	if( reqLen > MAILBOX_LEN ) reqLen = MAILBOX_LEN;
	e->max_len = reqLen;
}

// RX
void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	int offset = e->count<<3;
	int torx = e->max_len - offset;
	if( torx > len ) torx = len;
	if( torx > 0 )
	{
		memcpy( (uint8_t*)mailbox_in + offset, data, torx );
		e->count++;
		if( ( e->count << 3 ) >= e->max_len )
		{
			ApplyCommand( (const uint8_t*)mailbox_in );
		}
	}
}

void usb_handle_other_control_message( struct usb_endpoint * e, struct usb_urb * s, struct rv003usb_internal * ist )
{
	// Nothing else expected; ignore.
}
