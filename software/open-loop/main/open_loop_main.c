#include "driver/gptimer.h" 
#include "driver/dedic_gpio.h"
#include "sdkconfig.h"
#include <stdint.h>


#define GPIO_OUTPUT_IO_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_IO_2 CONFIG_GPIO_OUTPUT_2
#define GPIO_OUTPUT_IO_3 CONFIG_GPIO_OUTPUT_3
#define GPIO_OUTPUT_IO_4 CONFIG_GPIO_OUTPUT_4
#define GPIO_OUTPUT_IO_5 CONFIG_GPIO_OUTPUT_5

// The representation of the states of the high side fets of the half bridge
static const uint8_t STATE_BITS = 3;
static const uint8_t STATE_FIELD_MASK =  ((1 << STATE_BITS) - 1);
static const uint32_t GPIO_BIT_MASK = (1 << (STATE_BITS * 2)) - 1;
static const uint32_t STATES_PACKED =
    (0b111u << (7 * STATE_BITS)) |
    (0b101u << (6 * STATE_BITS)) |
    (0b100u << (5 * STATE_BITS)) |
    (0b110u << (4 * STATE_BITS)) |
    (0b101u << (3 * STATE_BITS)) |
    (0b011u << (2 * STATE_BITS)) |
    (0b001u << (1 * STATE_BITS)) |
    (0b000u);

int gpio_bundle[6] = {GPIO_OUTPUT_IO_0, GPIO_OUTPUT_IO_1, GPIO_OUTPUT_IO_2, GPIO_OUTPUT_IO_3, GPIO_OUTPUT_IO_4, GPIO_OUTPUT_IO_5};
dedic_gpio_bundle_handle_t gpios = NULL;
dedic_gpio_bundle_config_t gpios_config = {
    .gpio_array = gpio_bundle,
    .array_size = sizeof(gpio_bundle) / sizeof(gpio_bundle[0]),
    .flags = {
        .out_en = 1,
    },
};

const int TIMER_RESOLUTION = 1 * 1000 * 1000; // timer resolution set to 1MHz
const int CYCLE_PERIOD = 100000;

// global state of the stators being commutated. The zero state is the dead state of the motor
volatile uint8_t GLOBAL_STATE = 6;

gptimer_handle_t state_timer = NULL;
gptimer_config_t timer_config = {
	.clk_src = GPTIMER_CLK_SRC_DEFAULT,
	.direction = GPTIMER_COUNT_UP,
	.resolution_hz = TIMER_RESOLUTION,
};


IRAM_ATTR static bool change_global_state(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void * user_ctx) {
	GLOBAL_STATE++;
	if (GLOBAL_STATE > 6) {
		GLOBAL_STATE = 1;
	} 

	uint8_t high_side = (STATES_PACKED >> (GLOBAL_STATE * STATE_BITS)) & STATE_FIELD_MASK;
	uint8_t low_side = (~high_side) & STATE_FIELD_MASK;
    uint32_t gpio_values = (high_side << STATE_BITS) | low_side;

	dedic_gpio_bundle_write(gpios, GPIO_BIT_MASK, gpio_values);
	return true;
}

gptimer_alarm_config_t alarm_config = {
	.reload_count = 0,
	.alarm_count = CYCLE_PERIOD,
	.flags.auto_reload_on_alarm = true,
};
gptimer_event_callbacks_t callback = {
	.on_alarm = change_global_state,
};

void app_main(void) {
	dedic_gpio_new_bundle(&gpios_config, &gpios);
	gptimer_new_timer(&timer_config, &state_timer);
	gptimer_set_alarm_action(state_timer, &alarm_config);
	gptimer_register_event_callbacks(state_timer, &callback, NULL);
	gptimer_enable(state_timer);
	for(;;) {

	}
}


