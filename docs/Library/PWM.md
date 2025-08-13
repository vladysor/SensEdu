---
title: PWM
layout: default
parent: Library
math: mathjax
nav_order: 4
---

# PWM Peripheral
{: .fs-8 .fw-500 .no_toc}
---

PWM is cool.
{: .fw-500}

- TOC
{:toc}

## Errors

Main PWM error code is `0x50xx`. Find the way to display errors in your Arduino sketch [here]({% link Library/index.md %}#error-handling).

An overview of possible errors for timers:
* `0x5000`: No Errors

## Functions

### SensEdu_PWM_Init
Initializes PWM.

```c
void SensEdu_PWM_Init(uint8_t pwm_pin);
```

#### Parameters
{: .no_toc}
* `pwm_pin`: blank.

## Examples

### Generate_PWM

does things.

1. Include the SensEdu library and declare the PWM pin
2. ???

{: .NOTE}
???

```c
#include "SensEdu.h"

uint8_t pwm = D7;

void setup() {
    
}

void loop() {

}
```

## Developer Notes
TIM8 is used, move it to timers.md

[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
