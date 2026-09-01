// #include "ch32fun.h"
// #include <string.h>
// #include "rv003usb.h"


#define PIN_HV_GATE_DRIVE		//PC7
#define PIN_HV_FEEDBACK		//PC4
#define ANALOG_HV_FEEDBACK	2		//PC4 Analog 2

#define PIN_SDA			//PC1
#define PIN_SCL 		//PC2

void omnixie_init() {
	// configure

	// init

	// default states
}

/**
 * TODO
 * 
 * How bout you learn how to program a PID in firmware
 * Efficient PID code in firmware. Or at least get the bones
 * understand the bones then use cnixxie to do the optimizations
 * 
 * Driver for I2C GPIO Expander
 * 
 * Try simulating PID control on-chip? MATLAB is what this is for really
 * 
 * HV PID control
 * 	Get 200V max
 * 	Use T1Ch2 or T2Ch2
 * 	Feedback via A2
 * 
 * 
 */

