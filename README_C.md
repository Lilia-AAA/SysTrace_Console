# SysTrace Console

基于 STM32F103,在 FreeRTOS 上运行的设备日志与诊断终端。

SysTrace Console 把这块小型 STM32 开发板做成一台独立黑匣子：可以通过串口控制台接收命令，上报实时系统状态，驱动板载外设，并把设备事件记录到外部 SPI NOR Flash，断电后日志不会消失。

整套固件运行在 **64KB Flash** 和 **20KB RAM** 的资源限制中：当前固件占用约 **50KB Flash(77%)** 和 **12.3KB RAM(62%)**，其中包含 **8.2KB** 的 FreeRTOS 堆内存。

---

## 功能特性
- **串口命令控制台**：共 11 条命令，覆盖外设控制、系统状态查询、日志读取和清除以及 RTC 设置；
  命令表采用函数指针数组实现，如需新增命令只需在命令表中添加新的记录，配套编写解析函数即可。
- **两级日志缓存**：事件先存入拥有 16 个 32B 记录的 RAM 环形缓冲区，之后异步写入 SPI Flash；
  Flash 侧由 4 个 4KB 扇区首尾相接组成环形日志区，当前扇区写满就擦除并切换下一个扇区循环覆盖；
  上电时扫描全部记录、按 ID 找出最新一条来恢复写指针，复位、掉电之后日志可以接续写入。
- **系统快照**：通过串口发送 `STATUS` 命令可以获取当前系统的全部任务的栈水位、任务优先级、堆使用情况、队列占用、
  RTC 时间以及设备查询时刻的运行状态。
- **按键交互**：本项目使用状态机实现按键消抖、长按、双击；按键产生的事件（单击、双击、长按）会和串口命令送入同一个事件队列，
  由事件处理模块集中处理。
- **死锁检测看门狗**：仅在 `vApplicationIdleHook` 里面喂狗(IWDG)，一旦某个任务卡住或者持续霸占 CPU，空闲任务得不到运行，
  看门狗会在大约4s内触发复位。调试时可以冻结看门狗，避免调试暂停时自动复位。
- **RTC 时间同步**：`SET TIME` 可同时设置日期和时间，由 VBAT 备份域维持，复位后继续走时。STM32F1 的 RTC
  本身没有硬件日期寄存器，日期缓存位于 RAM，换日时把年/月/日和星期写入备份寄存器 BKP_DR2、BKP_DR3，开机恢复。

---

## 硬件与开发工具

| 项目 | 详情 |
|---|---|
| 主控 | STM32F103C8T6，Cortex-M3，Flash 64KB / RAM 20KB, 主频 72MHz |
| RTOS | FreeRTOS V11.3.0，heap_4, 抢占式调度 系统节拍 1000Hz |
| 外设 | USART1, 波特率 115200，8N1；DMA + IDLE 接收；SPI(W25Q NOR Flash)；IWDG；LSE 驱动 RTC; 通用 GPIO |
| 工具链 | CMake + Ninja + arm-none-eabi-gcc，STM32Cube HAL |
| 开发环境 | VS Code + Cortex-Debug + ST-Link |

---

## 系统架构

在 CubeMX 自动生成的硬件初始化代码之上，整体分成四层：

```
Core/       时钟、GPIO、DMA、USART、SPI、IWDG、RTC 的 Hal 初始化
App/        uart_dma_rx, uart_console, event_process, key_control, 
            log_storage, sys_monitor
Utils/      print_helper（串口打印）, w25q_flash（flash 驱动）, get_rtc
FreeRTOS/   Kernel 源码
```

### 任务列表
| 任务名 | 优先级 | 栈深度（word） | 描述 |
|---|---|---|---|
| `Event_Process_Task` | 3 | 260 | 事件处理分发、外设动作执行、生成日志 |
| `DMA_Task` | 2 | 400 | 拼接整行数据、命令解析与分发 |
| `Sys_Monitor_Task` | 1 | 240 | 周期性采集系统快照，周期为 10ms |
| `Log_Flush_Task` | 1 | 160 | 将日志异步刷写进 Flash |
| `UART_Gatekeeper_Task` | 1 | 100 | 统一管理所有串口发送，避免任务并发发送冲突 |
| `Key_Control_Task` | 1 | 80 | 按键状态机处理 |
| `Beep_Task` | 1 | 70 | 蜂鸣器定时控制 |

### 数据流

```
按键事件 / 串口命令
        |
        V
事件队列（Log_t，每条日志 32B）
        |
        V
Event_Process_Task ——> LED / 蜂鸣器 / 命令响应
        |
        V           
Log_add ——> RAM 环形缓冲区（16 × 32B）
        | 缓存达到阈值，释放二进制信号（默认每存满四条日志触发一次刷写，可配置）
        V
Log_Flush_Task ——> W25Q SPI Flash，按ID顺序追加写入

任意任务 ——> PrintStr ——> 打印队列 ——> UART_Gatekeeper_Task ——> UART1 发送    
```

---

## 模块说明

### 串口控制台

接收采用 DMA + IDLE 中断。空闲中断在一段数据爆发接收完成之后触发，不是按行触发，所以 `DMA_Task` 负责把多段
数据拼成完整一行，只有读到 `\r` 或者 `\n` 的时候才分发命令；终端需要用换行符结束一条命令。

接受回调函数里面专门过滤半传输事件：HAL 会把半传输事件送入回调，如果不做过滤，长度超过缓冲区一半的命令会被拦腰截断（如SET TIME）。

### 两级日志

单条日志记录固定长度为 32B 结构体，包含事件类型、参数、RTC 时间戳、递增序号 ID 以及合法魔数。Flash 侧把 4 个 4KB
扇区首尾相接当作环形缓冲区使用：当前扇区写满后，先擦除下一个扇区再继续写，`sector 0 → 1 → 2 → 3 → 0` 循环覆盖。

上电时不能简单地"读到第一条非法记录就停"——环回之后地址最小的扇区反而可能装着最新的数据。因此 `Flash_Log_Init`
遍历全部 4 个扇区，读出每条 magic 合法的记录，用 `(int16_t)(id - max_id) > 0` 选出 ID 最大（最新）的一条，
写指针落在它后面一格。ID 是 16 位的、会回绕，这个有符号差值比较能正确处理 65535 → 0 的跨越。
`READ FLASHLOG` 按扇区顺序打印，环回之后输出顺序不等于时间顺序，需要看 ID 判断先后。

### 系统监控

STATUS 命令输出一份系统快照结构体，包含设备状态、RTC 时间、系统节拍、堆总大小/剩余空间/历史最小剩余，所有任务的栈水位、
优先级以及队列占用量。上面任务的栈大小就是依靠此快照反复调优得来的。

### 按键控制

20ms 消抖，长按触发阈值 1s，双击检测窗口 300ms，采用显式状态机实现：空闲、消抖、按下、等待第二次按下、第二次消抖、等待释放。

### 可靠性设计

- **看门狗用于死锁检测**：IWDG 使用内部 LSI 驱动，超时时间大约 4s，只在 `vApplicationIdleHook` 里面喂狗。
  一旦任务阻塞或霸占CPU，空闲任务跑不起来，芯片就自动复位，避免无声死机；调试时使用 `__HAL_DBGMCU_FREEZE_IWDG()` 冻结看门狗，
  避免调试暂停时自动复位。
- **栈溢出检测**：开启 `configCHECK_FOR_STACK_OVERFLOW = 2`；configASSERT 会记录断言失败的文件名与行号。
- **复位源识别**：开机读取 RCC 复位标志，可以识别本次上电的原因，便于排查问题。

### RTC 同步

RTC 使用 32.768kHz LSE 外部晶振驱动。STM32F1 的 RTC 只有一个 32 位秒计数器，没有硬件日期寄存器：
`HAL_RTC_GetTime()` 把计数器换算成时分秒，每当累计秒数跨过 24 小时，HAL 调用 `RTC_DateUpdate()`
把 RAM 中的日期缓存 `hrtc.DateToUpdate` 推进相应天数，并把计数器回绕到一天以内。

备份寄存器 BKP_DR1 存放魔数（`0x2005`），决定开机是否需要重新初始化时间。日期缓存只在 RAM 里，
`HAL_RTC_Init()` 每次上电都会把它重置为 00-01-01，所以固件在换日时把日期写入 `BKP_DR2`（年、月）和
`BKP_DR3`（星期、日）；开机在魔数分支里直接写回 `hrtc.DateToUpdate`，而不是调用 `HAL_RTC_SetDate()`——
后者会把计数器里累积的整天数截掉。这样即使断电期间过了若干天，备份域里的计数器仍在计数，
下一次读时间时会自动把这几天补进日期。

---

## 资源占用

| 区域 | 使用量 | 总大小 | 占比 |
|---|---|---|---|
| Flash (.text + .data) | ~50KB | 64KB | 77% |
| RAM (.data + .bss，包含 8.2KB RTOS 堆) | ~12.3KB | 20KB | 62% |

---

## 命令参考
| 命令 | 说明 |
|---|---|
| `LED ON` / `LED OFF` | 控制状态指示灯 |
| `BEEP <ms>` | 蜂鸣器鸣叫指定毫秒数 |
| `STATUS` | 打印完整系统快照 |
| `LOCK` / `UNLOCK` | 锁定 / 解锁设备 |
| `READ LOG` | 导出 RAM 环形缓冲区日志 |
| `CLEAR LOG` | 清空 RAM 环形缓冲区日志 |
| `READ FLASHLOG` | 读取存放在 Flash 的日志 |
| `SET TIME YYYY/MM/DD HH:MM:SS` | 设置 RTC 日期与时间 |
| `HELP` | 打印全部可用命令 |

输入不区分大小写。横杠、下划线、制表符都会归一化成空格，设置时间的时候使用斜杠作为日期分隔符。单次接收缓冲区为 40B，命令不要过长。

---

## 编译构建

本工程可通过 `STM32Cube for Visual Studio Code` 插件一键构建；下方为终端手动构建命令，生成固件elf文件。

```bash
cmake -B build/Debug -G Ninja \
-DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake

cmake --build build/Debug
```

---

## 仓库目录

```
Core/        CubeMX 生成的初始化代码与中断服务函数
App/         应用模块：控制台、按键扫描、事件处理、日志、系统监控
Utils/       通用工具：打印、W25Q 驱动、RTC 读取接口
FreeRTOS/    内核源码以及 FreeRTOSConfig.h
Drivers/     STM32F1 HAL 库与 CMSIS
```

---

## 可优化方向

- 在每条日志后增加 CRC，防止 Flash 写入时数据损坏；
- `READ FLASHLOG` 改为按 ID 排序输出，让环回后的日志也保持时间顺序；
- 使用静态任务创建 `xTaskCreateStatic`，消除堆碎片风险；
- 增加 RTT 调试通道，减轻串口控制台压力。