#include "inverter.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "inverter";

esp_err_t inverter_create(const inverter_config_t *config,
                          inverter_handle_t *inverter_out) {
  if (config == NULL || inverter_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  esp_err_t ret;
  inverter_handle_t *inverter_dev = calloc(1, sizeof(inverter_handle_t));

  if (!inverter_dev) {
    ESP_LOGE(TAG, "no memory");
    return ESP_ERR_NO_MEM;
  }

  ESP_GOTO_ON_ERROR(
      mcpwm_new_timer(&config->timer_config, &inverter_dev->timer), err, TAG,
      "Create MCPWM timer failed");

  for (int i = 0; i < 3; i++) {
    ESP_GOTO_ON_ERROR(mcpwm_new_operator(&config->operator_config,
                                         &inverter_dev->operators[i]),
                      err, TAG, "Create MCPWM operator failed");
    ESP_GOTO_ON_ERROR(mcpwm_operator_connect_timer(inverter_dev->operators[i],
                                                   inverter_dev->timer),
                      err, TAG, "Connect operators to the same timer failed");
  }
  for (int i = 0; i < 3; i++) {
    ESP_GOTO_ON_ERROR(mcpwm_new_comparator(inverter_dev->operators[i],
                                           &config->compare_config,
                                           &inverter_dev->comparators[i]),
                      err, TAG, "Create comparators failed");
    ESP_GOTO_ON_ERROR(
        mcpwm_comparator_set_compare_value(inverter_dev->comparators[i], 0),
        err, TAG, "Set comparators failed");
  }
  mcpwm_generator_config_t gen_config = {};
  for (int i = 0; i < 3; i++) {
    gen_config.gen_gpio_num = config->gen_gpios[i];
    ESP_GOTO_ON_ERROR(mcpwm_new_generator(inverter_dev->operators[i],
                                          &gen_config,
                                          &inverter_dev->generators[i]),
                      err, TAG, "Create PWM generator pin %d failed",
                      gen_config.gen_gpio_num);
  }
  for (int i = 0; i < 3; i++) {
    ESP_GOTO_ON_ERROR(
        mcpwm_generator_set_actions_on_compare_event(
            inverter_dev->generators[i],
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                           inverter_dev->comparators[i],
                                           MCPWM_GEN_ACTION_LOW),
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN,
                                           inverter_dev->comparators[i],
                                           MCPWM_GEN_ACTION_HIGH),
            MCPWM_GEN_COMPARE_EVENT_ACTION_END()),
        err, TAG, "Set generator actions failed");
  }
  inverter_out = inverter_dev;
  return ESP_OK;
err:
  free(inverter_dev);
  return ret;
}

esp_err_t inverter_register_cbs(inverter_handle_t *handle,
                                const mcpwm_timer_event_callbacks_t *event,
                                void *user_ctx) {
  ESP_RETURN_ON_ERROR(
      mcpwm_timer_register_event_callbacks(handle->timer, event, user_ctx), TAG,
      "register callbacks failed");
  return ESP_OK;
}

esp_err_t inverter_start(inverter_handle_t *handle,
                         mcpwm_timer_start_stop_cmd_t command) {
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
  if ((command != MCPWM_TIMER_STOP_EMPTY) &&
      (command != MCPWM_TIMER_STOP_FULL)) {
    ESP_RETURN_ON_ERROR(mcpwm_timer_enable(handle->timer), TAG,
                        "mcpwm timer enable failed");
  }
  ESP_RETURN_ON_ERROR(mcpwm_timer_start_stop(handle->timer, command), TAG,
                      "mcpwm timer start failed");
  return ESP_OK;
}

esp_err_t inverter_set_duty(inverter_handle_t *handle, uint16_t u, uint16_t v,
                            uint16_t w) {
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

  ESP_RETURN_ON_ERROR(
      mcpwm_comparator_set_compare_value(handle->comparators[0], u), TAG,
      "set duty failed");
  ESP_RETURN_ON_ERROR(
      mcpwm_comparator_set_compare_value(handle->comparators[1], v), TAG,
      "set duty failed");
  ESP_RETURN_ON_ERROR(
      mcpwm_comparator_set_compare_value(handle->comparators[2], w), TAG,
      "set duty failed");
  return ESP_OK;
}

esp_err_t inverter_del(inverter_handle_t *handle) {
  ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid argument");

  ESP_RETURN_ON_ERROR(mcpwm_timer_disable(handle->timer), TAG,
                      "mcpwm timer disable failed");
  for (int i = 0; i < 3; i++) {
    ESP_RETURN_ON_ERROR(mcpwm_del_generator(handle->generators[i]), TAG,
                        "free mcpwm positive generator failed");
    ESP_RETURN_ON_ERROR(mcpwm_del_comparator(handle->comparators[i]), TAG,
                        "free mcpwm comparator failed");
    ESP_RETURN_ON_ERROR(mcpwm_del_operator(handle->operators[i]), TAG,
                        "free mcpwm operator failed");
  }
  ESP_RETURN_ON_ERROR(mcpwm_del_timer(handle->timer), TAG,
                      "free mcpwm timer failed");
  free(handle);
  return ESP_OK;
}
