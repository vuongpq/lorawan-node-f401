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
set(CMAKE_PROJECT_NAME LORAWAN_NODE)

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

# LoRaMac-node board port + stack
# These options select region / radio / secure-element at CMake configure time
option(REGION_AS923  "Region AS923"  ON)
option(REGION_EU868  "Region EU868"  OFF)
set(RADIO          "sx1276"   CACHE STRING "Radio driver")
set(SECURE_ELEMENT "SOFT_SE"  CACHE STRING "Secure element")
set(BOARD          "board"    CACHE STRING "Board target name")  # used by radio CMakeLists

add_subdirectory(Core/LoRaMac/board)
add_subdirectory(Core/LoRaMac/system)
add_subdirectory(Core/LoRaMac/peripherals)
add_subdirectory(Core/LoRaMac/radio)
add_subdirectory(Core/LoRaMac/mac)

# Include paths for the application layer (lora_app.c, main.c)
# Core/Inc MUST be listed first so that CubeMX gpio.h / i2c.h / spi.h
# shadow the LoRaMac system headers of the same name.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    Core/LoRaMac/board
    Core/LoRaMac/system
    Core/LoRaMac/mac
    Core/LoRaMac/mac/region
    Core/LoRaMac/radio
    Core/LoRaMac/radio/sx1276
    Core/LoRaMac/peripherals
    Core/LoRaMac/peripherals/soft-se
)

# Macro definitions forwarded to the application
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    REGION_AS923
    SOFT_SE
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Link: stm32cubemx HAL + all LoRaMac object libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    $<TARGET_OBJECTS:board>
    $<TARGET_OBJECTS:system>
    $<TARGET_OBJECTS:peripherals>
    $<TARGET_OBJECTS:radio>
    $<TARGET_OBJECTS:mac>
)
