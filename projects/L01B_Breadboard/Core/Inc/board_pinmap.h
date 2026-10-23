/**
 * @file    board_pinmap.h
 * @brief   全项目唯一的引脚定义表
 *
 * @note    为什么要有这个文件？
 *          如果每个 .c 里都直接写 GPIOA / GPIO_PIN_7，那么换一块板子、
 *          或者引脚改版时，你要在十几个文件里搜索替换，必定漏掉一处。
 *          把引脚集中在一个文件里，改板子只改这里 —— 这是"可移植性"最便宜的实现。
 *
 * @note    命名规则：<外设>_<序号>_<PORT|PIN>
 *          这样在代码里看到 LED2_PIN 就知道它属于哪颗灯。
 *
 * @note    引脚来源：《主控原理图(PDF档)20251118》U3 网络标号，并且已在 L01 实测验证。
 *          实测结果：(待填)
 */

#ifndef BOARD_PINMAP_H
#define BOARD_PINMAP_H

/* 为什么包含 main.h？
 * CubeMX 把所有引脚宏（GPIO_PIN_7 等）和端口指针（GPIOA 等）的声明
 * 都放在 main.h 里 —— 那是 CubeMX 自动生成的。包含它，才能用这些宏。 */
#include "main.h"

/* ==========================================================================
 * LED —— 四颗外接 LED（面包板）
 * --------------------------------------------------------------------------
 * 接线事实（面包板）：
 *   3.3V ── LED 阳极（长脚） ── LED 阴极（短脚） ── 330Ω ── PB12~PB15
 *   => 输出低电平(0V) 时，电流从 3.3V 经 LED、电阻流入引脚 → 灯亮；
 *   => 输出高电平(3.3V) 时两端等电位，没有电流 → 灯灭。
 *
 * 与飞控板的区别：飞控板 LED 阳极接 SW_BAT（电池 3.7~4.2V）、无限流电阻；
 * 面包板接 3.3V、串 330Ω（约 4mA），更安全，但"低电平点亮"这一点相同，
 * 所以 bsp_led.c 一行都不用改，只改这个文件。
 *
 * 为什么单独定义 LEVEL_ON / LEVEL_OFF：
 *   如果代码里到处写 GPIO_PIN_RESET，没人知道那是"亮"还是"灭"；
 *   写成 LED_LEVEL_ON 就自解释了，而且将来若换成高电平点亮的板子，只改这两行。
 * ========================================================================== */

/* LED1 → PB12 */
#define LED1_PORT       GPIOB
#define LED1_PIN        GPIO_PIN_12

/* LED2 → PB13 */
#define LED2_PORT       GPIOB
#define LED2_PIN        GPIO_PIN_13

/* LED3 → PB14 */
#define LED3_PORT       GPIOB
#define LED3_PIN        GPIO_PIN_14

/* LED4 → PB15 */
#define LED4_PORT       GPIOB
#define LED4_PIN        GPIO_PIN_15

/* 点亮 / 熄灭 对应的电平。
 * 反例：如果按直觉写成 ON=SET，四颗灯会"常亮不灭"，而且 led_blink 会变成常亮。 */
#define LED_LEVEL_ON    GPIO_PIN_RESET   /* 低电平 = 亮 */
#define LED_LEVEL_OFF   GPIO_PIN_SET     /* 高电平 = 灭 */

/* 灯的总数。
 * 用途：① 数组大小（b_lens[]）② 循环边界 ③ 参数合法性检查 id < LED_COUNT。
 * 危险：如果这里改成 4 而枚举里只有 4 个，正好；若枚举加了第 5 个却忘了改这里，
 *       通过 led_on(第5个) 时数组会越界 —— 这类 bug 在单片机上表现是"莫名其妙重启"。
 * 后缀 U：无符号，避免和 int 比较时的隐式转换警告。 */
#define LED_COUNT       4U

#endif /* BOARD_PINMAP_H */
