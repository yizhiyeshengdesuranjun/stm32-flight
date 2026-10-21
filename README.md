# STM32 四轴飞控 · 学习仓库

> 从零手写的 STM32F103 飞控，按 13 个实验逐步搭建，每个实验产出可复用的库。
> 本仓库记录完整过程：代码、库、踩坑、每一课的验收结果。

---

## 硬件

| 项目 | 型号 |
| --- | --- |
| 主控 | STM32F103C8T6（Cortex-M3，72 MHz，64 KB Flash，20 KB RAM） |
| 姿态传感器 | MPU6050（六轴） |
| 遥控 | NRF24L01 + 自制遥控器 |
| 电机驱动 | 四路 PWM（TIM2 CH1~4，PA0~PA3） |
| 调试器 | J-Link / ST-Link |

## 开发环境

- STM32CubeMX 6.18.1
- VSCode + STM32 VS Code Extension 3.11.0
- arm-none-eabi-gcc 14.3.1 + CMake + Ninja
- Git

## 目录结构

```
.
├── 00-common/          # 所有可复用库（四层：bsp / drv / alg / app）
├── L01_Blink/          # 实验 1：点灯
├── L02_UsbCdc/         # 实验 2：USB 虚拟串口
├── ...
├── FLIGHT/             # 最终合并工程
├── docs/               # 学习笔记、踩坑记录
└── README.md
```

## 学习进度

| 课 | 主题 | 状态 | 标签 |
| --- | --- | --- | --- |
| L00 | volatile 与位运算 | ⬜ | — |
| L01 | GPIO 点灯 | ⬜ | `L01-blink` |
| L02 | USB 虚拟串口 | ⬜ | `L02-usbcdc` |
| L03 | 1 ms 心跳 | ⬜ | `L03-tick` |
| L04 | 四路 PWM | ⬜ | `L04-motor` |
| L05 | 软件 I2C | ⬜ | `L05-i2c` |
| L06 | MPU6050 | ⬜ | `L06-imu` |
| L07 | 滤波 | ⬜ | `L07-filter` |
| L08 | 姿态解算 | ⬜ | `L08-ahrs` |
| L09 | 串级 PID | ⬜ | `L09-pid` |
| L10 | NRF24L01 | ⬜ | `L10-rc` |
| L11 | 混控与解锁 | ⬜ | `L11-mixer` |
| L12 | 合并飞控 | ⬜ | `L12-flight` |

图例：⬜ 未开始　🟨 进行中　✅ 已验收

## 快速开始

```bash
git clone git@github.com:<你的用户名>/stm32-flight.git
cd stm32-flight/L01_Blink
cmake -B build -G Ninja
cmake --build build
```

或用 VSCode 打开工程文件夹，点 STM32 扩展的 Build / Run。

## 关于

- 参考工程：PCBfly / ZIN-7 四轴套件（原工程为 SPL 库 + Keil，本仓库为 HAL + CMake 重构）
- 学习笔记在 `docs/`，每课一篇
- 所有库的接口文档见各 `.h` 文件头部注释
