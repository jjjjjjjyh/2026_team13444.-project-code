#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include <ctype.h>

#include "distance.h"
#include "metal.h"
#include "wet.h"
#include "stepper.h"
#include "button.h"
#include "servo.h"          // 新增舵机头文件
#include "uart_receiver.h"

#define PIN_BUZZER          GET_PIN(B, 0)   // 蜂鸣器 PB0
#define DISTANCE_ALERT_CM   20

/* 传感器综合读取函数（由按钮线程回调） */
void sensor_read_once(void)
{
    #define READ_COUNT 14
    int humidity_values[READ_COUNT];
    int valid_count = 0;
    int metal_detected = 0;
    int i;

    #define MAX_VISION_READS 7
    char vision_labels[MAX_VISION_READS][32];
    int vision_valid = 0;

    for (i = 0; i < READ_COUNT; i++) {
        /* 1. 金属探测 */
        if (metal_is_detected()) {
            metal_detected = 1;
            LOG_I("Read %d: Metal detected", i + 1);
        } else {
            LOG_I("Read %d: No metal", i + 1);
        }

        /* 2. 温湿度读取 */
        dht11_data_t dht11_data;
        if (dht11_read(&dht11_data) == RT_EOK) {
            humidity_values[valid_count] = dht11_data.humidity_int;
            valid_count++;
            LOG_I("Read %d: Humidity=%d.%d%%, Temp=%d.%d°C", i + 1,
                  dht11_data.humidity_int, dht11_data.humidity_dec,
                  dht11_data.temperature_int, dht11_data.temperature_dec);
        } else {
            LOG_W("Read %d: DHT11 read failed", i + 1);
        }

        /* 3. 视觉识别（每两次循环读取一次） */
        if (i % 2 == 0 && vision_valid < MAX_VISION_READS) {
            char vision_result[64];
            if (uart_get_latest_result(vision_result, sizeof(vision_result)) > 0) {
                char *label_start = strstr(vision_result, "Label: ");
                if (label_start) {
                    label_start += 7;
                    char *comma = strchr(label_start, ',');
                    if (comma) {
                        int len = comma - label_start;
                        if (len > 0 && len < 31) {
                            strncpy(vision_labels[vision_valid], label_start, len);
                            vision_labels[vision_valid][len] = '\0';
                            for (int k = 0; k < len; k++) {
                                vision_labels[vision_valid][k] = tolower(vision_labels[vision_valid][k]);
                            }
                            vision_valid++;
                        }
                    }
                }
                LOG_I("Vision recognition: %s", vision_result);
            } else {
                LOG_I("Vision recognition: no data yet");
            }
        }

        /* 4. 间隔0.5秒（最后一次无需等待） */
        if (i < READ_COUNT - 1) {
            rt_thread_mdelay(500);
        }
    }

    /* 湿度有效判断 */
    if (valid_count == 0) {
        LOG_W("No valid humidity data, cannot classify.");
        return;
    }

    int max_h = humidity_values[0], min_h = humidity_values[0];
    for (i = 1; i < valid_count; i++) {
        if (humidity_values[i] > max_h) max_h = humidity_values[i];
        if (humidity_values[i] < min_h) min_h = humidity_values[i];
    }
    int diff = max_h - min_h;
    LOG_I("Humidity diff: %d%% (max=%d, min=%d)", diff, max_h, min_h);
    int is_wet = (diff > 4) ? 1 : 0;

    /* 视觉多数投票 */
    char final_vision_label[32] = "other";
    int max_count = 0;
    if (vision_valid > 0) {
        for (int a = 0; a < vision_valid; a++) {
            int count = 0;
            for (int b = 0; b < vision_valid; b++) {
                if (strcmp(vision_labels[a], vision_labels[b]) == 0) {
                    count++;
                }
            }
            if (count > max_count) {
                max_count = count;
                strncpy(final_vision_label, vision_labels[a], sizeof(final_vision_label) - 1);
                final_vision_label[sizeof(final_vision_label) - 1] = '\0';
            }
        }
    }
    if (max_count < 5) {
        strcpy(final_vision_label, "other");
    }
    LOG_I("Vision final label: %s (count=%d)", final_vision_label, max_count);

    /* 分类规则 */
    bin_id_t target_bin;
    const char *category;

    if (strcmp(final_vision_label, "other") == 0) {
        target_bin = BIN_OTHER;
        category = "Other (其他垃圾)";
    } else if (strcmp(final_vision_label, "kitchen") == 0 && metal_detected) {
        target_bin = BIN_OTHER;
        category = "Other (其他垃圾)";
    } else if (strcmp(final_vision_label, "kitchen") != 0 && is_wet) {
        target_bin = BIN_OTHER;
        category = "Other (其他垃圾)";
    } else {
        if (strcmp(final_vision_label, "recyclable") == 0) {
            target_bin = BIN_RECYCLABLE;
            category = "Recyclable (可回收垃圾)";
        } else if (strcmp(final_vision_label, "battery") == 0) {
            target_bin = BIN_HAZARDOUS;
            category = "Hazardous (有害垃圾)";
        } else if (strcmp(final_vision_label, "kitchen") == 0) {
            target_bin = BIN_KITCHEN;
            category = "Kitchen waste (厨余垃圾)";
        } else {
            target_bin = BIN_OTHER;
            category = "Other (其他垃圾)";
        }
    }

    LOG_I("Garbage category: %s, moving to bin %d", category, target_bin);
    stepper_move_to(target_bin);

    /* 舵机动作 */
    LOG_I("Stepper finished, servo rotating to 90°...");
    servo_set_angle(90);
    rt_thread_mdelay(1500);
    servo_set_angle(0);
    LOG_I("Servo returned to 0°");

    /* 最终距离测量（只保留这一次） */
    rt_thread_mdelay(500);
        int final_distance = distance_get_cm();
        if (final_distance >= 0) {
            LOG_I("Final distance after servo action: %d cm", final_distance);
            if (final_distance < 15) {
                rt_pin_write(PIN_BUZZER, PIN_HIGH);   // 打开蜂鸣器
                rt_thread_mdelay(5000);               // 鸣响5秒
                rt_pin_write(PIN_BUZZER, PIN_LOW);    // 关闭
            } else {
                rt_pin_write(PIN_BUZZER, PIN_LOW);
            }
        } else {
            LOG_W("Final distance measurement timeout");
            rt_pin_write(PIN_BUZZER, PIN_LOW);
        }
}

int main(void)
{
    // 初始化各传感器模块
    distance_init();
    metal_init();
    dht11_init();
    stepper_init();
    servo_init();          // 新增：初始化舵机（PWM）
    uart_receiver_init();

    // 初始化蜂鸣器引脚
    rt_pin_mode(PIN_BUZZER, PIN_MODE_OUTPUT);
    rt_pin_write(PIN_BUZZER, PIN_LOW);

    // 初始化按键（内部创建线程和信号量）
    button_init();

    LOG_I("All sensors initialized. Press PE4 button to read and classify.");

    // 主线程可以执行其他后台任务
    while (1) {
        rt_thread_mdelay(1000);
    }
}
