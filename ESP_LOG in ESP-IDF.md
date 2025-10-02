# Using ESP_LOG in ESP-IDF


## Log Macros

```c
#include "esp_log.h"

ESP_LOGE(tag, format, ...);   // Error
ESP_LOGW(tag, format, ...);   // Warning
ESP_LOGI(tag, format, ...);   // Info
ESP_LOGD(tag, format, ...);   // Debug
ESP_LOGV(tag, format, ...);   // Verbose
```

## Example
```c
#include "esp_log.h"

void app_main(void) {
    int led = 2;
    float voltage = 3.3;

    ESP_LOGI("MAIN", "Starting application...");
    ESP_LOGI("MAIN", "LED pin: %d", led);
    ESP_LOGW("SENSOR", "Low voltage detected: %.2f V", voltage);
    ESP_LOGE("NETWORK", "Connection failed!");
}
```

## Output
```
I (10) MAIN: Starting application...
I (11) MAIN: LED pin: 2
W (12) SENSOR: Low voltage detected: 3.30 V
E (13) NETWORK: Connection failed!
```
I (10) MAIN: Starting application...
I (11) MAIN: LED pin: 2
W (12) SENSOR: Low voltage detected: 3.30 V
E (13) NETWORK: Connection failed!

