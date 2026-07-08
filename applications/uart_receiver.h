#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include <rtthread.h>

/**
 * @brief 初始化 UART2 接收模块（创建解析线程和打印线程）
 * @return 0 成功，-1 失败
 */
int uart_receiver_init(void);

/**
 * @brief 获取最新识别结果（线程安全）
 * @param buf      输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 实际复制到 buf 的字符串长度（不含 '\0'）
 */
int uart_get_latest_result(char *buf, int buf_size);

#endif
