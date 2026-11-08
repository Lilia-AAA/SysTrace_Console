#ifndef __SYS_MONITOR_H
#define __SYS_MONITOR_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"   
#include "queue.h"  
#include "event_process_bsp.h"

#include <stdbool.h>

#define TASK_TOTAL_CNT              7U
#define SYS_TASK_SCAN_PERIOD        pdMS_TO_TICKS(10)

#define SYS_MONITOR_TASK_PRIO       1U
#define SYS_MONITOR_STACK_DEPTH     240U

extern bool led_manual_ctrl;
extern bool is_locked;   
extern bool beep_active;    

/*A task is shown as its name, priority, xTaskGetHandle() result, 
  stack size and free stack space.*/
typedef struct 
{
    const char *name;
    uint8_t     prio;
    bool        handle_ok;
    uint16_t    stack_words;
    uint16_t    stack_free_words;
}Status_task_t;

/*A queue is shown as items currently queued, and depth. */
typedef struct
{
    UBaseType_t wait;
    UBaseType_t depth;
}Status_queue_t;

typedef struct
{
/*--------Business Layer-----------*/    
    Dev_status_e dev_status;
    bool locked;
    bool led;
    bool beep;     
    uint8_t rtc_time[6];            /*time: YY MM DD HH MM SS*/
/*--------FreeRTOS Kernel Info-----*/
    uint32_t sys_tick;
    size_t   free_heap;
    size_t   min_free_heap;
    size_t   heap_total;     

    Status_task_t task[TASK_TOTAL_CNT];   
/*--------Queue occupancy-----------*/
    Status_queue_t q_dma;
    Status_queue_t q_uart_gatekeeper;
    Status_queue_t q_beep;
    Status_queue_t q_sys_monitor;
    Status_queue_t q_event;
}Status_snapshot_t;

#define SYS_MONITOR_QUEUE_LEN       2U
#define SYS_MONITOR_ITEM_SIZE       sizeof(Status_snapshot_t)

void vSys_Monitor_Task_Init(void);
void vSys_Monitor_Task(void *pvParameters);
void status_get_snapshot(Status_snapshot_t *snap);

#endif