# 基于 FreeRTOS 的工业环境监测与控制系统
## 项目简介
本系统基于 STM32 + FreeRTOS 实现工业环境下的温度、湿度、烟雾浓度实时监测与自动控制，通过 ESP8266（有时间更新以太网方式） 实现云端数据上报与远程指令下发，具备多级报警、断线重连、SD 卡离线存储等功能。
## 系统功能
- **环境监测**：通过 Modbus RTU 协议周期采集温度、湿度、烟雾浓度
- **自动控制**：根据温湿度阈值自动控制工业空调和加湿器
- **烟雾报警**：三级状态机（正常 / 警告 / 报警），超标时自动开窗并触发蜂鸣器
- **远程通信**：ESP8266 通过 WiFi 上报数据至云端平台，支持远程指令下发控制设备
- **数据备份**：上报失败时自动写入 SD 卡存储，防止数据丢失
- **断线重连**：WiFi 断开后自动检测并执行重连，重连期间看门狗持续喂狗
- **HMI 显示**：LVGL 驱动触摸屏实时显示环境数据
- **看门狗保护**：独立看门狗防止任务阻塞导致系统死机
- 
## 硬件平台
| 模块 | 说明 |
|---|---|
| 主控 | STM32 系列（HAL 库） |
| WiFi 模块 | ESP8266（AT 指令集，UART 通信） |
| 温湿度传感器 | Modbus RTU 接口 |
| 烟雾传感器 | Modbus RTU 接口，PPM 级输出 |
| 设备执行器 | 工业空调、加湿器（Modbus 控制） |
| 电动窗户 | 电机驱动 |
| 显示屏 | LVGL 支持的触摸屏（SPI/并口） |
| 存储 | SD 卡（SPI 接口，FatFS） |
| 蜂鸣器 | GPIO 驱动，报警输出 |
## 软件架构

### 开发环境
- IDE：Keil MDK / STM32CubeIDE
- 操作系统：FreeRTOS v10.x（CMSIS-RTOS v2 封装）
- GUI 库：LVGL v8.x
- 文件系统：FatFS

### 任务设计
系统共 12 个 FreeRTOS 任务，通过队列、事件组、互斥锁实现任务间同步与通信。
| 优先级 | 任务 | 功能 | 周期/触发方式 | 栈大小(word) |
|---|---|---|---|---|
| 6 | Task_Start | 启动任务，创建其余任务后自删除 | 一次性 | 128 |
| 5 | emergency_task | 烟雾报警应急响应（开窗+蜂鸣器） | 事件组驱动 | 128 |
| 5 | Device_Contral | 设备 Modbus 控制 | 队列驱动 | 128 |
| 4 | smoking_test | 烟雾采集与三级状态机 | 2-10 秒 | 128 |
| 3 | Sensor_Contral | 温湿度采集 | 15 秒 | 128 |
| 2 | Widows_Contral | 电动窗户控制 | 队列驱动 | 128 |
| 2 | cooper_task | 温湿度协同自动控制 | 500ms | 128 |
| 2 | updata_task | LVGL 显示刷新 | 2 秒 | 256 |
| 2 | lv_timer_task | LVGL 定时器处理 | 10ms | 512 |
| 2 | Task_Report | 云端数据上报 | 30 秒 | 128 |
| 2 | Task_Reconnect | WiFi 断线重连 | 标志位触发 | 1024 |
| 1 | Task_Cmd | ESP8266 指令解析分发 | 50ms | 128 |

### 任务间通信机制
**队列通信：**
- Cmd_queue：接收 ESP8266 原始数据
- device_queue：设备控制指令分发
- widows_queue：窗户控制指令分发

**事件组：**
- emergency_event：烟雾状态变化触发应急任务
  - bit0：轻度超标（SMOKE_HIGHER_BIT）
  - bit1：严重超标（SMOKE_HIGHEST_BIT）
  - bit2：恢复正常（SMOKE_NORMAL_BIT）
  - 
**互斥锁：**
- modbus_mutex：保护 Modbus 总线（传感器采集与设备控制共享）
- lvgl_mutex：保护 LVGL 资源（显示刷新与定时器共享）
- sensor_data_mutex：温湿度全局数据访问
- smoke_data_mutex：烟雾数据全局数据访问
- device_param_mutex：设备配置参数访问

### 烟雾报警状态机
NORMAL（正常）── ppm >= 阈值 ──▶ WARN（警告）── ppm >= 阈值+500 ──▶ ALARM（报警）
采集周期 10s                     采集周期 6s                         采集周期 2s
设置 HIGHER_BIT                    设置 HIGHEST_BIT
开窗 + 蜂鸣器报警

### 远程指令协议

ESP8266 下发指令为整型数值，按万位拆分类型：

- 1xxxx：设备控制指令（转发至 device_queue）
- 3xxxx：窗户控制指令（转发至 widows_queue）

### 数据上报
每 30 秒向云端上报以下数据：
- 温度（x10 精度）
- 湿度（x10 精度）
- 烟雾浓度（PPM）
任一上报失败则将该批次数据写入 SD 卡备份。

## 工程目录
你说得对，两个问题我都认：

    1.
    定位错误 — Modbus RTU 工业总线 + PPM 级烟雾浓度阈值 + 工业级空调/加湿器控制，这显然是工业环境监测系统，不是智能家居。
    2.
    格式问题 — 上面那个我输出的格式确实有问题，有些地方复制到 README.md 会乱。


下面重新来，这个可以直接复制：


markdown

markdown

# 基于 FreeRTOS 的工业环境监测与控制系统

## 项目简介

本系统基于 STM32 + FreeRTOS 实现工业环境下的温度、湿度、烟雾浓度实时监测与自动控制，通过 ESP8266 实现云端数据上报与远程指令下发，具备多级报警、断线重连、SD 卡离线存储等功能。

## 系统功能

- **环境监测**：通过 Modbus RTU 协议周期采集温度、湿度、烟雾浓度
- **自动控制**：根据温湿度阈值自动控制工业空调和加湿器
- **烟雾报警**：三级状态机（正常 / 警告 / 报警），超标时自动开窗并触发蜂鸣器
- **远程通信**：ESP8266 通过 WiFi 上报数据至云端平台，支持远程指令下发控制设备
- **数据备份**：上报失败时自动写入 SD 卡存储，防止数据丢失
- **断线重连**：WiFi 断开后自动检测并执行重连，重连期间看门狗持续喂狗
- **HMI 显示**：LVGL 驱动触摸屏实时显示环境数据
- **看门狗保护**：独立看门狗防止任务阻塞导致系统死机

## 硬件平台

| 模块 | 说明 |
|---|---|
| 主控 | STM32 系列（HAL 库） |
| WiFi 模块 | ESP8266（AT 指令集，UART 通信） |
| 温湿度传感器 | Modbus RTU 接口 |
| 烟雾传感器 | Modbus RTU 接口，PPM 级输出 |
| 设备执行器 | 工业空调、加湿器（Modbus 控制） |
| 电动窗户 | 电机驱动 |
| 显示屏 | LVGL 支持的触摸屏（SPI/并口） |
| 存储 | SD 卡（SPI 接口，FatFS） |
| 蜂鸣器 | GPIO 驱动，报警输出 |

## 软件架构

### 开发环境

- IDE：Keil MDK / STM32CubeIDE
- 操作系统：FreeRTOS v10.x（CMSIS-RTOS v2 封装）
- GUI 库：LVGL v8.x
- 文件系统：FatFS

### 任务设计

系统共 12 个 FreeRTOS 任务，通过队列、事件组、互斥锁实现任务间同步与通信。

| 优先级 | 任务 | 功能 | 周期/触发方式 | 栈大小(word) |
|---|---|---|---|---|
| 6 | Task_Start | 启动任务，创建其余任务后自删除 | 一次性 | 128 |
| 5 | emergency_task | 烟雾报警应急响应（开窗+蜂鸣器） | 事件组驱动 | 128 |
| 5 | Device_Contral | 设备 Modbus 控制 | 队列驱动 | 128 |
| 4 | smoking_test | 烟雾采集与三级状态机 | 2-10 秒 | 128 |
| 3 | Sensor_Contral | 温湿度采集 | 15 秒 | 128 |
| 2 | Widows_Contral | 电动窗户控制 | 队列驱动 | 128 |
| 2 | cooper_task | 温湿度协同自动控制 | 500ms | 128 |
| 2 | updata_task | LVGL 显示刷新 | 2 秒 | 256 |
| 2 | lv_timer_task | LVGL 定时器处理 | 10ms | 512 |
| 2 | Task_Report | 云端数据上报 | 30 秒 | 128 |
| 2 | Task_Reconnect | WiFi 断线重连 | 标志位触发 | 1024 |
| 1 | Task_Cmd | ESP8266 指令解析分发 | 50ms | 128 |

### 任务间通信机制

**队列通信：**

- Cmd_queue：接收 ESP8266 原始数据
- device_queue：设备控制指令分发
- widows_queue：窗户控制指令分发

**事件组：**

- emergency_event：烟雾状态变化触发应急任务
  - bit0：轻度超标（SMOKE_HIGHER_BIT）
  - bit1：严重超标（SMOKE_HIGHEST_BIT）
  - bit2：恢复正常（SMOKE_NORMAL_BIT）

**互斥锁：**

- modbus_mutex：保护 Modbus 总线（传感器采集与设备控制共享）
- lvgl_mutex：保护 LVGL 资源（显示刷新与定时器共享）
- sensor_data_mutex：温湿度全局数据访问
- smoke_data_mutex：烟雾数据全局数据访问
- device_param_mutex：设备配置参数访问

### 烟雾报警状态机

NORMAL（正常）── ppm >= 阈值 ──▶ WARN（警告）── ppm >= 阈值+500 ──▶ ALARM（报警）
采集周期 10s                     采集周期 6s                         采集周期 2s
设置 HIGHER_BIT                    设置 HIGHEST_BIT
开窗 + 蜂鸣器报警
text

text


### 远程指令协议

ESP8266 下发指令为整型数值，按万位拆分类型：

- 1xxxx：设备控制指令（转发至 device_queue）
- 3xxxx：窗户控制指令（转发至 widows_queue）

### 数据上报

每 30 秒向云端上报以下数据：

- 温度（x10 精度）
- 湿度（x10 精度）
- 烟雾浓度（PPM）

任一上报失败则将该批次数据写入 SD 卡备份。

## 工程目录

├── freertos.c              # FreeRTOS 任务定义与系统初始化
├── Core/
│   ├── main.c              # 主函数入口
│   ├── gpio.c/h            # GPIO 初始化
│   ├── usart.c/h           # 串口驱动
│   ├── iwdg.c/h            # 独立看门狗
│   └── spi.c/h             # SPI 驱动
├── Drivers/
│   ├── sensor.c/h          # 传感器驱动（Modbus 读取）
│   ├── esp8266.c/h         # ESP8266 AT 指令驱动
│   ├── device.c/h          # 设备 Modbus 控制
│   ├── alarm.c/h           # 蜂鸣器报警控制
│   ├── motor.c/h           # 电动窗户电机驱动
│   └── sd_data.c/h         # SD 卡数据存储
├── App/
│   ├── mygui.c/h           # LVGL 界面与数据刷新
│   ├── Public_Data.c/h     # 全局数据接口（互斥保护）
│   └── personal_Data.c/h   # 设备配置参数
└── Middlewares/
└── FreeRTOS/           # FreeRTOS 内核

## 注意事项
- Modbus 总线被多个任务共享，所有读写操作必须先获取 modbus_mutex
- LVGL 操作必须在 lvgl_mutex 保护下执行，避免并发访问导致显示异常
- 看门狗喂狗间隔设为 8 秒，需确保所有任务不会长时间阻塞超过看门狗超时时间
- LVGL 相关任务栈较大，如出现 HardFault 可适当增加栈空间
- 代码中 "Widows" 为历史命名
-代码中协同任务的设备可对应device.c的设备表（注册）进行补充修改，项目只留有框架
-代码底层与应用层解耦，如需更换传感器找到对应api接口函数重写即可（协同除外原理差不多懒得弄了）
-涉及全局访问的数据有用户私有与项目公用两个.c文件，用户可在此注册或修改全局访问的接口函数，做到数据的合理访问。
