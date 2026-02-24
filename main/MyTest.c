#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"    


#define PWM_TIMER LEDC_TIMER_0
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_GPIO 18          // هر GPIO آزاد
#define PWM_FREQ 20000       // 20kHz (motor-safe)
#define PWM_RES LEDC_TIMER_10_BIT

static const char *TAG_MyTest = "ACT_PWM";

void actuator_pwm_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = PWM_MODE,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ,
        .duty_resolution = PWM_RES,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num = PWM_GPIO,
        .speed_mode = PWM_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch);

    ESP_LOGI(TAG_MyTest, "PWM init done");
}

void actuator_set_power(uint8_t percent)
{
   // ESP_LOGI(TAG_MyTest,"Set Power: %d",percent);
    if (percent > 100) percent = 100;
    uint32_t duty = (percent * ((1 << PWM_RES) - 1)) / 100;
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}
void actuator_ramp_to(uint8_t target)
{
    uint32_t duty_current_val = ledc_get_duty(PWM_MODE, PWM_CHANNEL);
    uint32_t duty_max = (1 << PWM_RES) - 1; // حداکثر مقدار Duty (1023 برای 10 بیت)
    uint8_t current_percent = (duty_current_val * 100) / duty_max;
    uint8_t target_percent = target;
    uint8_t step = 2; // گام کوچکتر برای مشاهده بهتر (قبلاً 5 بود)
    int delay_ms = 100; // تأخیر افزایش یافته (قبلاً 20 بود)

    //ESP_LOGI(TAG_MyTest,"Ramp to: %d%%, starting from %d%%", target_percent, current_percent);

    if (current_percent < target_percent)
    {
        for (; current_percent <= target_percent; current_percent += step)
        {
            actuator_set_power(current_percent);
            vTaskDelay(pdMS_TO_TICKS(delay_ms)); // ⬅️ تأخیر 100 میلی‌ثانیه‌ای
        }
    }
    else
    {
        for (; current_percent >= target_percent; current_percent -= step)
        {
            actuator_set_power(current_percent);
            vTaskDelay(pdMS_TO_TICKS(delay_ms)); // ⬅️ تأخیر 100 میلی‌ثانیه‌ای
            if (current_percent < step) break; // برای جلوگیری از رفتن به زیر صفر
        }
    }
    // اطمینان از رسیدن دقیق به هدف نهایی
    actuator_set_power(target_percent);
    //ESP_LOGI(TAG_MyTest,"Ramp finished.");
}


void actuator_shutdown(void)
{
    ESP_LOGW(TAG_MyTest, "Actuator SHUTDOWN forced.");
    ledc_set_duty(PWM_MODE, PWM_CHANNEL, 0);
    ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}