#ifndef FOC_H_
#define FOC_H_

#include "sin_lut.c"
#include <stdint.h>
#define SQRT_3 1.73205
#define ONE_SQRT_3 0.57735

typedef struct {
  float alpha;
  float beta;
} foc_ab_t;

typedef struct {
  float d;
  float q;
} foc_dq_t;

typedef struct {
  float u;
  float v;
  float w;
} foc_uvw_t;

// Calculates the duty cycle for the uvw legs of an inverter given a space
// vector in the stationary alpha beta system Duty cycles are output in the form
// 0 to 100
void foc_svpwm_duty_calculate(const foc_ab_t *ab_in, foc_uvw_t *uvw_out,
                              const uint32_t pwm_period);

// Performs an inverse park transform
// Given a vector in the rotating dq coordinate system, and the angle in radians
// with respect to the alpha axis, gives a vector in the alpha beta system
void foc_inverse_park(const float angle, const foc_dq_t *dq_in,
                      foc_ab_t *ab_out);

// Calculates sine from a look up table
extern const float sin_lut[1024];
void sin_and_cos(float angle, float *sin, float *cos) {
  uint16_t index = angle * INTEGERS_PER_RADIAN;
  // index >>= (16 - 10);
  *sin = sin_lut[index];
  *cos = sin_lut[index + COSINE_PHASE_SHIFT];
};
#endif // FOC_H_
