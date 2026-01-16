#include "driver/gptimer.h" 
#include "driver/gpio.h"
#include <stdint.h>
// The possible states the stator coils can be commutated with
const int STATES[7][3] = {
	{0,0,1},
	{0,1,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,0,1},
	{0,0,0}
};

const uint8_t NUM_POSITIONAL_STATES = 6;
const int CYCLE_PERIOD = 100000; // set to 1 microsecond, assuming timer resolution is 1MHz

// global state of the stators being commutated. The zero state is the dead state of the motor
volatile uint8_t GLOBAL_STATE = 6;

gptimer_handle_t state_timer = NULL;
// TODO: make another timer to throttle the PWM value
gptimer_handle_t modulation_timer = NULL;
gptimer_config_t timer_config = {
	.clk_src = GPTIMER_CLK_SRC_DEFAULT,
	.direction = GPTIMER_COUNT_UP,
	.resolution_hz = 1 * 1000 * 1000, // 1MHz resolution
};


gptimer_new_timer(&timer_config, &state_timer);

static void change_global_state(gptimer_handle_t timer, const gptimer_alarm_data_t *edata, void * user_ctx) {
	GLOBAL_STATE = (GLOBAL_STATE + 1) % NUM_POSITIONAL_STATES;
	return;
}

gptimer_alarm_config_t alarm_config = {
	.reload_count = 0,
	.alarm_count = CYCLE_PERIOD,
	// TODO: Do i need stdbool or is this defined in gptimer
	.flags.auto_reload_on_alarm = true,
};

gptimer_set_alarm_action(state_timer, &alarm_config);

gptimer_event_callbacks_t callback = {
	.on_alarm = change_global_state,
};
gptimer_register_event_callbacks(gptimer, &callback, NULL);
gptimer_enable(state_timer);

void app_main(void) {
	//TODO: enable and disable gpio writes based on the global state
}


