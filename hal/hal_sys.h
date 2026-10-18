/**
 * 系统级硬件抽象 - 各平台在 platform/<平台>/ 下实现
 */

#ifndef HAL_SYS_H
#define HAL_SYS_H

// 保护多个任务共享的小段数据，临界区内不能阻塞
void hal_critical_enter(void);
void hal_critical_exit(void);

#endif
