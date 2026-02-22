#include "foc.h"
#include "inverter.h"
#include "sdkconfig.h"

#define GPIO_OUTPUT_U CONFIG_U_LEG_GPIO
#define GPIO_OUTPUT_V CONFIG_V_LEG_GPIO
#define GPIO_OUTPUT_W CONFIG_W_LEG_GPIO

#define MCPWM_TIMER_RESOLUTION 1 * 1000 * 1000 // 1 MHz
#define FOC_PWM_PERIOD                                                         \
  50 // microseconds
     //
// static const uint8_t FOC_ELECTRICAL_FREQUENCY = 50; // Hz
// static const uint8_t FOC_ELECTRICAL_AMPLITUDE = 10; // Based on
// FOC_PWM_PERIOD static const char *TAG = "foc inverter";

void app_main(void) {
  // inverter_config_t cfg = {
  //     .timer_config =
  //         {
  //             .group_id = 0,
  //             .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
  //             .resolution_hz = MCPWM_TIMER_RESOLUTION,
  //             .count_mode =
  //                 MCPWM_TIMER_COUNT_MODE_UP_DOWN, // UP_DOWN mode will
  //                 generate
  //                                                 // center align pwm wave,
  //                                                 // which can reduce MOSFET
  //                                                 // switch times on same
  //                                                 // effect, extend life
  //             .period_ticks = FOC_PWM_PERIOD,
  //         },
  //     .operator_config =
  //         {
  //             .group_id = 0,
  //         },
  //     .compare_config =
  //         {
  //             .flags.update_cmp_on_tez = true,
  //         },
  //     .gen_gpios = {GPIO_OUTPUT_U, GPIO_OUTPUT_V, GPIO_OUTPUT_W},
  // };
}
