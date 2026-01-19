#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_TIMER LEDC_TIMER_0
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_GPIO 18    // هر GPIO آزاد
#define PWM_FREQ 20000 // 20kHz (motor-safe)
#define PWM_RES LEDC_TIMER_10_BIT

static const char *TAG = "ACT_PWM";

void actuator_pwm_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = PWM_MODE,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ,
        .duty_resolution = PWM_RES,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num = PWM_GPIO,
        .speed_mode = PWM_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ch);

    ESP_LOGI(TAG, "PWM init done");
}

void actuator_set_power(uint8_t percent)
{
    ESP_LOGI(TAG, "Set Power: %d", percent);
    if (percent > 100)
        percent = 100;
    uint32_t duty = (percent * ((1 << PWM_RES) - 1)) / 100;
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}
void actuator_ramp_to(uint8_t target_percent)
{
    if (target_percent > 100)target_percent = 100;
    uint32_t target_duty = (target_percent * ((1 << PWM_RES) - 1)) / 100;

    uint32_t current_duty = ledc_get_duty(PWM_MODE, PWM_CHANNEL);
    int step = 25;
    int delay_ms = 10;
    
    //--------------
      if (current_duty < target_duty)
      {
          for (; current_duty <= target_duty; current_duty += step)
          {
              ledc_set_duty(PWM_MODE, PWM_CHANNEL, current_duty);
              ledc_update_duty(PWM_MODE, PWM_CHANNEL);
              vTaskDelay(pdMS_TO_TICKS(delay_ms));
          }
      }
      else if (current_duty > target_duty)
      {
          for (; current_duty >= target_duty; current_duty -= step)
          {
              ledc_set_duty(PWM_MODE, PWM_CHANNEL, current_duty);
              ledc_update_duty(PWM_MODE, PWM_CHANNEL);
              vTaskDelay(pdMS_TO_TICKS(delay_ms));
              if (current_duty < step) break; // جلوگیری از underflow
          }
      }
      // اطمینان از رسیدن به مقدار نهایی
      if (current_duty != target_duty) {
         ledc_set_duty(PWM_MODE, PWM_CHANNEL, target_duty);
         ledc_update_duty(PWM_MODE, PWM_CHANNEL);
      }

    //----------

    /*TODO: ERROR  uint8_t current = ledc_get_duty(PWM_MODE, PWM_CHANNEL);

    if (current < target)
    {
        for (; current <= target; current += 5)
        {
            actuator_set_power(current);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    else
    {
        for (; current >= target; current -= 5)
        {
            actuator_set_power(current);
            vTaskDelay(pdMS_TO_TICKS(20));
            if (current < 5) break; // جلوگیری از underflow
        }
    }*/
}
