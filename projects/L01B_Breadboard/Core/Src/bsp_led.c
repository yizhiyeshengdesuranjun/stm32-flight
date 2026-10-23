/**
 * @file    bsp_led.c
 * @brief   板载 LED 驱动实现（L01 的第一个库）
 *
 * @note    设计要点：
 *          1. 本文件内**没有 HAL_Delay**。闪烁完全由 led_poll() 驱动，
 *             这样主循环可以同时服务多个模块，谁都不会被谁卡住。
 *          2. 硬件属性（端口/引脚）与运行状态（闪烁周期/上次时刻）分开放：
 *             前者是常量，编译期就确定，放 Flash；后者运行时变化，放 RAM。
 *          3. 所有公开函数都先做参数检查，非法输入直接返回，绝不越界写内存。
 *
 * @note    调用顺序：
 *          MX_GPIO_Init() 由 CubeMX 在 main() 里调用，负责配置 GPIO 硬件；
 *          本库的 led_init() 只负责"把灯设成全灭"这个逻辑状态。
 *
 * @version 1.0  2026-10-22  L01 验收版本
 */

#include "bsp_led.h"
#include "stm32f1xx_hal.h"

/* ==========================================================================
 * 本文件用到的 HAL 函数（先看这里，再看代码）
 * --------------------------------------------------------------------------
 * 1) HAL_GPIO_WritePin(port, pin, level)
 *    作用：把指定引脚的输出电平设成高或低。
 *    参数：port  端口（GPIOA/GPIOB…）
 *          pin   引脚掩码，可用 | 同时指定多位（必须同端口）
 *          level GPIO_PIN_SET(高) / GPIO_PIN_RESET(低)
 *    本质：往 BSRR 或 BRR 寄存器写掩码，硬件一次性置位/复位。
 *
 * 2) HAL_GPIO_TogglePin(port, pin)
 *    作用：翻转引脚电平（亮的变灭、灭的变亮）。
 *    本质：读出 ODR 当前值 → 取反 → 写回，所以不需要你告诉它当前状态。
 *
 * 3) HAL_GetTick()
 *    作用：返回系统启动以来的毫秒数（uint32_t）。
 *    来源：HAL 在 SysTick 中断里每 1ms 累加一次 uwTick。
 *    注意：读取它**不会阻塞**，这是它和 HAL_Delay 的根本区别。
 * ========================================================================== */

/* ==========================================================================
 * 一、私有类型与静态数据
 * ========================================================================== */

/**
 * @brief 一颗灯的硬件"坐标"
 * @note  这些值编译期确定、运行时不变，所以数组用 const，编译器会放进 Flash，
 *        不占用宝贵的 20KB RAM。
 * @note  反例：如果把 port/pin 直接写死在每个函数里（switch 硬编码），
 *        以后加一颗灯要改 5 个地方，必漏。
 */
typedef struct {
    GPIO_TypeDef *port;   /* 端口基地址，如 GPIOA */
    uint16_t      pin;    /* 引脚掩码，如 GPIO_PIN_7 */
} led_hw_t;

/* 下标必须与 led_id_t 的顺序严格对应：
 *   s_led_hw[LED_1] = PA7，s_led_hw[LED_4] = PB0
 * 顺序错了，灯就会"张冠李戴"（点 A 亮 B）。 */
static const led_hw_t s_led_hw[LED_COUNT] = {
    { LED1_PORT, LED1_PIN },   /* LED_1 → PA7 */
    { LED2_PORT, LED2_PIN },   /* LED_2 → PA6 */
    { LED3_PORT, LED3_PIN },   /* LED_3 → PB1 */
    { LED4_PORT, LED4_PIN },   /* LED_4 → PB0 */
};

/**
 * @brief 一颗灯的运行时状态
 * @note  period_ms == 0 表示"不闪烁"，此时灯完全由 led_on/off/toggle 手动控制；
 *        period_ms > 0  表示"闪烁中"，led_poll() 会按这个周期自动翻转。
 */
typedef struct {
    uint16_t period_ms;   /* 翻转周期，单位 ms；0 = 停止闪烁 */
    uint32_t last_tick;   /* 上次翻转时的 HAL_GetTick() 值 */
} led_state_t;

static led_state_t s_led_state[LED_COUNT];

/* ==========================================================================
 * 二、私有工具函数
 * ========================================================================== */

/**
 * @brief  判断灯编号是否合法
 * @param  id 灯编号
 * @retval 1 合法；0 非法
 * @note   为什么要 (uint8_t) 强制转换：
 *         枚举在 C 里本质是 int，调用者可以传 -1。
 *         若不转换，"-1 < 4" 为真，检查就失效了；
 *         转成 uint8_t 后 -1 变成 255，255 < 4 为假，才能拦住。
 */
static uint8_t led_valid(led_id_t id)
{
    return (uint8_t)id < LED_COUNT;
}

/**
 * @brief 直接写引脚电平（不做任何状态管理）
 * @note  之所以单独抽出来，是因为闪烁逻辑需要"只翻转、不改 period_ms"的底层动作，
 *        而公开的 led_toggle() 会顺手取消闪烁——两者语义不同，不能混用。
 */
static void led_write(led_id_t id, GPIO_PinState level)
{
    HAL_GPIO_WritePin(s_led_hw[id].port, s_led_hw[id].pin, level);
}

/* ==========================================================================
 * 三、公开接口实现
 * ========================================================================== */

void led_init(void)
{
    /* 注意：这里不配置 GPIO 时钟/模式。
     * 那是 CubeMX 生成的 MX_GPIO_Init() 的职责，库不重复做别人已经做的事。
     * 我们只负责把"逻辑状态"置为初始值：全灭、不闪烁。 */
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        s_led_state[i].period_ms = 0;
        s_led_state[i].last_tick = HAL_GetTick();
        led_write((led_id_t)i, LED_LEVEL_OFF);
    }
}

void led_on(led_id_t id)
{
    if (!led_valid(id)) return;        /* 非法编号：直接退出，绝不越界 */

    /* 关键：单次动作 = 手动接管，必须先退出闪烁模式。
     * 反例：如果不清零，led_poll() 仍会按旧周期翻转，
     *       你刚点亮的灯最多维持半个周期就被它灭掉。 */
    s_led_state[id].period_ms = 0;
    led_write(id, LED_LEVEL_ON);       /* LED_LEVEL_ON = 低电平 = 亮 */
}

void led_off(led_id_t id)
{
    if (!led_valid(id)) return;
    s_led_state[id].period_ms = 0;
    led_write(id, LED_LEVEL_OFF);      /* LED_LEVEL_OFF = 高电平 = 灭 */
}

void led_toggle(led_id_t id)
{
    if (!led_valid(id)) return;
    s_led_state[id].period_ms = 0;

    /* 用 Toggle 而不是 Write：不需要知道当前是亮还是灭，硬件自己取反。
     * 代价是"读-改-写"，比 Write 慢一点点，但点灯场景完全无所谓。 */
    HAL_GPIO_TogglePin(s_led_hw[id].port, s_led_hw[id].pin);
}

void led_blink(led_id_t id, uint16_t period_ms)
{
    if (!led_valid(id)) return;

    s_led_state[id].period_ms = period_ms;
    s_led_state[id].last_tick = HAL_GetTick();   /* 从现在开始计时 */

    /* 设计选择：设置闪烁时立刻点亮，让调用者马上看到"生效了"。
     * 如果等半个周期才亮，会有"怎么没反应"的错觉。
     * period_ms == 0 时表示停止闪烁，保持当前状态不动。 */
    if (period_ms > 0) {
        led_write(id, LED_LEVEL_ON);
    }
}

void led_poll(void)
{
    /* 整个循环只取一次时间：HAL_GetTick() 内部要读 SysTick 寄存器，
     * 四颗灯各取一次没必要，省几个时钟周期。 */
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        uint16_t p = s_led_state[i].period_ms;
        if (p == 0) continue;          /* 不闪的灯直接跳过（手动模式） */

        /* 溢出安全的定时比较：
         *   无符号减法 now - last 即使遇到 uint32_t 回绕（约 49.7 天一次）
         *   结果仍然正确。
         * 反例：写成 now >= last + p，回绕那一刻 last + p 会溢出成小数，
         *       条件永远不成立 —— 现象是"跑了一个多月后灯突然不闪了"。 */
        if ((uint32_t)(now - s_led_state[i].last_tick) >= p) {
            /* 更新为 now，而不是 last += p：
             * 前者以"实际时刻"为准，不会累积误差；
             * 后者在主循环偶发变慢时会越积越多，最后闪烁明显偏慢。 */
            s_led_state[i].last_tick = now;
            HAL_GPIO_TogglePin(s_led_hw[i].port, s_led_hw[i].pin);
        }
    }
}

/*************************** END OF FILE *************************************/
