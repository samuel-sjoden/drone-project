#include "driver/gptimer.h" 
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdint.h>


#define GPIO_OUTPUT_IO_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_IO_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_IO_2 CONFIG_GPIO_OUTPUT_2
#define GPIO_OUTPUT_IO_3 CONFIG_GPIO_OUTPUT_3
#define GPIO_OUTPUT_IO_4 CONFIG_GPIO_OUTPUT_4
#define GPIO_OUTPUT_IO_5 CONFIG_GPIO_OUTPUT_5

int gpio_bundle[6] = {GPIO_OUTPUT_IO_0, GPIO_OUTPUT_IO_1, GPIO_OUTPUT_IO_2, GPIO_OUTPUT_IO_3, GPIO_OUTPUT_IO_4, GPIO_OUTPUT_IO_5};
dedic_gpio_bundle_handle_t gpios = NULL;
dedic_gpio_bundle_config_t gpios_config = {
    .gpio_array = gpio_bundle,
    .array_size = sizeof(gpio_bundle) / sizeof(gpio_bundle[0]),
    .flags = {
        .out_en = 1,
    },
};


// The possible states the stator coils can be commutated with
// The representaion is of the high side fets 0bABC
// Each high side fet has a corresponding low side fet which should always have the opposite
// position as the high side fets
// TODO: this can just be a 32 bit integer with these chunks in them
const uint8_t STATES[8] = {
	0b000,
	0b001,
	0b011,
	0b101,
	0b110,
	0b100,
	0b101,
	0b111
};
const uint8_t NUM_POSITIONAL_STATES = 6;

const uint16_t DC_LINK_MILIV = 12000;
const uint16_t PHASOR_MAGNITUDE_MILIV = 12000;
// const uint8_t NUM_MECHANICAL_POLES = 4;
// volatile uint16_t MECHANICAL_ANGULAR_FREQ = 0;
// volatile uint16_t ELECTRICAL_ANGLE_DEG = 0;
// volatile uint16_t ELECTRICAL_ANGULAR_FREQ_DEG = NUM_MECHANICAL_POLES * MECHANICAL_ANGULAR_FREQ;

const int TIMER_RESOLUTION = 1 * 1000 * 1000; // timer resolution set to 1MHz
// TODO: this will eventually be replaced with the desired speed (the desired electrical freq)
const int CYCLE_PERIOD = 100000;

// global state of the stators being commutated. The zero state is the dead state of the motor
volatile uint8_t GLOBAL_STATE = 6;


gptimer_handle_t state_timer = NULL;
gptimer_config_t timer_config = {
	.clk_src = GPTIMER_CLK_SRC_DEFAULT,
	.direction = GPTIMER_COUNT_UP,
	.resolution_hz = TIMER_RESOLUTION,
};


IRAM_ATTR static void change_global_state(gptimer_handle_t timer, const gptimer_alarm_data_t *edata, void * user_ctx) {
	uint8_t prev_state = GLOBAL_STATE;
	GLOBAL_STATE = GLOBAL_STATE % NUM_POSITIONAL_STATES + 1;
	uint8_t high_side = STATES[GLOBAL_STATE];
	uint8_t low_side = (~high_side) & 0b111;
    uint32_t gpio_values = (high_side << 3) | low_side;
	dedic_gpio_bundle_write(gpios, 0b111111, gpio_values);
	return;
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


