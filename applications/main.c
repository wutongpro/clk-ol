/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>

#include "aht10.h"

#include <arpa/inet.h>         /* ���� ip_addr_t �ȵ�ַ��ص�ͷ�ļ� */
#include <netdev.h>            /* ����ȫ���� netdev ��ز����ӿں��� */
#include <ntp.h>

#include <drv_lcd.h>
#include "clk.h"
#include "humi.h"
#include "temp.h"

aht10_device_t dev; /* device object */
/* defined the LED0 pin: PE7 */
#define LED0_PIN    GET_PIN(E, 7)

static void led_thread_entry(void *parameter)
{
    while (1) {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}

static int led_thread_start(void)
{
    rt_err_t ret = RT_EOK;
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
    /* ���� led �߳� */
    rt_thread_t thread = rt_thread_create("led", led_thread_entry, RT_NULL, 256, 25, 10);
    /* �����ɹ��������߳� */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    }
    else {
        ret = RT_ERROR;
    }

    return ret;
}

//static void aht10_thread_entry(void *parameter)
//{
//    float humidity, temperature;
//
//    /* continous reading */
//    while (1) {
//        /* read humidity */
//        humidity = aht10_read_humidity(dev);
//        /* former is integer and behind is decimal */
//        rt_kprintf("humidity : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
//        /* read temperature */
//        temperature = aht10_read_temperature(dev);
//        /* former is integer and behind is decimal */
//        rt_kprintf("temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10);
//        rt_thread_mdelay(3000);
//    }
//}
//
//static int aht10_thread_start(void)
//{
//    rt_err_t ret = RT_EOK;
//    const char *i2c_bus_name = "i2c4"; /* i2c bus station */
//
//    rt_thread_mdelay(2000);/* waiting for sensor work */
//    /* initializes aht10, registered device driver */
//    dev = aht10_init(i2c_bus_name);
//    if(dev == RT_NULL) {
//        rt_kprintf(" The sensor initializes failure");
//        return 0;
//    }
//    /* ���� aht10 �߳� */
//    rt_thread_t thread = rt_thread_create("aht10", aht10_thread_entry, RT_NULL, 1024, 25, 10);
//    /* �����ɹ��������߳� */
//    if (thread != RT_NULL) {
//        rt_thread_startup(thread);
//    }
//    else {
//        ret = RT_ERROR;
//    }
//
//    return ret;
//}

static void lcd_draw_symbol(rt_uint8_t symbol)
{
    switch (symbol) {
        case 0:
            /* ���� */
            lcd_clear(WHITE);
            /* ��ʾ clk logo */
            lcd_show_image(56, 40, 128, 128, gImage_clk);
            break;
        case 1:
            /* ���� */
            lcd_clear(WHITE);
            /* ��ʾ humi logo */
            lcd_show_image(56, 40, 128, 128, gImage_humi);
            break;
        case 2:
            /* ���� */
            lcd_clear(WHITE);
            /* ��ʾ temp logo */
            lcd_show_image(56, 40, 128, 128, gImage_temp);
            break;
        default:
            break;
    }
}

static void lcd_thread_entry(void *parameter)
{
    rt_uint8_t status = 0;
    time_t now;
    struct tm *hm;
    int min = 0, hour = 0;
    char hmstr[20];
    float humidity, temperature;
    char htstr[20];

    while (1) {
        switch (status) {
            case 0:
                now = time(RT_NULL);
                hm = gmtime((const time_t *)&now);
                hour = hm->tm_hour;
                min = hm->tm_min;
                sprintf(hmstr, "%02d:%02d", hour, min);
                lcd_draw_symbol(0);
                /* ���ñ���ɫ��ǰ��ɫ */
                lcd_set_color(WHITE, BLACK);
                /* �� LCD ����ʾ�ַ� */
                lcd_show_string(80, 40+128, 32, hmstr);
                rt_thread_mdelay(3000);
                status = 1;
                break;
            case 1:
                /* read humidity */
                humidity = aht10_read_humidity(dev);
                sprintf(htstr, "%d.%d%%", (int)humidity, (int)(humidity * 10) % 10);
                lcd_draw_symbol(1);
                /* ���ñ���ɫ��ǰ��ɫ */
                lcd_set_color(WHITE, BLUE);
                /* �� LCD ����ʾ�ַ� */
                lcd_show_string(80, 40+128, 32, htstr);
                rt_thread_mdelay(3000);
                status = 2;
                break;
            case 2:
                /* read humidity */
                temperature = aht10_read_temperature(dev);
                sprintf(htstr, "%d.%d", (int)temperature, (int)(temperature * 10) % 10);
                lcd_draw_symbol(2);
                /* ���ñ���ɫ��ǰ��ɫ */
                lcd_set_color(WHITE, RED);
                /* �� LCD ����ʾ�ַ� */
                lcd_show_string(80, 40+128, 32, htstr);
                rt_thread_mdelay(3000);
                status = 0;
                break;
            default:
                break;
        }
    }
}

static int lcd_thread_start(void)
{
    rt_err_t ret = RT_EOK;

    /* ���� ntp �߳� */
    rt_thread_t thread = rt_thread_create("lcd", lcd_thread_entry, RT_NULL, 1024, 25, 10);
    /* �����ɹ��������߳� */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
    }
    else {
        ret = RT_ERROR;
    }

    return ret;
}

int main(void)
{
    const char *i2c_bus_name = "i2c4"; /* i2c bus station */
    /* waiting for sensor work */
    rt_thread_mdelay(2000);
    /* initializes aht10, registered device driver */
    dev = aht10_init(i2c_bus_name);
    if(dev == RT_NULL) {
        rt_kprintf(" The sensor initializes failure");
        return 0;
    }
    //��ȡ��������
    struct netdev* net = netdev_get_by_name("esp0");
    //�����жϵ�ǰ�����Ƿ���������
    while(netdev_is_internet_up(net) != 1) {
       rt_thread_mdelay(200);
    }
    //��ʾ��ǰ�����Ѿ���
    rt_kprintf("network is ok!\n");
    //NTP�Զ���ʱ
    time_t cur_time = ntp_sync_to_rtc(NULL);
    if (cur_time) {
        rt_kprintf("Cur Time: %s", ctime((const time_t*) &cur_time));
    }
    else {
        rt_kprintf("NTP sync fail.\n");
    }

    led_thread_start();
//    aht10_thread_start();
    lcd_thread_start();

    return RT_EOK;
}
