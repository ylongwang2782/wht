# GD32_clang_cmake_template

## Introduction

这是一个完全脱离`keil`使用`vscode`作为IDE进行GD32嵌入式开发的模板，适合更喜欢使用vscode的开发者。
- `Cmake:build`：构建固件
- `Cmake:install`：烧录固件到设备
- `Cortex-Debug`：对设备进行调试

## 特点

Editor:
- 使用`clangd`作为编辑器的lsp服务器,实现高效跳转和代码补全，以及更好的静态分析

Compile & Build:
- 使用`cmake`作为构建系统
- 使用`ninja`作为构建工具
- 使用`arm-none-eabi-gcc`作为编译器

os:
- 使用`freertos`作为操作系统

## 环境搭建

### Windows

#### 安装依赖

- [vscode](https://code.visualstudio.com/Download)
- [cmake](https://cmake.org/download/)
1. Add to Path `cmake\bin`
- [ninja](https://github.com/ninja-build/ninja/releases/tag/v1.12.1)
1. Add to Path `ninja\`
- [GD32 Embedded Builder](https://www.gd32mcu.com/cn/download)
1. Add to Path `EmbeddedBuilder_v1.4.7.26843\Tools\OpenOCD\xpack-openocd-0.11.0-3\bin`
- gcc-arm-none-eabi
1. download [xPack GNU Arm Embedded GCC](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/tag/v14.2.1-1.1)
2. Add to Path `xpack-arm-none-eabi-gcc-14.2.1-1.1/bin`
- llvm/clang/clangd/clang-format
1. Download [LLVM](https://github.com/llvm/llvm-project/releases/tag/llvmorg-19.1.6)
2. Add to Path `LLVM-19.1.64/bin`

#### 配置vscode
- 指定clangd的编译器
1. 将`arm-none-eabi-g++`的地址定义为环境变量`ARM_CXX_PATH`
2. `export ARM_CXX_PATH="your_path/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++"`
3. reboot生效环境变量 

### Ubuntu

#### 安装依赖

- Vscode
1. `sudo snap install --classic code`

- Ninja
1. `sudo apt update`
2. `sudo apt install ninja-build`
3. `ninja --version`

- Cmake
1. `sudo apt update`
2. `sudo apt install cmake`
3. `cmake --version`

- gcc-arm-none-eabi
1. download [xPack GNU Arm Embedded GCC](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/tag/v14.2.1-1.1)
2. Add to `~/.bashrc`

- llvm/clang/clangd/clang-format
1. Download [LLVM](https://github.com/llvm/llvm-project/releases/tag/llvmorg-19.1.6)
2. Add to `~/.bashrc`
```
export PATH=$PATH:/home/gd32-dev/Documents/tools/LLVM-19.1.6-Linux-X64/bin:/home/gd32-dev/Documents/tools/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin
```

#### 配置vscode
- 指定clangd的编译器
1. 将`arm-none-eabi-g++`的地址定义为环境变量`ARM_CXX_PATH`
2. `export ARM_CXX_PATH="/home/gd32-dev/Documents/tools/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin/arm-none-eabi-g++"`
3. reboot生效环境变量   

### 需要的vscode拓展

可以在`vscode`拓展中搜索`@recommended`以便快速安装推荐的拓展

### 选择Cmake Kits

在`vscode`中选择`Select a Kit`,选择`Unspecified`