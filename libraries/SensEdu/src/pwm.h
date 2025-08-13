#ifndef __PWM_H__
#define __PWM_H__


#include "SensEdu.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PWM_ERROR_NO_ERRORS = 0x00,
    PWM_ERROR_TIM4_BAD_SET_FREQUENCY = 0x01
} PWM_ERROR;

void SensEdu_PWM_Init(uint8_t pin);

PWM_ERROR PWM_GetError(void);


#ifdef __cplusplus
}
#endif

#endif // __PWM_H__
