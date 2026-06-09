/**
 ******************************************************************************
 * @file    freertos.c
 * @brief   FreeRTOS 多任务应用
 *
 *  任务架构:
 *   优先级6  Task_Start         启动任务(创建其余任务后自删除)
 *   优先级5  Device_Contral     设备控制(队列驱动)
 *   优先级4  smoking_test       烟雾传感器采集
 *   优先级3  Sensor_Contral     温湿度传感器采集
 *   优先级2  Widows_Contral     窗户控制(队列驱动)
 *   优先级2  cooper_task        温湿度协同自动控制
 *   优先级2  updata_task        LVGL显示数据刷新
 *   优先级2  lv_timer           LVGL定时器处理
 *   优先级2  Task_Report        ESP8266数据上报(30秒)
 *   优先级2  Task_Reconnect     WiFi断线重连(低优先级不阻塞)
 *   优先级1  emergency_task     烟雾报警应急响应(事件组驱动)
 *   优先级1  Task_Cmd           ESP8266指令解析分发
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"
#include "usart.h"
#include "stdio.h"
#include "stdlib.h"
#include "semphr.h"
#include "event_groups.h"

#include "sensor.h"
#include "esp8266.h"
#include "device.h"
#include "alarm.h"
#include "Public_Data.h"
#include "personal_Data.h"
#include "mygui.h"
#include "motor.h"
#include "gpio.h"
#include "iwdg.h"
#include "sd_data.h"

/* ============================================================
 *  任务配置
 * ============================================================ */

/* 启动任务 */
#define TASK_START_PRIO        6
#define TASK_START_STACK       128
TaskHandle_t Task_Start_handle;

/* ESP8266指令解析 */
#define TASK_CMD_PRIO          1
#define TASK_CMD_STACK         128
TaskHandle_t Task_Cmd_handle;

/* 烟雾应急响应 */
#define EMERGENCY_PRIO         1
#define EMERGENCY_STACK        128
TaskHandle_t emergency_task_handle;

/* 设备控制 */
#define DEVICE_CTRL_PRIO       5
#define DEVICE_CTRL_STACK      128
TaskHandle_t Device_Contral_handle;

/* 窗户控制 */
#define WINDOW_CTRL_PRIO       2
#define WINDOW_CTRL_STACK      128
TaskHandle_t Widows_Contral_handle;

/* 温湿度采集 */
#define SENSOR_PRIO            3
#define SENSOR_STACK           128
TaskHandle_t Sensor_Contral_handle;

/* 烟雾采集 */
#define SMOKE_PRIO             4
#define SMOKE_STACK            128
TaskHandle_t smoking_test_handle;

/* 温湿度协同控制 */
#define COOPER_PRIO            2
#define COOPER_STACK           128
TaskHandle_t cooper_task_handle;

/* LVGL显示刷新 */
#define UPDATA_PRIO            2
#define UPDATA_STACK           256
TaskHandle_t updata_task_handle;

/* LVGL定时器 */
#define LV_TIMER_PRIO          2
#define LV_TIMER_STACK         512
TaskHandle_t lv_timer_handle;

/* 数据上报 */
#define REPORT_PRIO            2
#define REPORT_STACK           128
TaskHandle_t Task_Report_handle;

/* WiFi断线重连 */
#define RECONNECT_PRIO         2
#define RECONNECT_STACK        1024
TaskHandle_t Task_Reconnect_handle;

/* ============================================================
 *  队列 / 事件组 / 互斥锁
 * ============================================================ */
QueueHandle_t UART_queue_handle;
QueueHandle_t Cmd_queue_handle;
QueueHandle_t device_queue_handle;
QueueHandle_t widows_queue_handle;

EventGroupHandle_t emergency_event_handle;

#define SMOKE_HIGHER_BIT   (1 << 0)
#define SMOKE_HIGHEST_BIT  (1 << 1)
#define SMOKE_NORMAL_BIT   (1 << 2)

SemaphoreHandle_t device_param_mutex;
SemaphoreHandle_t sensor_data_mutex;
SemaphoreHandle_t smoke_data_mutex;
SemaphoreHandle_t lvgl_mutex;
SemaphoreHandle_t modbus_mutex;

typedef enum {
    DEVICE_CONTROL_ID = 1,
    PERSONAL_DATA_ID  = 2,
    WIDOWS_CONTROL_ID = 3,
} ControlID;

/* ============================================================
 *  看门狗喂狗间隔 (单位ms)
 *  配合看门狗超时5秒，每2秒喂一次留有余量
 * ============================================================ */
#define IWDG_FEED_INTERVAL_MS   8000

/* ============================================================
 *  上报失败连续计数 (用于触发重连)
 * ============================================================ */
#define REPORT_FAIL_THRESHOLD   5

/* ============================================================
 *  任务函数前向声明
 * ============================================================ */
static void Task_Start     (void *pvParameters);
static void emergency_task (void *pvParameters);
static void Task_Cmd       (void *pvParameters);
static void Device_Contral (void *pvParameters);
static void Widows_Contral (void *pvParameters);
static void Sensor_Contral (void *pvParameters);
static void smoking_test   (void *pvParameters);
static void cooper_task    (void *pvParameters);
static void updata_task    (void *pvParameters);
static void lv_timer_task  (void *pvParameters);
static void Task_Report    (void *pvParameters);
static void Task_Reconnect (void *pvParameters);

/* ============================================================
 *  FreeRTOS 初始化
 * ============================================================ */
void MX_FREERTOS_Init(void)
{
    UART_queue_handle    = xQueueCreate(5, 100);
    Cmd_queue_handle     = xQueueCreate(5, sizeof(uint32_t));
    device_queue_handle  = xQueueCreate(5, sizeof(uint16_t));
    widows_queue_handle  = xQueueCreate(5, sizeof(uint16_t));

    emergency_event_handle = xEventGroupCreate();

    device_param_mutex = xSemaphoreCreateMutex();
    sensor_data_mutex  = xSemaphoreCreateMutex();
    smoke_data_mutex   = xSemaphoreCreateMutex();
    lvgl_mutex         = xSemaphoreCreateMutex();
    modbus_mutex       = xSemaphoreCreateMutex();

    xTaskCreate(Task_Start, "Task_Start", TASK_START_STACK,
                NULL, TASK_START_PRIO, &Task_Start_handle);
}


/* ============================================================
 *  启动任务: 创建所有应用任务后自删除
 * ============================================================ */

static void Task_Start(void *pvParameters)
{
    printf("[START] 创建所有任务...\r\n");
		BaseType_t xReturn = pdPASS;
		// 1. 紧急任务
		xReturn = xTaskCreate(emergency_task,  "emergency", EMERGENCY_STACK,  NULL, EMERGENCY_PRIO,  &emergency_task_handle);
		if(xReturn == pdPASS) printf("emergency task create success\r\n");
		else printf("emergency task create failed\r\n");

		// 2. 命令任务
		xReturn = xTaskCreate(Task_Cmd,        "Cmd",       TASK_CMD_STACK,   NULL, TASK_CMD_PRIO,   &Task_Cmd_handle);
		if(xReturn == pdPASS) printf("Cmd task create success\r\n");
		else printf("Cmd task create failed\r\n");

		// 3. 设备控制任务
		xReturn = xTaskCreate(Device_Contral,  "DevCtrl",   DEVICE_CTRL_STACK, NULL, DEVICE_CTRL_PRIO, &Device_Contral_handle);
		if(xReturn == pdPASS) printf("DevCtrl task create success\r\n");
		else printf("DevCtrl task create failed\r\n");

		// 4. 窗口控制任务
		xReturn = xTaskCreate(Widows_Contral,  "WinCtrl",   WINDOW_CTRL_STACK, NULL, WINDOW_CTRL_PRIO, &Widows_Contral_handle);
		if(xReturn == pdPASS) printf("WinCtrl task create success\r\n");
		else printf("WinCtrl task create failed\r\n");

		// 5. 传感器控制任务
		xReturn = xTaskCreate(Sensor_Contral,  "Sensor",    SENSOR_STACK,     NULL, SENSOR_PRIO,     &Sensor_Contral_handle);
		if(xReturn == pdPASS) printf("Sensor task create success\r\n");
		else printf("Sensor task create failed\r\n");

		// 6. 烟雾检测任务
		xReturn = xTaskCreate(smoking_test,    "Smoke",     SMOKE_STACK,      NULL, SMOKE_PRIO,      &smoking_test_handle);
		if(xReturn == pdPASS) printf("Smoke task create success\r\n");
		else printf("Smoke task create failed\r\n");

		// 7. 协同任务
		xReturn = xTaskCreate(cooper_task,     "Cooper",    COOPER_STACK,     NULL, COOPER_PRIO,     &cooper_task_handle);
		if(xReturn == pdPASS) printf("Cooper task create success\r\n");
		else printf("Cooper task create failed\r\n");

		// 8. 更新任务
		xReturn = xTaskCreate(updata_task,     "Updata",    UPDATA_STACK,     NULL, UPDATA_PRIO,     &updata_task_handle);
		if(xReturn == pdPASS) printf("Updata task create success\r\n");
		else printf("Updata task create failed\r\n");

		// 9. LVGL定时器任务
		xReturn = xTaskCreate(lv_timer_task,   "LvTimer",   LV_TIMER_STACK,   NULL, LV_TIMER_PRIO,  &lv_timer_handle);
		if(xReturn == pdPASS) printf("LvTimer task create success\r\n");
		else printf("LvTimer task create failed\r\n");

		// 10. 上报任务
		xReturn = xTaskCreate(Task_Report,     "Report",    REPORT_STACK,     NULL, REPORT_PRIO,     &Task_Report_handle);
		if(xReturn == pdPASS) printf("Report task create success\r\n");
		else printf("Report task create failed\r\n");

		// 11. 重连任务
		xReturn = xTaskCreate(Task_Reconnect,  "Reconn",    RECONNECT_STACK,  NULL, RECONNECT_PRIO,  &Task_Reconnect_handle);
		if(xReturn == pdPASS) printf("Reconn task create success\r\n");
		else printf("Reconn task create failed\r\n");

    printf("[START] 所有任务已创建\r\n");
    vTaskDelete(NULL);
}

/* ============================================================
 *  烟雾应急响应任务
 *  触发源: smoking_test 通过事件组触发
 * ============================================================ */
static void emergency_task(void *pvParameters)
{
    printf("[EMERGENCY] 启动\r\n");

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(
            emergency_event_handle,
            SMOKE_HIGHER_BIT | SMOKE_HIGHEST_BIT | SMOKE_NORMAL_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & SMOKE_NORMAL_BIT)
        {
            printf("[EMERGENCY] 烟雾恢复正常\r\n");
            alarm_buzzer_off();
        }

        if (bits & SMOKE_HIGHEST_BIT)
        {
            printf("[EMERGENCY] 严重超标 开窗加报警\r\n");
            widows_control(301);
            alarm_buzzer_on();
        }

        if (bits & SMOKE_HIGHER_BIT)
        {
            printf("[EMERGENCY] 轻度超标\r\n");
        }
    }
}

/* ============================================================
 *  ESP8266 指令解析任务
 *  从云端接收指令按类型ID分发到对应队列
 * ============================================================ */
static void Task_Cmd(void *pvParameters)
{
    printf("[CMD] 启动\r\n");
    uint16_t raw_data;
    uint16_t ctrl_id;

    for (;;)
    {
        raw_data = esp8266_receive_msg();

        if (raw_data != 0)
        {
            printf("[CMD] 收到: %d\r\n", raw_data);
            ctrl_id = raw_data / 10000;

            switch (ctrl_id)
            {
                case DEVICE_CONTROL_ID:
                    xQueueSend(device_queue_handle, &raw_data, pdMS_TO_TICKS(10));
                    break;
                case WIDOWS_CONTROL_ID:
                    xQueueSend(widows_queue_handle, &raw_data, pdMS_TO_TICKS(10));
                    break;
                default:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ============================================================
 *  设备控制任务
 *  从队列接收指令, 加锁后执行Modbus写操作
 * ============================================================ */
static void Device_Contral(void *pvParameters)
{
    printf("[DEVICE] 启动\r\n");
    uint16_t cmd;

    for (;;)
    {
        if (xQueueReceive(device_queue_handle, &cmd, portMAX_DELAY) == pdPASS)
        {
            printf("[DEVICE] 执行: %d\r\n", cmd);

            if (xSemaphoreTake(modbus_mutex, pdMS_TO_TICKS(500)) == pdTRUE)
            {
                uint16_t result = device_control(cmd);
                xSemaphoreGive(modbus_mutex);
                printf("[DEVICE] 结果: %d\r\n", result);
            }
            else
            {
                printf("[DEVICE] 锁超时\r\n");
            }
        }
    }
}

/* ============================================================
 *  窗户控制任务
 * ============================================================ */
static void Widows_Contral(void *pvParameters)
{
    printf("[WINDOW] 启动\r\n");
    uint16_t cmd;

    for (;;)
    {
        if (xQueueReceive(widows_queue_handle, &cmd, portMAX_DELAY) == pdPASS)
        {
            printf("[WINDOW] 执行: %d\r\n", cmd);
            widows_control(cmd);
        }
    }
}

/* ============================================================
 *  温湿度采集任务
 *  周期15秒, 加锁后通过Modbus读取传感器
 * ============================================================ */
static void Sensor_Contral(void *pvParameters)
{
    printf("[SENSOR] 启动\r\n");

    for (;;)
    {
        float temp, humd;

        if (xSemaphoreTake(modbus_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            if (Read_TempHum(&temp, &humd))
            {
                printf("[SENSOR] T=%.1f H=%.1f\r\n", temp, humd);
                EnvData_Set(temp, humd);
            }
            else
            {
                printf("[SENSOR] 读取失败\r\n");
            }

            xSemaphoreGive(modbus_mutex);
        }
        else
        {
            printf("[SENSOR] 锁超时\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

/* ============================================================
 *  烟雾采集任务
 *  三级状态机: NORMAL, WARN, ALARM
 * ============================================================ */
void smoking_test(void *pvParameters)
{
    printf("[SMOKE] 启动\r\n");
    SmokeStateTypeDef state = SMOKE_STATE_NORMAL;

    for (;;)
    {
        uint16_t ppm;

        if (xSemaphoreTake(modbus_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            if (Read_Smoke(&ppm))
            {
                printf("[SMOKE] ppm=%d\r\n", ppm);
                SmokeData_SetValue(ppm);
            }
            else
            {
                printf("[SMOKE] 读取失败\r\n");
                xSemaphoreGive(modbus_mutex);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            xSemaphoreGive(modbus_mutex);
        }
        else
        {
            printf("[SMOKE] 锁超时\r\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        /* 阈值判断, 不涉及Modbus */
        DeviceStatus config = DeviceData_Read();
        uint16_t warn_threshold  = config.SMOKE_ALARM_VALUE;
        uint16_t alarm_threshold = config.SMOKE_ALARM_VALUE + 500;

        if (ppm < warn_threshold)
            state = SMOKE_STATE_NORMAL;
        else if (ppm <= alarm_threshold)
            state = SMOKE_STATE_WARN;
        else
            state = SMOKE_STATE_ALARM;

        SmokeData_SetState(state);

        switch (state)
        {
            case SMOKE_STATE_NORMAL:
                vTaskDelay(pdMS_TO_TICKS(10000));
                break;
            case SMOKE_STATE_WARN:
                printf("[SMOKE] 警告\r\n");
                xEventGroupSetBits(emergency_event_handle, SMOKE_HIGHER_BIT);
                vTaskDelay(pdMS_TO_TICKS(6000));
                break;
            case SMOKE_STATE_ALARM:
                printf("[SMOKE] 报警\r\n");
                xEventGroupSetBits(emergency_event_handle, SMOKE_HIGHEST_BIT);
                vTaskDelay(pdMS_TO_TICKS(2000));
                break;
            default:
                state = SMOKE_STATE_NORMAL;
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

/* ============================================================
 *  温湿度协同自动控制任务
 *  根据温度和湿度自动控制空调和加湿器
 * ============================================================ */
static void cooper_task(void *pvParameters)
{
   
		printf("[COOPER] 启动\r\n");
    float current_temp = 0.0f;
    float current_humd = 0.0f;
    DeviceStatus config;
		DeviceData_Init();
		printf("初始化完成\r\n");
    uint8_t ac_on   = 0;
    uint8_t humi_on = 0;

    for (;;)
    {
			
        EnvData_Get(&current_temp, &current_humd);
        config = DeviceData_Read();

        if (config.AC_AUTO_MODE && config.HUMI_AUTO_MODE)
        {
            uint16_t temp = (uint16_t)current_temp;
            uint16_t humd = (uint16_t)current_humd;

            if (temp >= config.AC_ON_VALUE || humd >= config.HUMIDIFIER_OFF_VALUE)
            {
                if (!ac_on)   { printf("[COOPER] 开空调\r\n");     ac_on = 1; }
                if (humi_on)  { printf("[COOPER] 关加湿器\r\n");   humi_on = 0; }
            }
            else if (temp <= config.AC_OFF_VALUE && humd <= config.HUMIDIFIER_ON_VALUE)
            {
                if (!ac_on)   { printf("[COOPER] 开空调制热\r\n"); ac_on = 1; }
                if (!humi_on) { printf("[COOPER] 开加湿器\r\n");   humi_on = 1; }
            }
            else
            {
                if (ac_on)    { printf("[COOPER] 关空调\r\n");     ac_on = 0; }
                if (humi_on)  { printf("[COOPER] 关加湿器\r\n");   humi_on = 0; }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* ============================================================
 *  LVGL 显示数据刷新任务
 * ============================================================ */
static void updata_task(void *pvParameters)
{
    printf("[LVGL] 刷新任务启动\r\n");

    for (;;)
    {
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        text_updata();
        xSemaphoreGive(lvgl_mutex);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ============================================================
 *  LVGL 定时器任务
 * ============================================================ */
static void lv_timer_task(void *pvParameters)
{
    printf("[LVGL] 定时器任务启动\r\n");

    for (;;)
    {
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        lv_timer_handler();
        xSemaphoreGive(lvgl_mutex);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ============================================================
 *  数据上报任务
 *
 *  周期30秒, 从全局数据接口安全读取后分条上报云端
 *  上报失败则写入SD卡保存
 *  连续失败超过阈值则置重连标志
 * ============================================================ */
static void Task_Report(void *pvParameters)
{
    printf("[REPORT] 启动\r\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    for (;;)
    {
        printf("[REPORT] 开始上报\r\n");

        float temp = 0.0f, humd = 0.0f;
        uint16_t smoke_val = 0;

        EnvData_Get(&temp, &humd);
        SmokeData_GetValue(&smoke_val);

        uint8_t fail_count = 0;

        /* 三条背靠背发, 中间不需要额外延时 */
        if (esp8266_send_msg("temp", (uint16_t)(temp * 10)) != 0){
					  printf("1\r\n");
					vTaskDelay(pdMS_TO_TICKS(200));
            fail_count++;}

        if (esp8266_send_msg("tum", (uint16_t)(humd * 10)) != 0){
					  printf("2\r\n");
					vTaskDelay(pdMS_TO_TICKS(200));
            fail_count++;}

        if (esp8266_send_msg("smoke", smoke_val) != 0){
					  printf("3\r\n");
					vTaskDelay(pdMS_TO_TICKS(200));
            fail_count++;}

        if (fail_count > 0)
        {
            printf("[REPORT] %d条失败 存SD卡\r\n", fail_count);
            SD_LogSensorData(temp, humd, smoke_val);
					  fail_count = 0;
        }
        else
        {
            printf("[REPORT] 全部成功\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}


/* ============================================================
 *  WiFi断线重连任务
 *
 *  优先级1, 平时阻塞等待, 不影响其他任务运行
 *  由 esp_need_reconnect 标志触发
 *  重连期间每2秒喂一次看门狗, 防止复位
 * ============================================================ */
static void Task_Reconnect(void *pvParameters)
{
    printf("[RECONNECT] 启动\r\n");

    for (;;)
    {
        /* 没有重连请求就一直等待, 周期性喂狗 */
        while (esp_need_reconnect == 0)
        {
					printf("[RECONNECT] 喂狗\r\n");
            vTaskDelay(pdMS_TO_TICKS(IWDG_FEED_INTERVAL_MS));
            HAL_IWDG_Refresh(&hiwdg);
        }

        esp_need_reconnect = 0;
        printf("[RECONNECT] 开始重连ESP8266\r\n");

        /* 重连前喂一次 */
        HAL_IWDG_Refresh(&hiwdg);

        /* 执行初始化重连, 内部有较长延时 */
        uint8_t ret = esp8266_init();

        /* 重连后喂一次 */
        HAL_IWDG_Refresh(&hiwdg);

        if (ret == 0)
        {
            printf("[RECONNECT] 重连成功\r\n");
					  esp_need_reconnect = 0;
        }
        else
        {
            printf("[RECONNECT] 重连失败 10秒后重试\r\n");
            esp_need_reconnect = 1;
        }
    }
}
