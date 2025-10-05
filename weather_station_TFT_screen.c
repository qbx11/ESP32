#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "lv_conf.h"

#define TAG "MAIN"

// --- ADC CONFIG ---
#define ADC_CHANNEL   ADC1_CHANNEL_6   // GPIO34
#define ADC_ATTEN     ADC_ATTEN_DB_0
#define ADC_WIDTH     ADC_WIDTH_BIT_12
#define DEFAULT_VREF  1100



// --- SPI & LCD CONFIG ---
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_DC   21
#define PIN_NUM_RST  22
#define PIN_NUM_BCKL 19

#define LCD_H_RES 240
#define LCD_V_RES 320

static esp_adc_cal_characteristics_t adc_chars;
static lv_obj_t *label;
static lv_color_t buf1[LCD_H_RES * 40];  // Bufor globalny zamiast lokalnego

// --- LVGL TICK TIMER (zamiast taska) ---
static void lv_tick_timer_cb(void *arg)
{
    lv_tick_inc(2);
}

// --- FLUSH CALLBACK ---
static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
    esp_lcd_panel_handle_t panel_handle =
        (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);

    esp_lcd_panel_draw_bitmap(panel_handle,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              color_p);

    lv_display_flush_ready(disp);
}

// --- GUI TASK ---
void guiTask(void *pvParameter)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)pvParameter;
    
    ESP_LOGI(TAG, "GUI Task started");
    
    lv_init();

    // --- LVGL DISPLAY ---
    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_set_flush_cb(disp, my_flush_cb);

    // Wyczyść bufor
    memset(buf1, 0xFF, sizeof(buf1)); // Wypełnij białym
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // --- LABEL ---
    // Ustaw białe tło dla całego ekranu
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    
    label = lv_label_create(screen);
    lv_label_set_text(label, "Initializing...");
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN); // Czarny tekst
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN); // Użyj czcionki 14px
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(label, LCD_H_RES - 20);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    ESP_LOGI(TAG, "LVGL initialized, label created");

    // --- Timer dla LVGL tick ---
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_timer_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2000)); // 2ms

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}

// --- ADC TASK ---
void adc_task(void *pvParameters)
{
    char buf[64];
    int count = 0;
    
    ESP_LOGI(TAG, "ADC Task started");
    
    // Poczekaj aż GUI się zainicjalizuje
    vTaskDelay(pdMS_TO_TICKS(500));
    
    while (1)
    {
        uint32_t adc_reading = 0;
        
        // Wielokrotne odczyty
        for (int i = 0; i < 32; i++) {
            adc_reading += adc1_get_raw(ADC_CHANNEL);
        }
        adc_reading /= 32;
        
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
        float temp = voltage / 10.0; // LM35 = 10 mV/°C

        snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
        
        ESP_LOGI(TAG, "[%d] ADC raw: %lu, Voltage: %lu mV, Temp: %.1f C", 
                 count++, adc_reading, voltage, temp);

        if (label != NULL)
        {
            lv_label_set_text(label, buf);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0); // Wycentruj po zmianie tekstu
            ESP_LOGI(TAG, "Label updated");
        }
        else
        {
            ESP_LOGE(TAG, "Label is NULL!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// --- APP MAIN ---
void app_main(void)
{
    ESP_LOGI(TAG, "=== Starting application ===");

    // --- Inicjalizacja ADC ---
    ESP_LOGI(TAG, "Configuring ADC...");
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, DEFAULT_VREF, &adc_chars);
    ESP_LOGI(TAG, "ADC configured");

    // --- SPI BUS ---
    ESP_LOGI(TAG, "Initializing SPI bus...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized");

    // --- LCD IO ---
    ESP_LOGI(TAG, "Initializing LCD IO...");
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_NUM_CS,
        .dc_gpio_num = PIN_NUM_DC,
        .spi_mode = 0,
        .pclk_hz = 20 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));
    ESP_LOGI(TAG, "LCD IO initialized");

    // --- LCD PANEL ---
    ESP_LOGI(TAG, "Initializing LCD panel...");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(TAG, "LCD panel initialized");

    // --- BACKLIGHT ---
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BCKL, 1);
    ESP_LOGI(TAG, "Backlight ON");

    // --- Uruchomienie tasków ---
    ESP_LOGI(TAG, "Creating tasks...");
    xTaskCreatePinnedToCore(guiTask, "gui", 4096, panel_handle, 5, NULL, 1);
    xTaskCreatePinnedToCore(adc_task, "adc", 3072, NULL, 4, NULL, 0);
    
    ESP_LOGI(TAG, "=== Application started ===");
}
