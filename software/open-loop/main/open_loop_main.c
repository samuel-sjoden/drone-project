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
static const uint8_t STATE_BITS = 3;
static const uint8_t STATE_FIELD_MASK = ((1 << (STATE_BITS + 1)) - 1);
static const uint32_t GPIO_BIT_MASK = (1 << (STATE_BITS + 1)) - 1;
static const uint32_t STATES_PACKED =
    (0b111u << (7 * STATE_BITS)) | (0b101u << (6 * STATE_BITS)) |
    (0b100u << (5 * STATE_BITS)) | (0b110u << (4 * STATE_BITS)) |
    (0b101u << (3 * STATE_BITS)) | (0b011u << (2 * STATE_BITS)) |
    (0b001u << (1 * STATE_BITS)) | (0b000u);

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
const uint32_t ELECTRICAL_FREQUENCY =
    194400; // degrees per second (27000KV * 12V * 360 degrees / 60s) TODO:
            // convert this to radians per second
const uint8_t VREF_VS =
    1; // scaling for voltage applied to motor. Must be less than 1/sqrt(3) VDC

// global state of the stators being commutated. The zero state is the dead
// state of the motor
volatile uint8_t stator_sector = 1;

const static char *TAG = "LKD";
volatile float phasor_angle = 0.0; // Wraps from 0 to 2PI
static const uint8_t PWM_PERIOD =
    50; // Number of microseconds in overall 4 prame pwm

typedef struct {
  uint64_t frame_periods[4];
  uint8_t frame_states[4];
} pwm_state_t;

volatile pwm_state_t current_pwm_state = {.frame_periods = {20, 10, 10, 20},
                                          .frame_states = {0, 1, 3, 7}};
void calculate_next_duty_cycle() {
  phasor_angle +=
      system_time *
      ELECTRICAL_FREQUENCY; // TODO: need a way to get system time either in
                            // seconds or someother way that
                            // ELECTRICAL_FREQUENCY should be adjusted
  if (phasor_angle > TWO_PI) {
    phasor_angle -= TWO_PI;
  }

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
  // the first state should be at least one bit flip away from rest state
}
// There are four pwm frames low - active - active - low. The next frame is
// calculated on the last frame to reduce latency of the calculation based on
// the phasor position
volatile uint8_t pwm_frame = 0;
static const uint8_t LAST_PWM_FRAME = 3;

IRAM_ATTR static bool timer_reload(gptimer_handle_t timer,
                                   const gptimer_alarm_event_data_t *edata,
                                   void *user_ctx) {
  uint32_t gpio_values =
      (STATES_PACKED >> (stator_sector * STATE_BITS)) & STATE_FIELD_MASK;
  dedic_gpio_bundle_write(gpios, GPIO_BIT_MASK, gpio_values);
  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 100, // TODO: Set this to be the reloaded value
  };
  gptimer_set_alarm_action(timer, &alarm_config);
  pwm_frame++;
  if (pwm_frame > LAST_PWM_FRAME) {
    pwm_frame = 0;
  }
  if (pwm_frame == LAST_PWM_FRAME) {
    calculate_next_duty_cycle();
  }

  return false;
}

gptimer_handle_t software_pwm_timer = NULL;
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz =
        1 * 1000 *
        1000, // timer resolution set to 1MHz, 1 tick every microsecond
};

gptimer_alarm_config_t alarm_config = {
    .reload_count = 0,
    .alarm_count = CYCLE_PERIOD,
    .flags.auto_reload_on_alarm = false,
};
gptimer_event_callbacks_t callback = {
    .on_alarm = timer_reload,
};

void app_main(void) {
  dedic_gpio_new_bundle(&gpios_config, &gpios);
  gptimer_new_timer(&timer_config, &software_pwm_timer);
  gptimer_set_alarm_action(software_pwm_timer, &alarm_config);
  gptimer_register_event_callbacks(software_pwm_timer, &callback, NULL);
  gptimer_enable(software_pwm_timer);
  gptimer_start(software_pwm_timer);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "%d", stator_sector);
  }
}
