#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <math.h>

#define ADC_CHANNEL_X ADC1_CHANNEL_6 //GPIO34
#define ADC_CHANNEL_Y ADC1_CHANNEL_7 //GPIO35

#define ADC_ATTEN ADC_ATTEN_DB_11 //0-3,3V
#define ADC_WIDTH   ADC_WIDTH_BIT_12 // 12-bit resolution

#define LED_0 GPIO_NUM_21
#define LED_1 GPIO_NUM_22
#define LED_2 GPIO_NUM_23

#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_DUTY_RES LEDC_TIMER_8_BIT // Set duty resolution to 8 bits
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_FREQUENCY (5000)
#define LEDC_CHANNEL_0 LEDC_CHANNEL_0
#define LEDC_CHANNEL_1 LEDC_CHANNEL_1
#define LEDC_CHANNEL_2 LEDC_CHANNEL_2

void ledc_init(void){
    ledc_timer_config_t ledc_timer ={
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES, 
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK

    };

    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel_0 = {
        .gpio_num   = LED_0,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0, // initial duty value
        .hpoint     = 0
    };
    
    ledc_channel_config_t ledc_channel_1 = ledc_channel_0;
    ledc_channel_1.gpio_num = LED_1;
    ledc_channel_1.channel  = LEDC_CHANNEL_1;

    ledc_channel_config_t ledc_channel_2 = ledc_channel_0;
    ledc_channel_2.gpio_num = LED_2;
    ledc_channel_2.channel  = LEDC_CHANNEL_2;

    ledc_channel_config(&ledc_channel_0);
    ledc_channel_config(&ledc_channel_1);
    ledc_channel_config(&ledc_channel_2);
}

void set_brightness(ledc_channel_t channel, int percent) {
    float gamma = 2.2;
    int duty = (255 * pow(percent/100.0, gamma));
    ledc_set_duty(LEDC_MODE, channel, duty);
    ledc_update_duty(LEDC_MODE, channel);
}

ledc_channel_t leds[] = { LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2 };
int brightness[] = {0,100,0};
int current_led=1;//middle LED
int previous_led =0;
int next_led=2;
int last_direction;
int time=5;


void blink_task(ledc_channel_t channel){
    
    if (brightness[current_led]<=4){
        set_brightness(channel,20);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    set_brightness(channel,0);
    vTaskDelay(pdMS_TO_TICKS(300));
    set_brightness(channel,brightness[current_led]); 
    
    
}

void update_leds_indices(int direction){
    current_led = (current_led + direction + 3) % 3;
    previous_led = (current_led + 2) % 3;
    next_led     = (current_led + 1) % 3;
    
    
}


void app_main(void) {

    ledc_init();

    // Configure ADC
    adc1_config_width(ADC_WIDTH);

    adc1_config_channel_atten(ADC_CHANNEL_X,ADC_ATTEN); 
    adc1_config_channel_atten(ADC_CHANNEL_Y,ADC_ATTEN);

    set_brightness(LEDC_CHANNEL_0,brightness[0]);
    set_brightness(LEDC_CHANNEL_1,brightness[1]);
    set_brightness(LEDC_CHANNEL_2,brightness[2]);


    while(1){
        int x = adc1_get_raw(ADC_CHANNEL_X); //0-4095
        int y = adc1_get_raw(ADC_CHANNEL_Y); 

        //left direction
        if (x<100 && y>2300 && y<3500 ){  

            if(last_direction==1)update_leds_indices(1);

            printf("Zaczynam ruch w lewo, prev_led: %d\n",previous_led);
            printf("Zaczynam ruch w lewo current_led: %d\n",current_led);
            printf("Zaczynam ruch w lewo, next_led: %d\n",next_led);
           
            brightness[previous_led]+=2;
            set_brightness(leds[previous_led],brightness[previous_led]);
            brightness[current_led]-=2;
            set_brightness(leds[current_led],brightness[current_led]);
            vTaskDelay(pdMS_TO_TICKS(time));
            last_direction = 0;

            if(brightness[current_led]==0){
                update_leds_indices(-1);    
                printf("current_led:%d\n",current_led);
                printf("previous_led:%d\n",previous_led);
            }


        }
        if (x>4000 && y>2300 && y<3500){

            if(last_direction==0)update_leds_indices(-1);
            printf("Zaczynam ruch w prawo, next_led: %d\n",next_led);
            printf("Zaczynam ruch w prawo current_led: %d\n",current_led);
            printf("Zaczynam ruch w prawo, prev_led: %d\n",previous_led);

            
            brightness[next_led]+=2;
            set_brightness(leds[next_led],brightness[next_led]);
            brightness[current_led]-=2;
            set_brightness(leds[current_led],brightness[current_led]);
            vTaskDelay(pdMS_TO_TICKS(time));
            last_direction=1;

            if(brightness[current_led]==0){
                update_leds_indices(1);    
                printf("current_led:%d\n",current_led);
                printf("next_led:%d\n",next_led);
            }
            }


        

        if(y<300 ){
            if(time >=1){
                time -=1;
                vTaskDelay(pdMS_TO_TICKS(20));
                printf("changed time to: %d\n",time);
            }
                
            
        }
        if(y>3900 ){
            
             if(time <50){
                time +=1;
                vTaskDelay(pdMS_TO_TICKS(20));
                printf("changed time to: %d\n",time);
            }
        }
        
        //printf("X: %d, Y: %d\n",x,y);

        vTaskDelay(pdMS_TO_TICKS(100)); 

    }
}