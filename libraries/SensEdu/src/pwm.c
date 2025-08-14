#include "pwm.h"

#define PJ8     4U
#define PJ6     37U
#define PK0     48U
#define PI2     71U

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

static pin pwm_pin_ch1 = {GPIOJ, PJ8, RCC_AHB4ENR_GPIOJEN};
static pin pwm_pin_ch2 = {GPIOJ, PJ6, RCC_AHB4ENR_GPIOJEN};
static pin pwm_pin_ch3 = {GPIOK, PK0, RCC_AHB4ENR_GPIOKEN};
static pin pwm_pin_ch4 = {GPIOI, PI2, RCC_AHB4ENR_GPIOIEN};

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
static void assign_error(PWM_ERROR new_error);
static PWM_ERROR check_freq(uint16_t freq);
static PWM_ERROR check_duty_cycle(uint8_t duty_cycle);
static pin* identify_pin(uint8_t arduino_pin_idx);
static void pin_init(pin* pwm_pin);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
void SensEdu_PWM_Init(uint8_t arduino_pin_idx, uint16_t freq, uint8_t duty_cycle) {
    assign_error(check_freq(freq));
    assign_error(check_duty_cycle(duty_cycle));
    pin* pwm_pin = identify_pin(arduino_pin_idx);
    if (error != PWM_ERROR_NO_ERRORS) {
        return;
    }

    pin_init(pwm_pin);
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

static pin* identify_pin(uint8_t arduino_pin_idx) {
    if (arduino_pin_idx == pwm_pin_ch1.idx) {
        return &pwm_pin_ch1;
    } else if (arduino_pin_idx == pwm_pin_ch2.idx) {
        return &pwm_pin_ch2;
    } else if (arduino_pin_idx == pwm_pin_ch3.idx) {
        return &pwm_pin_ch3;
    } else if (arduino_pin_idx == pwm_pin_ch4.idx) {
        return &pwm_pin_ch4;
    }

    assign_error(PWM_ERROR_WRONG_PIN_SELECTED);
    return NULL;
}
//TODO: fix shift, you must use stm number, not arduino
static void pin_init(pin* pwm_pin) {
    // Clock
    SET_BIT(RCC->AHB4ENR, pwm_pin->clock_mask);

    // GPIO Speed (play around with oscilloscope and this setting)
    GPIO_TypeDef* gpio = pwm_pin->gpio;
    uint16_t shift = (pwm_pin->idx)*2;
    //MODIFY_REG(gpio->OSPEEDR, 0x3UL << shift, 0b11 << shift);
    MODIFY_REG(gpio->OSPEEDR, GPIO_OSPEEDR_OSPEED8, 0b11 << GPIO_OSPEEDR_OSPEED8_Pos);

    // Not pull up, pull-down
    //MODIFY_REG(gpio->PUPDR, 0x3UL << shift, 0b00 << shift);

    // Alternate Function Mode
    //MODIFY_REG(gpio->MODER, 0x3UL << shift, 0b10 << shift);
    MODIFY_REG(gpio->MODER, GPIO_MODER_MODE8, 0b10 << GPIO_MODER_MODE8_Pos);

    // Alternate Function Configuration (AF3 for TIM8)
    MODIFY_REG(gpio->AFR[1], GPIO_AFRH_AFSEL8, 3U << GPIO_AFRH_AFSEL8_Pos);
    /*
    shift = (pwm_pin->idx)*4;
    if (shift < 32U) {
        MODIFY_REG(gpio->AFR[0], 0xFUL << shift, 3U << shift);
    } else {
        shift = shift - 32;
        MODIFY_REG(gpio->AFR[1], 0xFUL << shift, 3U << shift);
    }*/
}