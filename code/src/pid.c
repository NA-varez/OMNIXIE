/*
    pid.c

    PID Controller

        Proportionl
            Flat constant multiply on error

        Integral
            Over time, the integral can saturate the output to a maximum control signal

        Derivative
            Has low-pass filter to prevent HF signal from amplifying D term
        
    
    Adapted from https://github.com/pms67/PID by Phillip Salmony
*/

#include "pid.h"


void initPID(PIDController * pid) {

    /* Clear controller variables */
    pid->i = 0.0;
    pid->prevError = 0.0;

    pid->d = 0.0;
    pid->prevMeasurement = 0.0;

    pid->out = 0.0;

}

float updatePID(PIDController * pid, float setpoint, float measurement) {

    /*
        Error signal
    */
    float error = setpoint - measurement;

    /*
        Proportional
    */
    float p = pid->Kp * error;

    /*
        Integral
    */
    pid->i = pid->i
            + (pid->Ki * (pid->T / 2))
            * (error + pid->prevError);


    // Anti-wind up via dynamically ranged integrator clamping
    float limMinInt;
    float limMaxInt;

    // p + i must never be larger than maximum output
    if(p < pid->limMax) {
        limMaxInt = pid->limMax - p;
    } else {
        limMaxInt = 0.0;
    }

    // p + i must never be smaller than minimum output
    if(p > pid->limMin) {
        limMinInt = pid->limMin - p;
    } else {
        limMinInt = 0.0;
    }
    
    // Clamp integrator
    if(pid->i > limMaxInt) pid->i = limMaxInt;
    else if (pid->i < limMinInt) pid->i = limMinInt;


    /*
        Derivative
    */
    pid->d = -(2.0 * pid->Kd * (measurement - pid->prevMeasurement)	// Note: derivative on measurement, therefore minus sign in front of equation! 
            + (2.0 * pid->tau - pid->T) * pid->d)
            / (2.0 * pid->tau + pid->T);


    /*
        Compute output within limits
    */
    float _out = p + pid->i + pid->d;

    if (_out > pid->limMax) pid->out = pid->limMax;

    else if (_out < pid->limMin) pid->out = pid->limMin;

    else pid->out = _out;

    /*
        Update previous error for next update
    */
    pid->prevError = error;

    /*
        Update previous measurement for next update
    */
    pid->prevMeasurement = measurement;

    
    // Return output
    return pid->out;

}   