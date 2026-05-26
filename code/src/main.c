

#include <stdio.h>

#include "ch32fun.h"
#include "pid.h"

#define SYSTEM_CORE_CLOCK 48000000

// Function prototypes

#define ABSOLUTE_MAX_ADC_SET 190

// Clock divide for flyback switching frequency

// PID Loop Tuning Parameters



/**
 * ADC input filtering
 * 
 * Binary-shift IIR filtering
 */
#define ADC_IIR 2
#define VDD_IIR 2


// Target feedback, set by user.
int target_feedback = 0;

// Feedback based on what the user set and the part's VDD
int feedback_vdd = 0;

// Filtered ADC and VDD values.
int lastadc = 0;
int lastrefvdd = 0;

// NUMERIC FADING ?

// what da hell is dis

// inline? static?
// Fast multiply

// how did he get performance numbers here?
// How did he know that it is slower? B/c of cpu usage ?
// Does cpu usage inverserly correlate to faster program?
// or is it determined simply by number of instructions?
// probably depends on CPU architecture as well.






int main()
{

    // TODO: WHAT DO THESE HIGHLEVEL FUNCTIONS ACTUALLY LOOK LIKE?

	// Configure a watchdog timer so if the chip goes crazy it will reset.
	WatchdogSetup();

	// Use internall RC oscillator + 2xPLL to generate 48 MHz system clock.
    SystemInit48HSI();

	// For the ability to printf() if we want.
	SetupDebugPrintf();

	// Pet watchdog for the rest of startup.
	WatchdogPet();

    // TODO: DECOMPOSE THIS ENABLING. AM I USING THE SAME PINS FOR MY FLYBACK?
    
	// Enable Peripherals
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC |
		RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1 | RCC_APB2Periph_ADC1 |
		RCC_APB2Periph_AFIO;

	RCC->APB1PCENR = RCC_APB1Periph_TIM2;

    // TODO: HOW DOES THIS MAKE ALL TUBE CATHODES HIGH-Z?

	// I'm paranoid - let's make sure all tube cathodes are high-Z.
	ApplyOnMask( 0 );

	GPIOD->CFGLR = 
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*6) | // GPIO D6 Debug
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*7) | // DIG_AUX  (TIM2CH4)
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*3) | // DIG_9
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*2) | // DIG_8
		(GPIO_Speed_10MHz | GPIO_CNF_IN_FLOATING)<<(4*1) | // PGM Floats.
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);  // DIG_DOT

	GPIOC->CFGLR = 
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0) | // DIG_0
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*1) | // DIG_1
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*2) | // DIG_2
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*3) | // DIG_3
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*4) | // DIG_4
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*5) | // DIG_5
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*6) | // DIG_6
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*7);  // DIG_7


	GPIOA->CFGLR =
		(GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*1); //FLYBACK (T1CH2)

	SetupADC();
	SetupTimer1();
	SetupTimer2();

	*DMDATA0 = 0;

	target_feedback = 0;

	// Cause system timer to run and reload when it hits CMP and HCLK/8.
	// Also, don't stop at comparison value.
	SysTick->CTLR = 1;

	while(1)
	{
		uint32_t dmdword = *DMDATA0;
		if( (dmdword & 0xf0) == 0x40 )
		{
			// I think there is a compiler bug here.  For some reason if I put
			// the code in this function right here, it doesn't work right.
			// so I encapsulated the code in a function.
			//
			// This function handles commands we get over the programming
			// interface.  Like "set HV bus" or "set this digit on."
			HandleCommand( dmdword );
		}

		AdvanceFadePlace();

	}
}


// /* Controller parameters */
// #define PID_KP  2.0f
// #define PID_KI  0.5f
// #define PID_KD  0.25f

// #define PID_TAU 0.02f

// #define PID_LIM_MIN -10.0f
// #define PID_LIM_MAX  10.0f

// #define PID_LIM_MIN_INT -5.0f
// #define PID_LIM_MAX_INT  5.0f

// #define SAMPLE_TIME_S 0.01f

// /* Maximum run-time of simulation */
// #define SIMULATION_TIME_MAX 4.0f

// /* Simulated dynamical system (first order) */
// float TestSystem_Update(float inp);

// int main()
// {
    
//     /*
//         TODO

//         * The pid controller is very bulky. Uses a ton of floats. Do I really need those floats?

//         * Identify what pins I need to configure.
//             * ADC for feedback
//             * Switching signal

//         * Try a simple printf to verify that it can work after the implicit declaration of SetupDebugPrintf()

//         * Look into optimization strategies, less multiplies
//             * Try just using proportional term
        

//         * Avoid looking to deep on deriving the information yourself.
//             * Simply, digest the genius cnixxie code and adjust the PIDController 
        
//         * Get the rawdraw to work to adjust the setpoint for the converter on the fly via mouse

//         * Next steps are architecting a simple way for the device to identify that it is the seconds, tens, minutes, etc..
//             * Easiest way to do this is via hardware jumpers. 4 would be enough for 16 different places.
//             * The seconds is the master and holds the time value so that when a new driver board pings the i2c bus
//                 the master will send out i2c commands on the bus to whoever is listening. It sends out the value for minutes
//                 tens of minutes, hours, tens of hours,
//             * every digit has a specific hardcoded i2c address that is assigned everytime at startup byy checking the value of the digit pullups/pullldowns.
//             * every digit has a different delay it will have before it sends out a 0x00 call on the i2c bus.
    
    
//     */

// 	SystemInit();

// 	funGpioInitAll(); // Enable GPIOs

// 	SetupDebugPrintf();

//     /* Initialise PID controller */
//     PIDController pid = { PID_KP, PID_KI, PID_KD,
//                           PID_TAU,
//                           PID_LIM_MIN, PID_LIM_MAX,
// 			  PID_LIM_MIN_INT, PID_LIM_MAX_INT,
//                           SAMPLE_TIME_S };

//     initPID(&pid);

//     /* Simulate response using test system */
//     float setpoint = 1.0f;

//     printf("Time (s)\tSystem Output\tControllerOutput\r\n");
//     for (float t = 0.0f; t <= SIMULATION_TIME_MAX; t += SAMPLE_TIME_S) {

//         /* Get measurement from system */
//         float measurement = TestSystem_Update(pid.out);

//         /* Compute new control signal */
//         updatePID(&pid, setpoint, measurement);

//         printf("%f\t%f\t%f\r\n", t, measurement, pid.out);

//     }

//     return 0;
// }

// float TestSystem_Update(float inp) {

//     static float output = 0.0f;
//     static const float alpha = 0.02f;

//     output = (SAMPLE_TIME_S * inp + output) / (1.0f + alpha * SAMPLE_TIME_S);

//     return output;
// }