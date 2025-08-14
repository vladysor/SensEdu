#include "pwm.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    GPIO_TypeDef* gpio;
    uint8_t idx;
    uint32_t clock_mask;
} pin;

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static PWM_ERROR error = PWM_ERROR_NO_ERRORS;

static pin pwm_pin1;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
static void assign_error(PWM_ERROR new_error);
static PWM_ERROR check_pin(uint8_t arduino_pin_idx);
static PWM_ERROR check_freq(uint16_t freq);
static PWM_ERROR check_duty_cycle(uint8_t duty_cycle);
static void pin_init(pin* pwm_pin);
static uint32_t get_clock_mask(GPIO_TypeDef* gpio);
static void identify_pin(pin* pwm_pin, uint8_t arduino_pin_idx);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
void SensEdu_PWM_Init(uint8_t arduino_pin_idx, uint16_t freq, uint8_t duty_cycle) {
    assign_error(check_pin(arduino_pin_idx));
    assign_error(check_freq(freq));
    assign_error(check_duty_cycle(duty_cycle));
    if (error != PWM_ERROR_NO_ERRORS) {
        return;
    }

    identify_pin(&pwm_pin1, arduino_pin_idx);
    pin_init(&pwm_pin1);
    TIMER_PWMInit();
}

void SensEdu_PWM_Start(void) {
    TIMER_PWMEnable();
}

PWM_ERROR PWM_GetError(void) {
    return error;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
static void assign_error(PWM_ERROR new_error) {
    if (error == PWM_ERROR_NO_ERRORS) {
        error = new_error;
    }
}

static PWM_ERROR check_pin(uint8_t arduino_pin_idx) {
    if (arduino_pin_idx >= (2U) && arduino_pin_idx <= (7U)) {
        return PWM_ERROR_NO_ERRORS;
    }

    if (arduino_pin_idx >= (22U) && arduino_pin_idx <= (53U)) {
        return PWM_ERROR_NO_ERRORS;
    }

    return PWM_ERROR_WRONG_PIN_SELECTED;
}
//TODO
static PWM_ERROR check_freq(uint16_t freq)
{
    return PWM_ERROR_NO_ERRORS;
}
//TODO
static PWM_ERROR check_duty_cycle(uint8_t duty_cycle)
{
    return PWM_ERROR_NO_ERRORS;
}

static void pin_init(pin* pwm_pin) {
    
    GPIO_TypeDef* gpio = pwm_pin->gpio;

    // Clock
    SET_BIT(RCC->AHB4ENR, pwm_pin->clock_mask);
    
    // Output Mode
    uint16_t shift = (pwm_pin->idx)*2;
    MODIFY_REG(gpio->MODER, 0x3UL << shift, 0b01 << shift);

    // GPIO Speed (play around with oscilloscope and this setting)
    MODIFY_REG(gpio->OSPEEDR, 0x3UL << shift, 0b01 << shift);

    // Not pull up, pull-down
    MODIFY_REG(gpio->PUPDR, 0x3UL << shift, 0b00 << shift);

    // Output Register
    MODIFY_REG(gpio->ODR, 0x1UL << (pwm_pin->idx), 0b0 << (pwm_pin->idx));
}
// TODO
static void identify_pin(pin* pwm_pin, uint8_t arduino_pin_idx) {
    switch (arduino_pin_idx) {
        case 2U:
            pwm_pin->gpio = GPIOA;
            pwm_pin->idx = 3U;
            pwm_pin->clock_mask = RCC_AHB4ENR_GPIOAEN;
        default:
            assign_error(PWM_ERROR_WRONG_PIN_SELECTED);
    }
}

static uint32_t get_clock_mask(GPIO_TypeDef* gpio) {
    if (gpio == GPIOA) {
        return RCC_AHB4ENR_GPIOAEN;
    } else if (gpio == GPIOB) {
        return RCC_AHB4ENR_GPIOBEN;
    } else if (gpio == GPIOC) {
        return RCC_AHB4ENR_GPIOCEN;
    } else if (gpio == GPIOD) {
        return RCC_AHB4ENR_GPIODEN;
    } else if (gpio == GPIOE) {
        return RCC_AHB4ENR_GPIOEEN;
    } else if (gpio == GPIOF) {
        return RCC_AHB4ENR_GPIOFEN;
    } else if (gpio == GPIOG) {
        return RCC_AHB4ENR_GPIOGEN;
    } else if (gpio == GPIOH) {
        return RCC_AHB4ENR_GPIOHEN;
    } else if (gpio == GPIOI) {
        return RCC_AHB4ENR_GPIOIEN;
    } else if (gpio == GPIOJ) {
        return RCC_AHB4ENR_GPIOJEN;
    } else if (gpio == GPIOK) {
        return RCC_AHB4ENR_GPIOKEN;
    }
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

