---
title: Timers
layout: default
parent: Library
math: mathjax
nav_order: 1
---

# Timers Peripheral
{: .fs-8 .fw-500 .no_toc}
---

Timers allow precise event scheduling, such as creating delays or setting sampling rates for peripherals.
{: .fw-500}

- TOC
{:toc}

## Errors

The main timer error code prefix is `0x10xx`. Find the way to display errors in your Arduino sketch [here]({% link Library/index.md %}#error-handling).

An overview of possible errors for timers:
* `0x1000`: No Errors
* `0x1001`: Unexpected delay value. Minimum is 2ns.
* `0x1002`: Unexpected ADC frequency. Maximum possible is 120MHz, refer to [these calculations]({% link Library/Timers.md %}#frequency-settings) for more details
* `0x1003`: Unexpected DAC frequency. Maximum possible is 60MHz, refer to [these calculations]({% link Library/Timers.md %}#frequency-settings) for more details
* `0x1004`: TIM8 initialization attempt while TIM8 is running. Configuration is possible only for disabled timer
* `0x1005`: TIM8 Unexpected CCR channel. Possible options are: `CCR1`, `CCR2`, `CCR3` or `CCR4`

An overview of critical errors. They shouldn't happen in normal user case and indicate some problems in library code:
* `0x10A0`: Timer frequency calculations failed

## Functions

### SensEdu_TIMER_DelayInit
Initializes the timer used for microsecond delays. Call it once in the setup before using delays.

```c
void SensEdu_TIMER_DelayInit();
```

### SensEdu_TIMER_Delay_us

Pauses program execution for a specified duration in microseconds (blocking delay).

```c
void SensEdu_TIMER_Delay_us(uint32_t delay_us);
```

#### Parameters
{: .no_toc}
* `delay_us`: Delay duration in microseconds. Maximum: $$4,294,967,295 \text{us}$$.

### SensEdu_TIMER_Delay_ns

Pauses program execution for a specified duration in nanoseconds (blocking delay).

```c
void SensEdu_TIMER_Delay_ns(uint32_t delay_ns);
```

#### Parameters
{: .no_toc}
* `delay_ns`: Delay duration in nanoseconds. Maximum: $$4,294,967,295 \text{ns}$$.

#### Notes
{: .no_toc}
* The lowest possible timer resolution on STM32H747 MCU is ~$$4.17\text{ns}$$. Calculated this way: $$\text{tick} = 1/240\text{MHz} * 10^9 \approx4.17\text{ns}$$. Then, possible delays are multiples of the tick, approximately: $$4\text{ns}$$, $$8\text{ns}$$, $$13\text{ns}$$, $$17\text{ns}$$, and so on.
* Delays shorter than $$250\text{ns}$$ are generally not possible, due to set frequencies being close to the system clock frequency of the MCU. These delays are overpowered by the software overhead, due to the software execution itself taking around $$60-120$$ CPU cycles. This corresponds to an additional delay of ~$$125-250\text{ns}$$ on a $$480\text{MHz}$$ core.
* For `delay_ns` $$\geq$$ $$1\text{ms}$$, the function internally switches to `SensEdu_TIMER_Delay_us()`, to avoid potential 32-bit math overflow for `NS_TO_TICKS(ns)` macro.

## Examples

### Blink_Delay

Blinks a LED using microsecond delays.

1. Include the SensEdu library and declare the LED pin (`LED_BUILTIN` in this case)
2. Initialize the timer with `SensEdu_TIMER_DelayInit()`
3. Configure the LED pin as an `OUTPUT` using `pinMode()`
4. Use alternating calls to `SensEdu_TIMER_Delay_us()` and `digitalWrite()` in the main loop to toggle the LED state.

{: .NOTE}
On Arduino GIGA R1 the built-in LED is active-low: `LOW` = on, `HIGH` = off.

```c
#include "SensEdu.h"

uint8_t led = LED_BUILTIN;

void setup() {
    SensEdu_TIMER_DelayInit();

    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);
}

void loop() {
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, HIGH);
    SensEdu_TIMER_Delay_us(500000);
    digitalWrite(led, LOW);
}
```

## Developer Notes

### Timer occupation

Peripheral timers (ADC/DAC) are hidden, automatically configured and require no user involvement.

Timer allocation:
* **TIM1**: ADC sampling
* **TIM2**: Delays 
* **TIM4**: DAC sampling
* **TIM8**: PWM

{: .WARNING }
Avoid reusing occupied timers. Refer to [STM32H747 Reference Manual] to find free available timers. Be aware, future updates will assign dedicated timers to each ADC/DAC separately, which may occupy your custom timer.

### Frequency settings

Timer frequency is dependent on 3 parameters:
* Clock Frequency $$(CLK)$$
* Prescaler $$(PSC)$$: CLK divider
* Period $$(ARR)$$: Register containing the value up to which the count proceeds

$$TIM_{freq} = \frac{CLK}{PSC + 1} * \frac{1}{ARR + 1}$$

The CLK signal is derived from the APB1 and APB2 Timer Clocks, each running at 240MHz.

![]({{site.baseurl}}/assets/images/TIM_CLK.png)

Prescaler is set to its minimum value to achieve the finest frequency adjustments, resulting in a step size of $$\frac{1}{120\text{MHz}} \approx 8.33\text{ns}$$.

| ARR | Period | Frequency |
|:----|:-------|:----------|
| 1   | 16.7ns | 60MHz     |
| 2   | 25.0ns | 40MHz     |
| 3   | 33.3ns | 30MHz     |
| 4   | 41.7ns | 24MHz     |
| 5   | 50.0ns | 20MHz     |

{: .NOTE }
When a user specifies a frequency for a DAC or ADC, the target value is automatically rounded to the nearest achievable frequency dictated by the timer's step. The lower target frequency, the higher the achievable precision.

[STM32H747 Reference Manual]: https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
