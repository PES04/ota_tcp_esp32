#include "sys_feedback.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define FEEDBACK_GPIO       (GPIO_NUM_2)
#define PINNED_CORE         (1)
#define FEEDBACK_QUEUE_LEN  (5)

static const char *tag = "SYS_FEEDBACK";

static QueueHandle_t feedback_queue;

static void feedback_task(void *arg);

types_error_code_e sys_feedback_init(void)
{

  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << FEEDBACK_GPIO),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };

  if (gpio_config(&io_conf) != ESP_OK)
  {
    ESP_LOGE(tag, "----- Failed to configure GPIO -----");
    return ERR_CODE_FAIL;
  }

  feedback_queue = xQueueCreate(FEEDBACK_QUEUE_LEN, sizeof(sys_feedback_mode_t));

  if (feedback_queue == NULL)
  {
    ESP_LOGE(tag, "----- Failed to create feedback queue -----");
    return ERR_CODE_FAIL;
  }

  BaseType_t result = xTaskCreatePinnedToCore(feedback_task, "sys_feedback_task", 2048, NULL, 1, NULL, PINNED_CORE);
  if (result != pdPASS)
  {
    ESP_LOGE(tag, "----- Failed to create feedback task -----");
    return ERR_CODE_FAIL;
  }

  ESP_LOGI(tag, "----- Initialized successfully -----");
  return ERR_CODE_OK;
}

static void feedback_task(void *arg)
{
  uint8_t i = 0;
  
  sys_feedback_mode_t current_mode = SYS_FEEDBACK_MODE_NORMAL;
  gpio_set_level(FEEDBACK_GPIO, 1);

  while (1)
  {
    if (xQueueReceive(feedback_queue, &current_mode, 0) == pdTRUE)
    {
      ESP_LOGI(tag, "----- Feedback mode updated: %s -----", current_mode == SYS_FEEDBACK_MODE_UPDATE ? "UPDATE" : "NORMAL");

      if(current_mode == SYS_FEEDBACK_MODE_NORMAL) {
        gpio_set_level(FEEDBACK_GPIO, 1);
      }
    }

    if (current_mode == SYS_FEEDBACK_MODE_UPDATE)
    {
      gpio_set_level(FEEDBACK_GPIO, ++i%2);
    }

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void sys_feedback_set_update_mode(void)
{
  sys_feedback_mode_t mode = SYS_FEEDBACK_MODE_UPDATE;
  if (xQueueSend(feedback_queue, &mode, 0) != pdTRUE)
  {
    ESP_LOGW(tag, "----- Failed to queue update mode -----");
  }
}

void sys_feedback_set_normal_mode(void)
{
  sys_feedback_mode_t mode = SYS_FEEDBACK_MODE_NORMAL;
  if (xQueueSend(feedback_queue, &mode, 0) != pdTRUE)
  {
    ESP_LOGW(tag, "----- Failed to queue normal mode -----");
  }
}

void sys_feedback_whoiam(const uint8_t major, const uint8_t minor, const uint8_t patch)
{
  const char *build_time = __TIME__;
  const char *build_date = __DATE__;
  const char *idf_ver = esp_get_idf_version();

  ESP_LOGI(tag, "==== SYS_FEEDBACK WHOIAM ====");
  ESP_LOGI(tag, "Component version: v%u.%u.%u", major, minor, patch);
  ESP_LOGI(tag, "ESP-IDF version  : %s", idf_ver);
  ESP_LOGI(tag, "Build timestamp  : %s %s", build_date, build_time);
  ESP_LOGI(tag, "==============================");
}
