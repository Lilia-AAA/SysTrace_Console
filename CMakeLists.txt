cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME SysTrace_Console)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    FreeRTOS/Src/timers.c
    FreeRTOS/Src/tasks.c
    FreeRTOS/Src/stream_buffer.c
    FreeRTOS/Src/queue.c
    FreeRTOS/Src/port.c
    FreeRTOS/Src/list.c
    FreeRTOS/Src/heap_4.c
    FreeRTOS/Src/croutine.c
    FreeRTOS/Src/event_groups.c

    Utils/print_helper.c
    Utils/w25q_flash.c
    Utils/get_rtc.c

    App/uart_dma_rx/dma_task.c
    App/uart_dma_rx/uart_dma_impl.c

    App/uart_console/uart_console.c
    App/uart_console/uart_console_bsp.c

    App/key_control/key_control_task.c

    App/event_process/event_process_task.c
    App/event_process/event_process_bsp.c
    App/event_process/led_process.c
    App/event_process/beep_process.c

    App/sys_monitor/sys_monitor_task.c

    App/log_storage/log_storage.c
    App/log_storage/log_flush_task.c
    App/log_storage/flash_log.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    FreeRTOS/Inc
    App/uart_dma_rx
    App/uart_console
    App/key_control
    App/event_process
    App/sys_monitor
    App/log_storage
    Utils
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
