#pragma once

typedef struct {

    /* Controller gains */
    float Kp;
    float Ki;
    float Kd;

    /* Derivative low-pass filter constant */
    float tau;

    /* Control output limits */
    float limMin;
    float limMax;

    /* Sample time (in seconds)*/
    float T;

    /* Controller "memory" */
    float i;
    float prevError;        /* Required for integrator */
    float d;
    float prevMeasurement;  /* Required for differentiator */

    /* Controller output*/
    float out;

} PIDController;

void initPID(PIDController * pid);
float updatePID(PIDController * pid, float setpoint, float measurement);