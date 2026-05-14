##
##   ______                              _
##  / _____)             _              | |
## ( (____  _____ ____ _| |_ _____  ____| |__
##  \____ \| ___ |    (_   _) ___ |/ ___)  _ \
##  _____) ) ____| | | || |_| ____( (___| | | |
## (______/|_____)_|_|_| \__)_____)\____)_| |_|
## (C)2013-2017 Semtech
##  ___ _____ _   ___ _  _____ ___  ___  ___ ___
## / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
## \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
## |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
## embedded.connectivity.solutions.==============
##
## License:  Revised BSD License, see LICENSE.TXT file included in the project
## Authors:  Johannes Bruder (STACKFORCE), Miguel Luis (Semtech)
##
project(system)
cmake_minimum_required(VERSION 3.10)

#---------------------------------------------------------------------------------------
# Target
#---------------------------------------------------------------------------------------

# Only include system sources that we have board implementations for.
# Exclude adc, gps, i2c, uart, nvmm – they require board drivers we don't have.
set(${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/delay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/systime.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/fifo.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utilities.c"
)

add_library(${PROJECT_NAME} OBJECT EXCLUDE_FROM_ALL ${${PROJECT_NAME}_SOURCES})

target_include_directories( ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Inherit include paths AND compile definitions (STM32F401xC, USE_HAL_DRIVER, etc.)
# from the board target which itself links to stm32cubemx.
target_link_libraries( ${PROJECT_NAME} PUBLIC board )
