#include <Arduino.h>
#include <rheoscape.hpp>

using namespace rheoscape;
using namespace rheoscape::helpers;
using namespace rheoscape::operators;
using namespace rheoscape::sources::arduino;
using namespace rheoscape::sinks::arduino;

#define encoder_a_pin 8
#define encoder_b_pin 9
const int pwm_pin = 27;

static pull_fn pull_change_pwm_state;
static pull_fn pull_set_pwm_pin;

void setup() {
    auto encoder = quadrature_encode(digital_pin_interrupt_source<encoder_a_pin, encoder_b_pin>(INPUT_PULLUP));
    static MemoryState pwm_state(0);
    pull_change_pwm_state = make_state_editor(encoder, pwm_state, [](QuadratureEncodeDirection dir, int pwm) {
        switch (dir) {
            case QuadratureEncodeDirection::Clockwise:
                return pwm <= 90 ? pwm + 10 : pwm;
            case QuadratureEncodeDirection::CounterClockwise:
                return pwm >= 10 ? pwm - 10 : pwm;
        }
        return pwm;
    });
    pull_set_pwm_pin = pwm_state.get_source_fn()
        | map([](int v) { return v * 1023 / 100; })
        | analog_pin_sink(pwm_pin);
}

void loop() {
    pull_change_pwm_state();
    pull_set_pwm_pin();
}
