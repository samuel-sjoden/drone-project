#include "driver/dedic_gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include <stdint.h>

#define GPIO_OUTPUT_IO_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_IO_2 CONFIG_GPIO_OUTPUT_2
#define TWO_PI 6.28318530718
#define PI 3.14159265359
#define PI_3 1.0471975512
#define ONE_VDC 0.0833333333333
// The representation of the states of the high side fets of the half bridge
static const uint32_t GPIO_BIT_MASK = 0b111;

static const uint32_t OUTPUT_STATES[8] = {0b000, 0b001, 0b011, 0b101,
                                          0b110, 0b100, 0b101, 0b111};

const int gpio_bundle[3] = {GPIO_OUTPUT_IO_0, GPIO_OUTPUT_IO_1,
                            GPIO_OUTPUT_IO_2};
dedic_gpio_bundle_handle_t gpios = NULL;
dedic_gpio_bundle_config_t gpios_config = {
    .gpio_array = gpio_bundle,
    .array_size = sizeof(gpio_bundle) / sizeof(gpio_bundle[0]),
    .flags =
        {
            .out_en = 1,
        },
};

const int CYCLE_PERIOD = 500000;
const float ELECTRICAL_FREQUENCY =
    0.033929f; //  in radians per microseonds. calc is from : degrees per second
               //  (27000KV * 12V * 360 degrees / 60s)
const uint32_t ELECTRICAL_PERIOD =
    (uint32_t)(TWO_PI / ELECTRICAL_FREQUENCY); // in microseconds
const uint8_t VREF_VS =
    1; // scaling for voltage applied to motor. Must be less than 1/sqrt(3) VDC

// global state of the stators being commutated. The zero state is the dead
// state of the motor
volatile uint8_t stator_sector = 1;

const static char *TAG = "LKD";
const uint32_t SYSTEM_TICK_RATE =
    10; // microseonds. based of of frequency of the hardware timer
volatile float phasor_angle = 0.0; // Wraps from 0 to 2PI
volatile uint64_t system_time =
    0.0; // a global system tick from a hardware timer. Count resets every 2PI
         // revoltion of phasor_angle.
static const uint8_t PWM_PERIOD =
    50; // Number of microseconds in overall 4 prame pwm

typedef struct {
  uint64_t frame_periods[4];
  uint8_t frame_states[4];
} pwm_state_t;

volatile pwm_state_t current_pwm_state = {.frame_periods = {20, 10, 10, 20},
                                          .frame_states = {0, 1, 3, 7}};
// TODO: this is not a good setup. this all runs in an ISR triggered by PWM
// edge. This should instead be cacluated on a system tick. so much refactoring
// to do
// TODO: look into MCPWM periferal devices instead of this software nightmare
void calculate_next_duty_cycle() {
  phasor_angle = system_time * ELECTRICAL_FREQUENCY;

  if (phasor_angle < PI_3) {
    stator_sector = 1;
  } else if (phasor_angle < 2 * PI_3) {
    stator_sector = 2;
  } else if (phasor_angle < 3 * PI_3) {
    stator_sector = 3;
  } else if (phasor_angle < 4 * PI_3) {
    stator_sector = 4;
  } else if (phasor_angle < 5 * PI_3) {
    stator_sector = 5;
  } else {
    stator_sector = 6;
  }

  // Calculate the projection of the phasor vector on the relative basis
  float active_1_projection =
      cosine(phasor_angle -
             PI_3 * (stator_sector -
                     1)); // TODO need to find / implement a cosine function.
                          // And choose if floating point is ok to use
  float active_2_projection = cosine(phasor_angle - PI_3 * (stator_sector));
  uint64_t period_active_1 =
      (uint64_t)(active_1_projection * PWM_PERIOD * VREF_VS * ONE_VDC);
  uint64_t period_active_2 =
      (uint64_t)(active_2_projection * PWM_PERIOD * VREF_VS * ONE_VDC);
  uint64_t non_active_duty = PWM_PERIOD - period_active_1 - period_active_2;

  current_pwm_state.frame_periods[0] = non_active_duty / 2;
  current_pwm_state.frame_periods[3] = non_active_duty / 2;
  current_pwm_state.frame_states[0] =
      ~current_pwm_state.frame_states[0] & 0b111;
  current_pwm_state.frame_states[3] =
      ~current_pwm_state.frame_states[3] & 0b111;

  // Determine the states and the order that they go in the PWM frame
  uint8_t basis_1, basis_2;
  if (stator_sector == 6) {
    basis_2 = 1;
  } else {
    basis_2 = stator_sector + 1;
  }
  basis_1 = stator_sector;

  // Determine if the first basis is more than 1 bit flip away from the first
  // state (hamming distance greater than 1). Each state transision should only
  // have one bit flip between them
  uint8_t xored_bases =
      (((uint8_t)OUTPUT_STATES[basis_1]) ^ current_pwm_state.frame_states[0]) &
      (0b111);
  // this is a trick to determine if the hamming distance is 1 or not 1
  if (xored_bases != 0 && (xored_bases & (xored_bases - 1)) == 0) {
    current_pwm_state.frame_states[1] = (uint8_t)OUTPUT_STATES[basis_1];
    current_pwm_state.frame_periods[1] = period_active_1;
    current_pwm_state.frame_states[2] = (uint8_t)OUTPUT_STATES[basis_2];
    current_pwm_state.frame_periods[2] = period_active_2;
  } else {
    // If the first basis is more than a bit flip away, the second basis should
    // go first
    current_pwm_state.frame_states[1] = (uint8_t)OUTPUT_STATES[basis_2];
    current_pwm_state.frame_periods[1] = period_active_2;
    current_pwm_state.frame_states[2] = (uint8_t)OUTPUT_STATES[basis_1];
    current_pwm_state.frame_periods[2] = period_active_1;
  }
}

// There are four pwm frames low - active - active - low. The next frame is
// calculated on the last frame to reduce latency of the calculation based on
// the phasor position
volatile uint8_t pwm_frame = 0;
static const uint8_t LAST_PWM_FRAME = 3;

IRAM_ATTR static bool timer_reload(gptimer_handle_t timer,
                                   const gptimer_alarm_event_data_t *edata,
                                   void *user_ctx) {
  uint64_t gpio_values = (uint64_t)current_pwm_state.frame_states[pwm_frame];
  dedic_gpio_bundle_write(gpios, GPIO_BIT_MASK, gpio_values);
  gptimer_alarm_config_t pwm_alarm_config = {
      .alarm_count = current_pwm_state.frame_periods[pwm_frame],
  };
  gptimer_set_alarm_action(timer, &pwm_alarm_config);
  pwm_frame++;
  if (pwm_frame > LAST_PWM_FRAME) {
    pwm_frame = 0;
  }
  if (pwm_frame == LAST_PWM_FRAME) {
    calculate_next_duty_cycle();
  }

  return false;
}
IRAM_ATTR static bool
global_tick_update(gptimer_handle_t timer,
                   const gptimer_alarm_event_data_t *edata, void *user_ctx) {
  system_time++;
  if (system_time >= ELECTRICAL_PERIOD) {
    system_time = 0;
  }
}
gptimer_handle_t software_pwm_timer = NULL;
gptimer_handle_t global_tick_timer = NULL;
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz =
        1 * 1000 *
        1000, // timer resolution set to 1MHz, 1 tick every microsecond
};

gptimer_alarm_config_t pwm_alarm_config = {
    .reload_count = 0,
    .alarm_count = CYCLE_PERIOD,
    .flags.auto_reload_on_alarm = false,
};

gptimer_alarm_config_t global_tick_alarm_config = {
    .reload_count = 0,
    .alarm_count = SYSTEM_TICK_RATE,
    .flags.auto_reload_on_alarm = true,
};
gptimer_event_callbacks_t callback = {
    .on_alarm = timer_reload,
};
gptimer_event_callbacks_t global_timer_callback = {
    .on_alarm = global_tick_update,
};

void app_main(void) {
  dedic_gpio_new_bundle(&gpios_config, &gpios);
  gptimer_new_timer(&timer_config, &software_pwm_timer);
  gptimer_new_timer(&timer_config, &global_tick_timer);
  gptimer_set_alarm_action(software_pwm_timer, &pwm_alarm_config);
  gptimer_set_alarm_action(global_tick_timer, &global_tick_alarm_config);
  gptimer_register_event_callbacks(software_pwm_timer, &callback, NULL);
  gptimer_register_event_callbacks(global_tick_timer, &global_timer_callback,
                                   NULL);
  gptimer_enable(software_pwm_timer);
  gptimer_enable(global_tick_timer);
  gptimer_start(software_pwm_timer);
  gptimer_start(global_tick_timer);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "%d", stator_sector);
  }
}
