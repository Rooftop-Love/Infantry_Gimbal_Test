# VSCode + Ozone 使用方法

本工程使用 CMake + Ninja + ARM GNU Toolchain 构建，后续开发、调试和烧录都应使用 CMake 预设和 VSCode 任务。

## 必要工具

- STM32CubeMX
- CMake 3.22 或更新版本
- Ninja
- ARM GNU Toolchain，确保 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy` 在 `PATH` 中
- VSCode
- VSCode 插件：
  - C/C++
  - CMake Tools
  - Cortex-Debug
- OpenOCD，使用 CMSIS-DAP 调试/烧录时需要
- J-Link 软件包，使用 J-Link/Ozone 时需要
- SEGGER Ozone，可选，用于独立调试

## CubeMX 配置

在 `basic_framework.ioc` 中，`Project Manager` 的工具链应选择 `CMake`。重新生成代码后，需要保留并检查以下文件：

```text
CMakeLists.txt
CMakePresets.json
cmake/gcc-arm-none-eabi.cmake
cmake/stm32cubemx/CMakeLists.txt
```

自定义应用、模块和 BSP 源文件由顶层 `CMakeLists.txt` 维护。新增 `.c` 文件后，应加入顶层 `target_sources()`；新增头文件目录后，应加入顶层 `target_include_directories()`。

## 命令行构建

首次配置：

```powershell
cmake --preset Debug
```

编译：

```powershell
cmake --build --preset Debug -j
```

构建产物位于：

```text
build/Debug/basic_framework.elf
build/Debug/basic_framework.hex
build/Debug/basic_framework.bin
build/Debug/basic_framework.map
```

其中 `.hex` 和 `.bin` 已由 CMake post-build 自动生成。

清理：

```powershell
cmake --build --preset Debug --target clean
```

## VSCode 构建

当前 `.vscode/tasks.json` 已配置以下任务：

- `CMake: Configure Debug`
- `CMake: Build Debug`
- `CMake: Clean Debug`
- `Flash: CMSIS-DAP (OpenOCD)`
- `Flash: J-Link`
- `Build + Flash: CMSIS-DAP`
- `Build + Flash: J-Link`
- `RTT: J-Link Client`

默认构建任务是 `CMake: Build Debug`。在 VSCode 中可通过 `Terminal -> Run Build Task` 或快捷键 `Ctrl+Shift+B` 编译。

## VSCode 调试

当前 `.vscode/launch.json` 使用 `build/Debug/basic_framework.elf` 作为调试目标，并在启动调试前执行 `CMake: Build Debug`。

可用调试配置：

- `Cortex-Debug: CMSIS-DAP`
- `Cortex-Debug: J-Link`
- `Cortex-Debug: J-Link + RTT`
- `Cortex-Debug: CMSIS-DAP Attach`
- `Cortex-Debug: J-Link Attach`

CMSIS-DAP 使用：

```text
openocd_dap.cfg
```

J-Link 使用：

```text
stm32.jflash
STM32F407.svd
```

## 烧录

CMSIS-DAP：

```powershell
cmake --build --preset Debug -j
openocd -f openocd_dap.cfg -c init -c "reset halt" -c "flash write_image erase build/Debug/basic_framework.bin 0x08000000" -c "verify_image build/Debug/basic_framework.bin 0x08000000" -c "reset run" -c shutdown
```

J-Link：

```powershell
cmake --build --preset Debug -j
JFlash -openprj stm32.jflash -open build/Debug/basic_framework.hex,0x8000000 -auto -startapp -exit
```

VSCode 中可直接运行：

- `Build + Flash: CMSIS-DAP`
- `Build + Flash: J-Link`

## 常见问题

### 找不到 Ninja

确认 `ninja.exe` 已安装并加入 `PATH`。执行：

```powershell
ninja --version
```

### 找不到 ARM GCC

确认 ARM GNU Toolchain 的 `bin` 目录已加入 `PATH`。执行：

```powershell
arm-none-eabi-gcc --version
```

### 新文件没有参与编译

将源文件加入顶层 `CMakeLists.txt` 的 `target_sources()`，将头文件目录加入 `target_include_directories()`，然后重新配置：

```powershell
cmake --preset Debug
```

### CubeMX 重新生成后构建异常

先检查 `cmake/stm32cubemx/CMakeLists.txt` 是否被重新生成，再确认顶层 `CMakeLists.txt` 中的自定义源码列表没有丢失。CubeMX 主要维护 HAL、FreeRTOS、USB 和外设初始化代码；项目自定义层仍由顶层 CMake 维护。
