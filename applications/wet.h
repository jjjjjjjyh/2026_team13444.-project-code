#ifndef __WET_H__
#define __WET_H__

#include <rtthread.h>
#include <board.h>          // 新增

#define DHT11_PIN              GET_PIN(G, 6)

typedef struct
{
    rt_uint8_t humidity_int;
    rt_uint8_t humidity_dec;
    rt_uint8_t temperature_int;
    rt_uint8_t temperature_dec;
    rt_uint8_t checksum;
} dht11_data_t;

void dht11_init(void);
rt_err_t dht11_read(dht11_data_t *data);

#endif
