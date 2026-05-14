# LORAWAN_NODE

STM32F401-based LoRaWAN end-device project, generated from STM32CubeMX and integrated with Semtech LoRaMac-node sources.

This repository is configured for:
- MCU: STM32F401xC (Cortex-M4)
- Build system: CMake + Ninja
- Toolchain: GNU Arm Embedded (arm-none-eabi)
- LoRaWAN region default: AS923
- Radio driver default: sx1276
- Secure element default: SOFT_SE

## Project Layout

- `Core/Src`: application sources (`main.c`, `lora_app.c`, peripheral init)
- `Core/Inc`: application headers (`Commissioning.h`, HAL config)
- `Core/LoRaMac`: LoRaMac stack, region, radio, board/system layers
- `cmake/stm32cubemx`: STM32CubeMX generated CMake integration
- `CMakeLists.txt`: top-level project build setup
- `CMakePresets.json`: Debug/Release presets
- `STM32F401XX_FLASH.ld`: linker script

## Requirements

Install these tools and ensure they are available in PATH:
- CMake >= 3.22
- Ninja
- GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, etc.)

Optional but recommended:
- VS Code with CMake Tools extension
- ST-LINK tools (STM32CubeProgrammer CLI) for flashing

## Build

### Option 1: CMake Presets (recommended)

From repository root:

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Release build:

```bash
cmake --preset Release
cmake --build --preset Release
```

Build outputs are generated under `build/Debug` or `build/Release`.

### Option 2: Manual configure/build

```bash
cmake -S . -B build/Debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
```

## Flash to Board

After build, the ELF file is typically:
- `build/Debug/LORAWAN_NODE.elf`

Using STM32CubeProgrammer CLI (example):

```bash
STM32_Programmer_CLI -c port=SWD -w build/Debug/LORAWAN_NODE.elf -v -rst
```

Adjust command path/options to your local installation.

## LoRaWAN OTAA Configuration

Edit `Core/Inc/Commissioning.h`:
- `COMMISSIONING_DEVICE_EUI`
- `COMMISSIONING_JOIN_EUI`
- `COMMISSIONING_APP_KEY`
- `ACTIVE_REGION`

Current project defaults are set for AS923 and static credentials.

Important:
- Use your own production keys.
- Do not commit real production credentials to public repositories.

## Runtime Notes

- `Core/Src/lora_app.c` implements LoRaWAN init/process using LoRaMac 4.x API.
- `Core/Src/main.c` includes MQ-7 sensor ADC read and forwards values to LoRaWAN app logic.
- Region and stack options are defined in top-level `CMakeLists.txt`.

## Common Commands

Rebuild from clean state:

```bash
cmake --build --preset Debug --clean-first
```

Show binary size (if available in your toolchain):

```bash
arm-none-eabi-size build/Debug/LORAWAN_NODE.elf
```

## License

This repository contains STM32Cube and LoRaMac-node derived components.
Review upstream license files and your project policy before redistribution.
