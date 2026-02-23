#include "esp_log.h"
#include "foc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "inverter.h"
#include "sdkconfig.h"

#define GPIO_OUTPUT_U CONFIG_U_LEG_GPIO
#define GPIO_OUTPUT_V CONFIG_V_LEG_GPIO
#define GPIO_OUTPUT_W CONFIG_W_LEG_GPIO

#define MCPWM_TIMER_RESOLUTION 1 * 1000 * 1000 // 1 MHz
#define FOC_PWM_PERIOD 50                      // microseconds
#define INVETER_UPDATE_TASK_STACK_SIZE 4096
static const float FOC_ELECTRICAL_FREQUENCY = 0.33f; // Hz
static const float FOC_ELECTRICAL_AMPLITUDE = 25.0f; // Based on ticks of period
static const float ELECTRICAL_RADIANS_PER_SECOND =
    FOC_ELECTRICAL_FREQUENCY * TWO_PI;
static const float RADIANS_TRAVELLED_PER_PERIOD =
    ELECTRICAL_RADIANS_PER_SECOND /
    ((float)MCPWM_TIMER_RESOLUTION / ((float)FOC_PWM_PERIOD / 2));
static const char *TAG = "foc inverter";

static volatile float electrical_angle_radians = 0.0f;
static foc_dq_t dq = {0.0f, 0.0f};
static foc_ab_t ab = {0.0f, 0.0f};
static foc_uvw_t uvw = {0.0f, 0.0f, 0.0f};
static volatile uint16_t uvw_duty[3] = {0, 0, 0};

bool inverter_callback(mcpwm_timer_handle_t timer,
                       const mcpwm_timer_event_data_t *edata, void *user_ctx) {
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(((TaskHandle_t)user_ctx), &pxHigherPriorityTaskWoken);
  return pxHigherPriorityTaskWoken == pdTRUE;
}
void vInverterUpdate(void *pvParameters) {
  inverter_handle_t inverter = (inverter_handle_t)pvParameters;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    electrical_angle_radians += RADIANS_TRAVELLED_PER_PERIOD;
    if (electrical_angle_radians > TWO_PI) {
      electrical_angle_radians -= TWO_PI;
    }

    dq.d = FOC_ELECTRICAL_AMPLITUDE;
    foc_inverse_park(electrical_angle_radians, &dq, &ab);
    foc_svpwm_duty_calculate(&ab, &uvw, FOC_PWM_PERIOD);
    uvw_duty[0] = (FOC_PWM_PERIOD / 2) - (uint16_t)(uvw.u / 2);
    uvw_duty[1] = (FOC_PWM_PERIOD / 2) - (uint16_t)(uvw.v / 2);
    uvw_duty[2] = (FOC_PWM_PERIOD / 2) - (uint16_t)(uvw.w / 2);
    inverter_set_duty(inverter, uvw_duty[0], uvw_duty[1], uvw_duty[2]);
  }
}

void app_main(void) {
  vTaskDelay(pdMS_TO_TICKS(2000));
  ESP_LOGI(TAG, "Booting FOC System...");
  inverter_config_t cfg = {
      .timer_config =
          {
              .group_id = 0,
              .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
              .resolution_hz = MCPWM_TIMER_RESOLUTION,
              .count_mode =
                  MCPWM_TIMER_COUNT_MODE_UP_DOWN, // UP_DOWN => center align pwm
              .period_ticks = FOC_PWM_PERIOD,
          },
      .operator_config =
          {
              .group_id = 0,
          },
      .compare_config =
          {
              .flags.update_cmp_on_tez = true,
          },
      .gen_gpios = {GPIO_OUTPUT_U, GPIO_OUTPUT_V, GPIO_OUTPUT_W},
  };

  static inverter_handle_t inverter;
  inverter_create(&cfg, &inverter);
  ESP_LOGI(TAG, "Inverter created");

  TaskHandle_t xInverterHandle = NULL;
  xTaskCreate(vInverterUpdate, "inverter update",
              INVETER_UPDATE_TASK_STACK_SIZE, inverter, 2, &xInverterHandle);

  mcpwm_timer_event_callbacks_t callback_config = {
      .on_full = inverter_callback,
  };

  ESP_ERROR_CHECK(
      inverter_register_cbs(inverter, &callback_config, xInverterHandle));
  ESP_ERROR_CHECK(inverter_start(inverter, MCPWM_TIMER_START_NO_STOP));
  ESP_LOGI(TAG, "Inverter started succesfully");
  for (;;) {
    // ESP_LOGI(TAG, "--- Inverter Stats ---");
    // ESP_LOGI(TAG, "Angle: %.3f rad", electrical_angle_radians);
    // // uvw.u/v/w are the raw float results from SVPWM
    // ESP_LOGI(TAG, "SVPWM Floats: U:%.2f V:%.2f W:%.2f", uvw.u, uvw.v, uvw.w);
    // // uvw_duty are the actual ticks sent to MCPWM comparators
    // ESP_LOGI(TAG, "Final Ticks:  U:%u V:%u W:%u", uvw_duty[0], uvw_duty[1],
    //          uvw_duty[2]);
    //
    // vTaskDelay(pdMS_TO_TICKS(1000));
    //
    vTaskDelay(portMAX_DELAY);
  }

  ESP_ERROR_CHECK(inverter_start(inverter, MCPWM_TIMER_STOP_EMPTY));
  ESP_ERROR_CHECK(inverter_del(inverter));
}
