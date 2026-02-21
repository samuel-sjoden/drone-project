#include <stdint.h>

// Calculates the duty cycle for the uvw legs of an inverter given a space
// vector in the stationary alpha beta system Duty cycles are output in the form
// 0 to 100
void foc_svpwm_duty_calculate(const foc_ab_t *ab_in, foc_uvw_t *uvw_out,
                              const uint32_t pwm_period) {
  // First, determine the sector of the hexagon the space vector lies in
  uint8_t sector;
  if ((ab_in->beta) > 0) {
    if ((ab_in->alpha) > 0) {
      // Quadrant 1
      if ((ab_in->beta) > (ab_in->alpha) * SQRT_3) {
        sector = 2;
      } else {
        sector = 1;
      }
    } else {
      // Quadrant 2
      if ((ab_in->beta) > -(ab_in->alpha) * SQRT_3) {
        sector = 2;
      } else {
        sector = 3;
      }
    }
  } else {
    if ((ab_in->alpha) > 0) {
      // Quadrant 4
      if ((ab_in->beta) < -(ab_in->alpha) * SQRT_3) {
        sector = 5;
      } else {
        sector = 6;
      }
    } else {
      // Quadrant 3
      if ((ab_in->beta) < (ab_in->alpha) * SQRT_3) {
        sector = 5;
      } else {
        sector = 4;
      }
    }
  }

  // with the sector, the basis vectors can be determined, and the projections
  // of the space vector onto the basis
  float t1, t2, t3, t4, t5, t6;
  switch (sector) {
  case 1:
    t1 = ab_in->alpha - ONE_SQRT_3 * (ab_in->beta);
    t2 = (ab_in->beta) * 2 * ONE_SQRT_3;

    // Divide by two because half of the dead time is spent in the all on state,
    // the other half is the all off
    uvw_out->w = (pwm_period - t1 - t2) / 2;
    uvw_out->v = uvw_out->w + t2;
    uvw_out->u = uvw_out->v + t1;
    break;

  case 2:
    t2 = (ab_in->alpha) + ONE_SQRT_3 * (ab_in->beta);
    t3 = -(ab_in->alpha) + ONE_SQRT_3 * (ab_in->beta);

    uvw_out->w = (pwm_period - t2 - t3) / 2;
    uvw_out->u = uvw_out->w + t2;
    uvw_out->v = uvw_out->u + t3;
    break;

  case 3:
    t3 = 2 * ONE_SQRT_3 * (ab_in->beta);
    t4 = -(ab_in->alpha) - ONE_SQRT_3 * (ab_in->beta);

    uvw_out->u = (pwm_period - t3 - t4) / 2;
    uvw_out->w = uvw_out->u + t4;
    uvw_out->v = uvw_out->w + t3;
    break;
  case 4:
    t4 = -(ab_in->alpha) + ONE_SQRT_3 * (ab_in->beta);
    t5 = -2 * ONE_SQRT_3 * (ab_in->beta);

    uvw_out->u = (pwm_period - t4 - t5) / 2;
    uvw_out->v = uvw_out->u + t4;
    uvw_out->w = uvw_out->v + t5;
    break;

  case 5:
    t5 = -(ab_in->alpha) - ONE_SQRT_3 * (ab_in->beta);
    t6 = (ab_in->alpha) - ONE_SQRT_3 * (ab_in->beta);

    uvw_out->v = (pwm_period - t5 - t6) / 2;
    uvw_out->u = uvw_out->v + t6;
    uvw_out->w = uvw_out->u + t5;
    break;

  case 6:
    t6 = -2 * ONE_SQRT_3 * (ab_in->beta);
    t1 = (ab_in->alpha) + ONE_SQRT_3 * (ab_in->beta);

    uvw_out->v = (pwm_period - t6 - t1) / 2;
    uvw_out->w = uvw_out->v + t6;
    uvw_out->u = uvw_out->w + t1;
    break;
  }
}

// Performs an inverse park transform
// Given a vector in the rotating dq coordinate system, and the angle in radians
// with respect to the alpha axis, gives a vector in the alpha beta system
void foc_inverse_park(const float angle, const foc_dq_t *dq_in,
                      foc_ab_t *ab_out) {
  const float sin = sin(angle);
  const float cos = cos(angle);
  ab_out->alpha = (dq_in->d) * cos - (dq_in->q) * sin;
  ab_out->beta = (dq_in->d) * sin + (dq_in->q) * cos;
}
