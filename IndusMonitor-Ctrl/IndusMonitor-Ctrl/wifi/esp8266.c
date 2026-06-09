#include "esp8266.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "core_json.h"
#include "iwdg.h"

/* 带喂狗的延时, 在长时间等待中防止看门狗复位 */
static void delay_with_feed(uint32_t ms)
{
    uint32_t elapsed = 0;
    while (elapsed < ms)
    {
        uint32_t wait = (ms - elapsed > 1000) ? 1000 : (ms - elapsed);
        HAL_Delay(wait);
        HAL_IWDG_Refresh(&hiwdg);
        elapsed += wait;
    }
}

// 宏定义部分
// WiFi相关配置
#define WIFI_SSID "lvluoer"
#define WIFI_PASSWD "mx1234567890"

// onenet相关配置
#define PROID "bKMCl4vzOP"
#define TOKEN "version=2018-10-31&res=products%2FbKMCl4vzOP%2Fdevices%2Ftest1&et=99991799216765&method=md5&sign=wFb2g8QwiKPPZIym5glL%2BA%3D%3D"
#define DEVID "test1"

#define MQTT_PUB_CMD_TEMPLATE "AT+MQTTPUB=0,\"$sys/bKMCl4vzOP/test1/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%u\\}\\}}\",0,0\r\n"

// 变量定义部分
unsigned char receive_buf[256];  // 串口2接收缓存数组
unsigned char receive_start = 0; // 串口2接收开始标志位
uint16_t receive_count = 0;      // 串口2接收数据计数器
uint16_t receive_finish = 0;     // 串口2接收结束标志位
uint16_t light_value;
uint16_t device_value;
uint16_t setdata_value;
uint16_t value;
volatile uint8_t esp_need_reconnect = 0;
// 解析json数据函数
// 功能：解析接收到的JSON数据，查找并提取值

uint16_t parse_json_msg(uint8_t *json_msg, uint8_t json_len)
{
    JSONStatus_t result;
    char device_query[] = "params.device";
    char light_query[] = "params.light";
    char setdata_query[] = "params.setdata";

    char *device_value_str;
    char *light_value_str;
    char *setdata_value_str;

    size_t device_value_length;
    size_t light_value_length;
    size_t setdata_value_length;


    result = JSON_Validate((const char *)json_msg, json_len);
    if (result == JSONSuccess)
    {
        result = JSON_Search((char *)json_msg, json_len, device_query, sizeof(device_query) - 1, &device_value_str, &device_value_length);
        if (result == JSONSuccess)
        {
            device_value_str[device_value_length] = '\0';
            device_value = atoi(device_value_str);
            return device_value;
        }

        result = JSON_Search((char *)json_msg, json_len, light_query, sizeof(light_query) - 1, &light_value_str, &light_value_length);
        if (result == JSONSuccess)
        {
            light_value_str[light_value_length] = '\0';
            light_value = atoi(light_value_str);
            return light_value;
        }

        result = JSON_Search((char *)json_msg, json_len, setdata_query, sizeof(setdata_query) - 1, &setdata_value_str, &setdata_value_length);
        if (result == JSONSuccess)
        {
            setdata_value_str[setdata_value_length] = '\0';
            setdata_value = atoi(setdata_value_str);
            return setdata_value;
        }
				
    }
    return 0;
}
// 串口2数据接收处理函数
// 功能：处理串口2接收到的数据，将其存入接收缓存数组，并设置相关标志位
// 参数：无
// 返回值：无
void uart2_receiver_handle(void)
{
    unsigned char receive_data = 0;
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET)
    {
        HAL_UART_Receive(&huart2, &receive_data, 1, 1000); // 串口2接收1位数据
        receive_buf[receive_count++] = receive_data;
        receive_start = 1;  // 串口2接收数据开始标志位置1
        receive_finish = 0; // 串口2接收数据完成标志位清0
    }
}

// 串口2数据接收清0函数
// 功能：清空串口2接收缓存数组，并重置接收相关的标志位和计数器
// 参数：len - 要清空的数据长度
// 返回值：无
void uart2_receiver_clear(uint16_t len)
{
    memset(receive_buf, 0x00, len);
    receive_count = 0;
    receive_start = 0;
    receive_finish = 0;
}

// esp8266发送命令函数
// 功能：向ESP8266发送命令，并等待特定响应数据
// 参数：cmd - 要发送的命令，len - 命令长度，rec_data - 期望接收的数据
// 返回值：0 - 成功接收到期望数据，1 - 发送命令后超时未接收到数据，2 - 接收到数据但未包含期望字符串
uint8_t esp8266_send_cmd(unsigned char *cmd, unsigned char len, char *rec_data)
{
    unsigned char retval = 0;
    unsigned int count = 0;

    HAL_UART_Transmit(&huart2, cmd, len, 1000);
    while ((receive_start == 0) && (count < 1000))
    {
        count++;
        HAL_Delay(1);
        HAL_IWDG_Refresh(&hiwdg);    // 喂狗
    }

    if (count >= 1000)
    {
        retval = 1;
    }
    else
    {
        do
        {
            receive_finish++;
            HAL_Delay(1);
            HAL_IWDG_Refresh(&hiwdg);    // 喂狗
        } while (receive_finish < 500);
        retval = 2;
        if (strstr((const char *)receive_buf, rec_data))
        {
            retval = 0;
        }
    }
    uart2_receiver_clear(receive_count);
    return retval;
}

// esp8266配置wifi网络函数
// 功能：配置ESP8266连接到指定的WiFi网络
// 参数：无
// 返回值：0 - 网络配置成功，1 - 网络配置失败
uint8_t esp8266_config_network(void)
{
    uint8_t retval = 0;
    uint16_t count = 0;

    HAL_UART_Transmit(&huart2, (unsigned char *)"AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWD "\"\r\n", strlen("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWD "\"\r\n"), 1000);

    while ((receive_start == 0) && (count < 1000))
    {
        count++;
        HAL_Delay(1);
    }

    if (count >= 1000)
    {
        retval = 1;
    }
    else
    {
        delay_with_feed(8000); 
        if (strstr((const char *)receive_buf, "OK"))
        {
            retval = 0;
        }
        else
        {
            retval = 1;
        }
    }
    uart2_receiver_clear(receive_count);
    return retval;
}

// esp8266连接服务函数
// 功能：连接ESP8266到onenet服务器
// 参数：无
// 返回值：0 - 连接成功，1 - 连接失败
uint8_t esp8266_connect_server(void)
{
    uint8_t retval = 0;
    uint16_t count = 0;
    char ConnectCmd[256];

    // 构建OneNet平台的MQTT连接AT指令
    sprintf(ConnectCmd, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", DEVID, PROID, TOKEN);

    HAL_UART_Transmit(&huart2, (unsigned char *)ConnectCmd, strlen(ConnectCmd), 1000);

    while ((receive_start == 0) && (count < 1000))
    {
        count++;
        HAL_Delay(1);
    }

    if (count >= 1000)
    {
        retval = 1;
    }
    else
    {
        delay_with_feed(5000); 
        // 根据OneNet平台响应判断连接是否成功，这里假设响应包含"CONNECT OK"
        if (strstr((const char *)receive_buf, "OK"))
        {
            retval = 0;
        }
        else
        {
            retval = 1;
        }
    }
    uart2_receiver_clear(receive_count);
    return retval;
}

// esp8266复位函数
// 功能：复位ESP8266模块
// 参数：无
// 返回值：0 - 复位成功，1 - 复位失败
uint8_t esp8266_reset(void)
{
    uint8_t retval = 0;
    uint16_t count = 0;

    HAL_UART_Transmit(&huart2, (unsigned char *)"AT+RST\r\n", 8, 1000);
    while ((receive_start == 0) && (count < 2000))
    {
        count++;
        HAL_Delay(1);
        HAL_IWDG_Refresh(&hiwdg);    // 喂狗
    }
    if (count >= 2000)
    {
        retval = 1;
    }
    else
    {
        delay_with_feed(5000);
        if (strstr((const char *)receive_buf, "OK"))
        {
            retval = 0;
        }
        else
        {
            retval = 1;
        }
    }
    uart2_receiver_clear(receive_count);
    return retval;
}
// esp8266发送数据函数
// 功能：构建并向onenet服务器发送消息
// 返回值：0 - 发送数据成功，1 - 发送数据失败
uint8_t esp8266_send_msg(const char *name, uint16_t value)
{
    uint8_t retval = 0;
    uint16_t count = 0;
    static uint8_t error_count = 0;
    unsigned char msg_buf[256];

    sprintf((char *)msg_buf, MQTT_PUB_CMD_TEMPLATE, name, value);
    HAL_UART_Transmit(&huart2, (unsigned char *)msg_buf,
                      strlen((const char *)msg_buf), 1000);

    while ((receive_start == 0) && (count < 500))
    {
        count++;
        HAL_Delay(1);
    }

    if (count >= 500)
    {
        retval = 1;
    }
    else
    {
        HAL_Delay(500);
        if (strstr((const char *)receive_buf, "OK"))
        {
            retval = 0;
            error_count = 0;
        }
        else
        {
            retval = 1;
            error_count++;
            if (error_count >= 5)
            {
                error_count = 0;
							  esp_need_reconnect = 1;
                printf("[ESP] 连续失败5次，尝试重连\r\n");
            }
        }
    }

    uart2_receiver_clear(receive_count);
    return retval;  /* 0=成功 1=失败 */
}

// esp8266接收数据函数
// 功能：处理ESP8266接收到的onenet消息
// 参数：无
// 返回值：0 - 接收数据正常并成功解析，1 - 接收数据异常或无数据
uint16_t esp8266_receive_msg(void)
{

    uint16_t retval = 0;
    uint8_t msg_body[256] = {0};
    uint16_t json_len = 0;
    if (receive_start == 1)
    {
        do
        {
            receive_finish++;
            HAL_Delay(1);
        } while (receive_finish < 5);

        if (strstr((const char *)receive_buf, "+MQTTSUBRECV:"))
        {
            if (sscanf((const char *)receive_buf, "+MQTTSUBRECV:0,\"$sys/bKMCl4vzOP/test1/thing/property/set\",%hu,%255[^\n]", &json_len, msg_body) == 2)
            {
                //printf("Length: %hu, Msg: %s\r\n", json_len, msg_body);
                // 假设 parse_json_msg 函数正确定义
                retval = parse_json_msg((uint8_t *)msg_body, json_len);
                //printf("retval:%d\r\n", retval);
                // 表示成功提取数据
            }
        }
        else
        {
            //printf("失败");
            retval = 0;
        }
    }
    uart2_receiver_clear(receive_count);
    return retval;
}
// esp8266初始化函数
// 功能：初始化ESP8266模块，包括设置工作模式、关闭回显、禁止自动连接WiFi、复位、配置WiFi网络、配置MQTT用户信息、连接MQTT服务器以及订阅主题
// 参数：无
// 返回值：无
uint8_t esp8266_init(void)
{
    uint8_t retry_cnt; // 重试计数器（每个步骤复用）
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE); // 打开串口2接收中断

    // 步骤1：设置Station模式（最多重试2次，间隔200ms）
    printf("1.SETTING STATION MODE\r\n");
    retry_cnt = 0; // 重置重试计数器
    while (retry_cnt < 2) // 最多重试2次
    {
        if (esp8266_send_cmd((uint8_t *)"AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n"), "OK") == 0)
        {
            break; // 成功则退出重试循环
        }
        retry_cnt++; // 失败则重试次数+1
        HAL_Delay(200); // 重试间隔200ms
        printf("Retry %d times for SETTING STATION MODE...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2) // 2次重试仍失败
    {
        printf("Error: SETTING STATION MODE FAILED!\r\n");
        return 1;
    }

    // 步骤2：关闭回显（最多重试2次，间隔200ms）
    printf("2.CLOSE ESP8266 ECHO\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_send_cmd((uint8_t *)"ATE0\r\n", strlen("ATE0\r\n"), "OK") == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for CLOSE ESP8266 ECHO...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: CLOSE ESP8266 ECHO FAILED!\r\n");
        return 1;
    }

    // 步骤3：关闭自动连接WiFi（最多重试2次，间隔200ms）
    printf("3.NO AUTO CONNECT WIFI\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_send_cmd((uint8_t *)"AT+CWAUTOCONN=0\r\n", strlen("AT+CWAUTOCONN=0\r\n"), "OK") == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for NO AUTO CONNECT WIFI...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: NO AUTO CONNECT WIFI FAILED!\r\n");
        return 1;
    }

    // 步骤4：复位ESP8266（最多重试2次，间隔200ms）
    printf("4.复位ESP8266\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_reset() == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for RESET ESP8266...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: RESET ESP8266 FAILED!\r\n");
        return 1;
    }
    delay_with_feed(1000);  // 复位后延时，确保模块重启完成

    // 步骤5：连接WiFi（最多重试2次，间隔200ms）
    printf("5.连接wifi\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_config_network() == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for CONNECT WIFI...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: CONNECT WIFI FAILED!\r\n");
        return 1;
    }

    // 步骤6：连接服务器（最多重试2次，间隔200ms）
    printf("6.用户输入：连接服务器\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_connect_server() == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for CONNECT SERVER...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: CONNECT SERVER FAILED!\r\n");
        return 1;
    }

    // 步骤7：连接OneNET（最多重试2次，间隔200ms）
    printf("7.连接onenet\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_send_cmd((uint8_t *)"AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,0\r\n",
                                strlen("AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,0\r\n"), "OK") == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for CONNECT ONENET...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: CONNECT ONENET FAILED!\r\n");
        return 1;
    }

    // 步骤8：订阅下载主题（最多重试2次，间隔200ms）
    printf("8.订阅下载主题\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_send_cmd((uint8_t *)"AT+MQTTSUB=0,\"$sys/bKMCl4vzOP/test1/thing/property/set\",0\r\n",
                                strlen("AT+MQTTSUB=0,\"$sys/bKMCl4vzOP/test1/thing/property/set\",0\r\n"), "OK") == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for SUBSCRIBE DOWNLOAD TOPIC...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: SUBSCRIBE DOWNLOAD TOPIC FAILED!\r\n");
        return 1;
    }

    // 步骤9：订阅上传主题（最多重试2次，间隔200ms）
    printf("9.订阅上传主题\r\n");
    retry_cnt = 0;
    while (retry_cnt < 2)
    {
        if (esp8266_send_cmd((uint8_t *)"AT+MQTTSUB=0,\"$sys/bKMCl4vzOP/test1/thing/property/post\",0\r\n",
                                strlen("AT+MQTTSUB=0,\"$sys/bKMCl4vzOP/test1/thing/property/post\",0\r\n"), "OK") == 0)
        {
            break;
        }
        retry_cnt++;
        HAL_Delay(200);
        printf("Retry %d times for SUBSCRIBE UPLOAD TOPIC...\r\n", retry_cnt);
    }
    if (retry_cnt >= 2)
    {
        printf("Error: SUBSCRIBE UPLOAD TOPIC FAILED!\r\n");
        return 1;
    }

    // 所有步骤成功，返回0
    printf("10.ESP8266 INIT OK!!!\r\n");
    return 0;
}
