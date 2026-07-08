#include "uart_receiver.h"
#include <rtdevice.h>
#include <string.h>

/* 内部常量 */
#define RX_BUF_SIZE    64
#define LINE_BUF_SIZE  128

/* 静态变量 */
static rt_device_t uart2_dev = RT_NULL;
static char latest_result[RX_BUF_SIZE] = {0};
static struct rt_semaphore rx_sem;
static struct rt_mutex result_mutex;

static char line_buf[LINE_BUF_SIZE];
static int  line_len = 0;

/* 中断回调：释放信号量 */
static rt_err_t uart2_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

/* 解析线程：接收数据、拼接、提取完整行并解析 */
static void uart2_parser_thread_entry(void *parameter)
{
    char rx_buf[RX_BUF_SIZE];
    int len;
    char *newline;
    char *comma;

    while (1) {
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        len = rt_device_read(uart2_dev, 0, rx_buf, sizeof(rx_buf) - 1);
        if (len <= 0) continue;

        /* 追加到行缓冲区 */
        if (line_len + len < LINE_BUF_SIZE - 1) {
            memcpy(line_buf + line_len, rx_buf, len);
            line_len += len;
            line_buf[line_len] = '\0';
        } else {
            /* 缓冲区溢出，丢弃并重置 */
            line_len = 0;
            line_buf[0] = '\0';
            continue;
        }

        /* 处理所有完整行 */
        while ((newline = strchr(line_buf, '\n')) != RT_NULL) {
            int line_total = newline - line_buf + 1;  // 包括 '\n'
            int content_len = newline - line_buf;     // 有效内容长度

            if (content_len > 0 && content_len < RX_BUF_SIZE) {
                char temp_buf[RX_BUF_SIZE];
                strncpy(temp_buf, line_buf, content_len);
                temp_buf[content_len] = '\0';

                comma = strchr(temp_buf, ',');
                if (comma != RT_NULL) {
                    *comma = '\0';
                    char *label = temp_buf;
                    char *score = comma + 1;

                    rt_mutex_take(&result_mutex, RT_WAITING_FOREVER);
                    rt_snprintf(latest_result, RX_BUF_SIZE,
                                "Label: %s, Confidence: %s", label, score);
                    rt_mutex_release(&result_mutex);
                } else {
                    /* 无逗号时直接保存原字符串（兼容） */
                    rt_mutex_take(&result_mutex, RT_WAITING_FOREVER);
                    strncpy(latest_result, temp_buf, RX_BUF_SIZE - 1);
                    latest_result[RX_BUF_SIZE - 1] = '\0';
                    rt_mutex_release(&result_mutex);
                }
            }

            /* 移除已处理的行 */
            memmove(line_buf, line_buf + line_total, line_len - line_total + 1);
            line_len -= line_total;
        }
    }
}

/* 打印线程（调试用，可注释掉以禁用终端输出） */
static void print_result_thread_entry(void *parameter)
{
    char print_buf[RX_BUF_SIZE];
    while (1) {
        rt_mutex_take(&result_mutex, RT_WAITING_FOREVER);
        strncpy(print_buf, latest_result, sizeof(print_buf) - 1);
        print_buf[sizeof(print_buf) - 1] = '\0';
        rt_mutex_release(&result_mutex);

        if (print_buf[0] != '\0')
            rt_kprintf("Latest recognition: %s\n", print_buf);
        else
            rt_kprintf("No result received yet.\n");

        rt_thread_mdelay(1000);
    }
}

/* 对外初始化函数 */
int uart_receiver_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    uart2_dev = rt_device_find("uart2");
    if (uart2_dev == RT_NULL) {
        rt_kprintf("uart2 not found!\n");
        return -1;
    }

    config.baud_rate = 115200;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.parity    = PARITY_NONE;
    rt_device_control(uart2_dev, RT_DEVICE_CTRL_CONFIG, &config);

    rt_device_open(uart2_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(uart2_dev, uart2_rx_ind);

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_PRIO);
    rt_mutex_init(&result_mutex, "res_mtx", RT_IPC_FLAG_PRIO);

    rt_thread_t parser = rt_thread_create("uart_parser",
                                          uart2_parser_thread_entry,
                                          RT_NULL,
                                          1024,
                                          21,
                                          10);
    if (parser) rt_thread_startup(parser);

    // 注释掉打印线程，避免持续输出干扰
    // rt_thread_t printer = rt_thread_create("prt_res",
    //                                        print_result_thread_entry,
    //                                        RT_NULL,
    //                                        1024,
    //                                        20,
    //                                        10);
    // if (printer) rt_thread_startup(printer);

    rt_kprintf("uart2 initialized.\n");
    return 0;
}

/* 对外获取函数 */
int uart_get_latest_result(char *buf, int buf_size)
{
    if (buf == NULL || buf_size <= 0) return 0;

    rt_mutex_take(&result_mutex, RT_WAITING_FOREVER);
    int len = strlen(latest_result);
    if (len >= buf_size) len = buf_size - 1;
    strncpy(buf, latest_result, len);
    buf[len] = '\0';
    rt_mutex_release(&result_mutex);
    return len;
}
