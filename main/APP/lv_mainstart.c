/**
 ****************************************************************************************************
 * @file        lv_mainstart.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       LVGL 文件系统使用 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-P4 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
 #include "freertos/FreeRTOS.h"
#include "lv_mainstart.h"
#include "lvgl.h"
#include <stdio.h>
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "ui/ui_novel_reader.h"

/**
* @brief    获取读写指针位置
* @param    fd：文件结构体指针
* @return   返回读写指针在文件的位置(与头部的偏移量)
*/
long lv_tell(lv_fs_file_t *fd)
{
    uint32_t pos = 0;
    lv_fs_tell(fd, &pos);
    return pos;
}

/**
 * @brief   文件系统测试
 * @param   无
 * @return  无
 */
static void lv_fs_test(void)
{
    char rbuf[30] = {0};    /* 存放读取到文件的数据 */
    uint32_t wsize = 0;     /* 已写入到文件数据的字节数 */
    uint32_t rsize = 0;     /* 读取到文件数据的字节数 */
    lv_fs_file_t fd;        /* 文件系统结构体变量 */
    lv_fs_res_t res;        /* 文件操作返回值 */

    /* 写入测试(注意文件名中不能包含下划线) */
    res = lv_fs_open(&fd, "0:/lvgl.txt", LV_FS_MODE_WR);    /* 以写权限打开文件,若SD卡没有该文件会新建 */
    if (res != LV_FS_RES_OK)
    {
        printf("open 0:/lvgl.txt ERROR\n");
        return ;
    }

    res = lv_fs_write(&fd, "hello_lvgl_123", 15, &wsize);   /* 向成功打开的文件写入数据 */
    if (res != LV_FS_RES_OK)
    {
        printf("write /lvgl.txt fail\n");
        return ;
    }
    printf("write data to file succeed \n");                /* 串口显示已经成功写入数据到文件中 */

    lv_fs_close(&fd);                                       /* 关闭文件 */

    /* 读取测试 */
    res = lv_fs_open(&fd, "0:/lvgl.txt", LV_FS_MODE_RD);    /* 以读权限打开文件 */
    if (res != LV_FS_RES_OK)
    {
        printf("open 0:/lvgl.txt ERROR\n");
        return ;
    }

    lv_tell(&fd);                                           /* 获取读写指针的位置 */
    lv_fs_seek(&fd, 0, LV_FS_SEEK_SET);                     /* 设置读写指针到文件头部 */
    lv_tell(&fd);                                           /* 获取读写指针的位置 */

    res = lv_fs_read(&fd, rbuf, 100, &rsize);               /* 向已经成功打开的文件读取文件数据 */
    if (res != LV_FS_RES_OK)
    {
        printf("read ERROR\n");
        return ;
    }

    printf("file:lvgl.txt rd: %s \n", rbuf);                /* 串口显示读取到的数据 */

    lv_fs_close(&fd);                                       /* 关闭文件 */
}

/**
 * @brief   LVGL演示
 * @param   无
 * @return  无
 */

lv_obj_t *label;
void MyTimerCallback(TimerHandle_t xTimer)
{
    lv_obj_set_pos(label, lv_obj_get_x(label) - 1, lv_obj_get_y(label));
}


void lv_mainstart(void)
{
    /* 初始化小说阅读器UI */
    ui_novel_reader_init();
}

