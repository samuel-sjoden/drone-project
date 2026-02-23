#ifndef INVERTER_H_
#define INVERTER_H_
#include "driver/mcpwm_prelude.h"
#include "stdio.h"

typedef struct inverter_config {
  mcpwm_timer_config_t timer_config;        // pwm timer and timing config
  mcpwm_operator_config_t operator_config;  // mcpwm operator config
  mcpwm_comparator_config_t compare_config; // mcpwm comparator config
  int gen_gpios[3];                         // 3 GPIO pins for generator config
} inverter_config_t;

typedef struct inverter_t {
  mcpwm_timer_handle_t timer;
  mcpwm_oper_handle_t operators[3];
  mcpwm_cmpr_handle_t comparators[3];
  mcpwm_gen_handle_t generators[3];
} inverter_t;

typedef struct inverter_t *inverter_handle_t;

esp_err_t inverter_create(const inverter_config_t *config,
                          inverter_handle_t *inverter_out);

esp_err_t inverter_register_cbs(inverter_handle_t handle,
                                const mcpwm_timer_event_callbacks_t *event,
                                void *user_ctx);

esp_err_t inverter_start(inverter_handle_t handle,
                         mcpwm_timer_start_stop_cmd_t command);

esp_err_t inverter_set_duty(inverter_handle_t handle, uint16_t u, uint16_t v,
                            uint16_t w);

esp_err_t inverter_del(inverter_handle_t handle);
#endif // INVERTER_H_
