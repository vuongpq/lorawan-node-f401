##
## Board port layer CMakeLists.txt
## 
## Creates the `board` OBJECT library that the Semtech LoRaMac system/
## mac/ radio/ peripherals/ CMake targets depend on via:
##   $<TARGET_PROPERTY:board,INTERFACE_INCLUDE_DIRECTORIES>
##
project(board)
cmake_minimum_required(VERSION 3.10)

# Board-level sources (this directory)
set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio-board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/delay-board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/rtc-board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/spi-board.c
    # SX1276 board-level code lives with the driver but belongs to the board layer
    ${CMAKE_CURRENT_SOURCE_DIR}/../radio/sx1276/sx1276-board.c
)

add_library(${PROJECT_NAME} OBJECT EXCLUDE_FROM_ALL ${${PROJECT_NAME}_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    # This directory provides: board.h, pinName-board.h, pinName-ioe.h,
    # gpio-board.h, delay-board.h, rtc-board.h, spi-board (via spi.h re-export)
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Semtech system layer (for gpio.h, spi.h, timer.h, utilities.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/../system
    # SX1276 driver headers (sx1276-board.h needs sx1276.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../radio/sx1276
    ${CMAKE_CURRENT_SOURCE_DIR}/../radio
)

# Inherit STM32 HAL / CMSIS include paths from the top-level stm32cubemx
# interface library (added in the root CMakeLists)
target_link_libraries(${PROJECT_NAME} PUBLIC stm32cubemx)
