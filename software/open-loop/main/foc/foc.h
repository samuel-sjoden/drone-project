#ifndef FOC_H_
#define FOC_H_

#include <stdint.h>
#define SQRT_3 1.73205
#define ONE_SQRT_3 0.57735
#define TWO_PI 6.28318530718
#define PI TWO_PI / 2
#define LUT_SIZE 1024
#define INTEGERS_PER_RADIAN (float)LUT_SIZE / (float)(TWO_PI + PI / 2)
#define COSINE_PHASE_SHIFT (PI / 2 * INTEGERS_PER_RADIAN)

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
static inline void sin_and_cos(float angle, float *sin, float *cos) {
  uint16_t index = angle * INTEGERS_PER_RADIAN;
  uint16_t cos_index = index + COSINE_PHASE_SHIFT;
  *sin = sin_lut[index];
  *cos = sin_lut[cos_index];
};
#endif // FOC_H_
