/**
 ****************************************************************************************************
 * @file        ui_novel_reader.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       LVGL 小说阅读器 实现文件
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

#include "ui_novel_reader.h"
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 全局变量定义 */
static lv_obj_t *file_list_screen;    /* 文件列表屏幕 */
static lv_obj_t *reader_screen;       /* 阅读屏幕 */
static lv_obj_t *file_grid;           /* 文件网格控件 */
static lv_obj_t *reader_label;        /* 阅读内容标签 */
static lv_obj_t *title_label;         /* 标题标签 */
static lv_style_t reader_style;       /* 阅读器样式 */
static lv_style_t grid_style;         /* 网格样式 */
static lv_style_t book_style;         /* 书本样式 */
static lv_style_t title_style;        /* 标题样式 */
static lv_font_t *reader_font = NULL; /* 阅读器字体 */
static lv_obj_t *settings_screen;     /* 设置屏幕 */
static void *font_data = NULL;        /* 字体数据内存，用于释放 */

static char current_file_path[256];    /* 当前打开的文件路径 */
static char *text_buffer = NULL;       /* 文本缓冲区 */
static size_t text_buffer_size = 0;    /* 文本缓冲区大小 */
static size_t current_page = 0;        /* 当前页码 */
static size_t total_pages = 0;         /* 总页数 */
static size_t chars_per_page = 0;      /* 每页字符数 */
static size_t file_size = 0;           /* 文件总大小 */
static lv_fs_file_t novel_file;        /* 当前打开的小说文件 */
static bool file_opened = false;       /* 文件是否已打开 */
static size_t preload_pages = 3;       /* 预加载页数 */
static size_t loaded_start_page = 0;   /* 当前加载的起始页 */
static size_t loaded_end_page = 0;     /* 当前加载的结束页 */

/* 文件列表分页变量 */
static size_t file_list_current_page = 0;  /* 当前文件列表页 */
static size_t file_list_total_pages = 0;   /* 文件列表总页数 */
static size_t files_per_page = 14;          /* 每页显示的文件数 */
static char **file_name_list = NULL;       /* 文件名列表 */
static size_t file_count = 0;              /* 文件总数 */

/* 字体设置相关变量 */
static uint8_t font_size = 24;             /* 字体大小, 默认24像素 */
static uint8_t font_sizes[] = {16, 20, 24, 28, 32, 36, 40}; /* 可选字体大小 */
static uint8_t font_sizes_count = sizeof(font_sizes) / sizeof(font_sizes[0]);
static char font_config_path[] = "0:/font_config.txt"; /* 字体配置文件路径 */
/* 字体选择相关变量 */
static char **font_file_list = NULL;       /* 字体文件名列表 */
static size_t font_file_count = 0;         /* 字体文件数量 */
static char current_font_name[256] = {0};  /* 当前选择的字体名称 */
static int current_font_index = 0;         /* 当前选择的字体索引 */

/* 颜色定义 */
#define COLOR_BACKGROUND        lv_color_hex(0xF5F5F5)  /* 背景色 */
#define COLOR_BOOK              lv_color_hex(0x3498DB)  /* 书本颜色 */
#define COLOR_BOOK_BORDER       lv_color_hex(0x2980B9)  /* 书本边框 */
#define COLOR_BOOK_TEXT         lv_color_hex(0xFFFFFF)  /* 书本文字 */
#define COLOR_TITLE             lv_color_hex(0x2C3E50)  /* 标题颜色 */
#define COLOR_READER_BG         lv_color_hex(0xF5F5F5)  /* 阅读背景，改为与整体背景一致 */
#define COLOR_READER_TEXT       lv_color_hex(0x333333)  /* 阅读文字 */
#define COLOR_BUTTON            lv_color_hex(0x27AE60)  /* 按钮颜色 */
#define COLOR_BUTTON_TEXT       lv_color_hex(0xFFFFFF)  /* 按钮文字 */
#define COLOR_CARD              lv_color_hex(0xFFFFFF)  /* 卡片颜色 */
#define COLOR_CARD_SHADOW       lv_color_hex(0xAAAAAA)  /* 卡片阴影 */

/* 前向声明 */
static void open_reader(const char *file_path);
static void show_file_grid(void);
static bool load_page_content(size_t page_num);
static void display_current_page(void);
static void reader_event_cb(lv_event_t *e);
static void book_event_cb(lv_event_t *e);
static void back_btn_event_cb(lv_event_t *e);
static lv_font_t *load_font_from_sd(void);
static lv_font_t *load_font_with_size(uint8_t size);
static void calculate_page_layout(void);
static void init_styles(void);
static void file_list_prev_page(lv_event_t *e);
static void file_list_next_page(lv_event_t *e);
static void slider_event_cb(lv_event_t *e);
static void free_text_file_list_resources(void);
static bool save_font_config(void);
static bool load_font_config(void);
static void font_size_slider_event_cb(lv_event_t *e);
static void settings_save_btn_event_cb(lv_event_t *e);
static void settings_cancel_btn_event_cb(lv_event_t *e);
static void create_settings_screen(void);
static void settings_btn_event_cb(lv_event_t *e);
static void scan_font_files(void);
static void free_font_list_resources(void);
static void font_dd_event_cb(lv_event_t *e);

/**
 * @brief 初始化样式
 * 
 * @return 无
 */
static void init_styles(void)
{
    /* 初始化阅读器样式 */
    lv_style_init(&reader_style);
    if (reader_font != NULL) {
        lv_style_set_text_font(&reader_style, reader_font);
    }
    lv_style_set_text_color(&reader_style, COLOR_READER_TEXT);
    lv_style_set_text_align(&reader_style, LV_TEXT_ALIGN_LEFT);
    
    /* 初始化网格样式 */
    lv_style_init(&grid_style);
    lv_style_set_bg_color(&grid_style, COLOR_BACKGROUND);
    lv_style_set_pad_all(&grid_style, 10);
    lv_style_set_pad_row(&grid_style, 20);
    lv_style_set_pad_column(&grid_style, 20);
    
    /* 初始化书本样式 */
    lv_style_init(&book_style);
    lv_style_set_bg_color(&book_style, COLOR_CARD);
    lv_style_set_border_color(&book_style, COLOR_CARD);
    lv_style_set_border_width(&book_style, 0);
    lv_style_set_radius(&book_style, 10);
    lv_style_set_shadow_width(&book_style, 15);
    lv_style_set_shadow_color(&book_style, COLOR_CARD_SHADOW);
    lv_style_set_shadow_ofs_y(&book_style, 5);
    lv_style_set_shadow_opa(&book_style, LV_OPA_30);
    lv_style_set_text_color(&book_style, COLOR_READER_TEXT);
    if (reader_font != NULL) {
        lv_style_set_text_font(&book_style, reader_font);
    }
    
    /* 初始化标题样式 */
    lv_style_init(&title_style);
    lv_style_set_text_color(&title_style, COLOR_TITLE);
    if (reader_font != NULL) {
        lv_style_set_text_font(&title_style, reader_font);
    }
}

/**
 * @brief 初始化小说阅读器UI
 * 
 * @return 无
 */
void ui_novel_reader_init(void)
{
    /* 加载字体 */
    reader_font = load_font_from_sd();
    
    /* 初始化样式 */
    init_styles();
    
    /* 创建文件列表屏幕 */
    file_list_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(file_list_screen, COLOR_BACKGROUND, 0);
    
    /* 创建顶部状态栏 */
    lv_obj_t *top_bar = lv_obj_create(file_list_screen);
    lv_obj_set_size(top_bar, lv_pct(100), 60);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    
    /* 创建标题 */
    title_label = lv_label_create(top_bar);
    lv_label_set_text(title_label, "小说阅读器");
    lv_obj_add_style(title_label, &title_style, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_center(title_label);
    
    /* 创建设置按钮 */
    lv_obj_t *settings_btn = lv_btn_create(top_bar);
    lv_obj_set_size(settings_btn, 40, 40);
    lv_obj_align(settings_btn, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_set_style_bg_color(settings_btn, lv_color_hex(0x1ABC9C), 0);
    lv_obj_set_style_radius(settings_btn, 20, 0);
    lv_obj_add_event_cb(settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_label);
    
    /* 创建文件网格 */
    file_grid = lv_obj_create(file_list_screen);
    lv_obj_set_size(file_grid, lv_pct(95), lv_pct(75));
    lv_obj_align(file_grid, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_add_style(file_grid, &grid_style, 0);
    lv_obj_set_style_bg_opa(file_grid, LV_OPA_0, 0);
    lv_obj_set_style_border_width(file_grid, 0, 0);
    lv_obj_set_flex_flow(file_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_flex_main_place(file_grid, LV_FLEX_ALIGN_SPACE_EVENLY, 0);
    lv_obj_set_style_flex_cross_place(file_grid, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_scrollbar_mode(file_grid, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_scroll_dir(file_grid, LV_DIR_VER);
    
    /* 创建阅读屏幕 */
    reader_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(reader_screen, COLOR_READER_BG, 0);
    
    /* 创建返回按钮 */
    lv_obj_t *back_btn = lv_btn_create(reader_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(back_btn, COLOR_BUTTON, 0);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "返回");
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, COLOR_BUTTON_TEXT, 0);
    if (reader_font != NULL) {
        lv_obj_add_style(back_label, &reader_style, 0);
    }
    
    /* 创建阅读内容容器 - 扩大尺寸 */
    lv_obj_t *reader_cont = lv_obj_create(reader_screen);
    lv_obj_set_size(reader_cont, lv_pct(95), lv_pct(85));  /* 增加高度，几乎占满整个屏幕 */
    lv_obj_align(reader_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_clear_flag(reader_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(reader_cont, COLOR_READER_BG, 0); /* 背景与整体背景一致 */
    lv_obj_set_style_bg_opa(reader_cont, LV_OPA_100, 0);
    lv_obj_set_style_border_width(reader_cont, 0, 0);
    lv_obj_set_style_shadow_width(reader_cont, 0, 0); /* 移除阴影 */
    lv_obj_set_style_pad_all(reader_cont, 15, 0);
    
    /* 创建阅读内容标签 */
    reader_label = lv_label_create(reader_cont);
    lv_label_set_long_mode(reader_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(reader_label, lv_pct(100));
    lv_obj_set_style_text_align(reader_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(reader_label, 2, 0); /* 增加行间距 */
    lv_obj_align(reader_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_style(reader_label, &reader_style, 0);
    
    /* 添加手势事件 */
    lv_obj_add_event_cb(reader_cont, reader_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_clear_flag(reader_cont, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(reader_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    /* 创建页面跳转区域 - 简化设计 */
    lv_obj_t *jump_cont = lv_obj_create(reader_screen);
    lv_obj_set_size(jump_cont, lv_pct(95), 40);
    lv_obj_align(jump_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(jump_cont, COLOR_READER_BG, 0); /* 背景与整体背景一致 */
    lv_obj_set_style_bg_opa(jump_cont, LV_OPA_0, 0); /* 透明背景 */
    lv_obj_set_style_border_width(jump_cont, 0, 0);
    
    /* 创建进度条 */
    lv_obj_t *slider = lv_slider_create(jump_cont);
    lv_obj_set_width(slider, lv_pct(90));
    lv_obj_center(slider);
    lv_slider_set_range(slider, 0, 100);  /* 使用百分比范围 */
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    /* 显示文件列表屏幕 */
    lv_scr_load(file_list_screen);
    
    /* 初始化文件列表分页 */
    file_list_current_page = 0;
    
    /* 扫描TXT文件 */
    show_file_grid();
    
    /* 创建设置屏幕 */
    create_settings_screen();
}

/**
 * @brief 显示文件网格
 * 
 * @return 无
 */
static void show_file_grid(void)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    char fn[256];
    
    /* 清空文件网格 */
    lv_obj_clean(file_grid);
    
    /* 释放之前的文件名列表 */
    if (file_name_list != NULL) {
        for (size_t i = 0; i < file_count; i++) {
            if (file_name_list[i] != NULL) {
                free(file_name_list[i]);
            }
        }
        free(file_name_list);
        file_name_list = NULL;
        file_count = 0;
    }
    
    /* 打开SD卡根目录 */
    res = lv_fs_dir_open(&dir, "0:/");
    if (res != LV_FS_RES_OK) {
        printf("无法打开根目录, 错误码: %d\n", res);
        return;
    }
    
    /* 先计算TXT文件总数 */
    size_t txt_count = 0;
    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK || fn[0] == '\0') {
            break; /* 没有更多文件或出错 */
        }
        
        /* 检查是否为TXT文件 */
        size_t len = strlen(fn);
        if (len > 4) {
            if ((strcmp(&fn[len-4], ".txt") == 0) || 
                (strcmp(&fn[len-4], ".TXT") == 0)) {
                txt_count++;
            }
        }
    }
    
    /* 分配文件名列表内存 */
    if (txt_count > 0) {
        file_name_list = (char **)malloc(sizeof(char *) * txt_count);
        if (file_name_list == NULL) {
            printf("内存分配失败\n");
            lv_fs_dir_close(&dir);
            return;
        }
        memset(file_name_list, 0, sizeof(char *) * txt_count);
    }
    
    /* 重新打开目录 */
    lv_fs_dir_close(&dir);
    res = lv_fs_dir_open(&dir, "0:/");
    if (res != LV_FS_RES_OK) {
        printf("无法打开根目录, 错误码: %d\n", res);
        return;
    }
    
    /* 收集所有TXT文件名 */
    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK || fn[0] == '\0') {
            break; /* 没有更多文件或出错 */
        }
        
        /* 检查是否为TXT文件 */
        size_t len = strlen(fn);
        if (len > 4) {
            if ((strcmp(&fn[len-4], ".txt") == 0) || 
                (strcmp(&fn[len-4], ".TXT") == 0)) {
                
                /* 保存文件名 */
                file_name_list[file_count] = (char *)malloc(len + 1);
                if (file_name_list[file_count] != NULL) {
                    strcpy(file_name_list[file_count], fn);
                    file_count++;
                }
            }
        }
    }
    
    /* 关闭目录 */
    lv_fs_dir_close(&dir);
    
    /* 计算总页数 */
    file_list_total_pages = (file_count + files_per_page - 1) / files_per_page;
    if (file_list_total_pages == 0) file_list_total_pages = 1;
    
    /* 确保当前页有效 */
    if (file_list_current_page >= file_list_total_pages) {
        file_list_current_page = file_list_total_pages - 1;
    }
    
    /* 计算当前页显示的文件范围 */
    size_t start_idx = file_list_current_page * files_per_page;
    size_t end_idx = start_idx + files_per_page;
    if (end_idx > file_count) end_idx = file_count;
    
    /* 显示当前页的文件 */
    for (size_t i = start_idx; i < end_idx; i++) {
        /* 创建书本容器 */
        lv_obj_t *book_cont = lv_obj_create(file_grid);
        lv_obj_set_size(book_cont, 150, 210);
        lv_obj_add_style(book_cont, &book_style, 0);
        lv_obj_set_style_pad_all(book_cont, 0, 0);
        
        /* 创建书本顶部颜色条 */
        lv_obj_t *book_top = lv_obj_create(book_cont);
        lv_obj_set_size(book_top, lv_pct(100), 70);
        lv_obj_align(book_top, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_radius(book_top, 10, 0);
        lv_obj_set_style_border_width(book_top, 0, 0);
        
        /* 为每本书随机生成一个颜色 */
        uint32_t colors[] = {0x3498DB, 0x27AE60, 0x9B59B6, 0xE74C3C, 0xF39C12, 0x16A085};
        uint32_t color_idx = i % (sizeof(colors) / sizeof(colors[0]));
        lv_obj_set_style_bg_color(book_top, lv_color_hex(colors[color_idx]), 0);
        
        /* 创建书本图标 */
        lv_obj_t *book_icon = lv_label_create(book_top);
        lv_label_set_text(book_icon, LV_SYMBOL_FILE);
        if (reader_font != NULL) {
            lv_obj_set_style_text_font(book_icon, reader_font, 0);
        } else {
            lv_obj_set_style_text_font(book_icon, &lv_font_montserrat_22, 0);
        }
        lv_obj_center(book_icon);
        lv_obj_set_style_text_color(book_icon, lv_color_white(), 0);
        
        /* 创建书本标题 */
        lv_obj_t *book_title = lv_label_create(book_cont);
        
        /* 截取文件名，去掉扩展名 */
        size_t len = strlen(file_name_list[i]);
        char short_name[64] = {0};
        strncpy(short_name, file_name_list[i], len - 4 > 63 ? 63 : len - 4);
        
        /* 如果文件名太长，截断并添加省略号 */
        if (strlen(short_name) > 10) {
            short_name[9] = '.';
            short_name[10] = '.';
            short_name[11] = '.';
            short_name[12] = '\0';
        }
        
        lv_label_set_text(book_title, short_name);
        lv_obj_align(book_title, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_set_style_text_align(book_title, LV_TEXT_ALIGN_CENTER, 0);
        if (reader_font != NULL) {
            lv_obj_set_style_text_font(book_title, reader_font, 0);
        } else {
            lv_obj_set_style_text_font(book_title, &lv_font_montserrat_16, 0);
        }
        
        /* 创建点击覆盖层，用于处理点击事件 */
        lv_obj_t *click_area = lv_obj_create(book_cont);
        lv_obj_set_size(click_area, lv_pct(100), lv_pct(100));
        lv_obj_align(click_area, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(click_area, LV_OPA_0, 0);
        lv_obj_set_style_border_width(click_area, 0, 0);
        lv_obj_add_event_cb(click_area, book_event_cb, LV_EVENT_CLICKED, NULL);
        
        /* 存储文件名 */
        char *file_name = strdup(file_name_list[i]);
        if (file_name != NULL) {
            lv_obj_set_user_data(click_area, file_name);
        }
        
        /* 添加文件信息标签 */
        lv_obj_t *file_info = lv_label_create(book_cont);
        char info_text[32];
        snprintf(info_text, sizeof(info_text), "文件 #%zu", i + 1);
        lv_label_set_text(file_info, info_text);
        lv_obj_align(file_info, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_text_color(file_info, lv_color_hex(0x777777), 0);
        if (reader_font != NULL) {
            lv_obj_set_style_text_font(file_info, reader_font, 0);
        } else {
            lv_obj_set_style_text_font(file_info, &lv_font_montserrat_14, 0);
        }
    }
    
    /* 添加分页导航区域 */
    if (file_list_total_pages > 1) {
        /* 创建底部导航栏 */
        lv_obj_t *nav_bar = lv_obj_create(file_list_screen);
        lv_obj_set_size(nav_bar, lv_pct(100), 60);
        lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(nav_bar, lv_color_hex(0xECF0F1), 0);
        lv_obj_set_style_bg_opa(nav_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(nav_bar, 0, 0);
        lv_obj_set_style_radius(nav_bar, 0, 0);
        
        /* 添加上一页按钮 */
        lv_obj_t *prev_btn = lv_btn_create(nav_bar);
        lv_obj_set_size(prev_btn, 80, 40);
        lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 20, 0);
        lv_obj_set_style_bg_color(prev_btn, COLOR_BUTTON, 0);
        lv_obj_set_style_radius(prev_btn, 20, 0);
        
        lv_obj_t *prev_label = lv_label_create(prev_btn);
        lv_label_set_text(prev_label, LV_SYMBOL_LEFT);
        lv_obj_center(prev_label);
        lv_obj_set_style_text_color(prev_label, COLOR_BUTTON_TEXT, 0);
        
        /* 添加事件处理 */
        lv_obj_add_event_cb(prev_btn, file_list_prev_page, LV_EVENT_CLICKED, NULL);
        
        /* 添加下一页按钮 */
        lv_obj_t *next_btn = lv_btn_create(nav_bar);
        lv_obj_set_size(next_btn, 80, 40);
        lv_obj_align(next_btn, LV_ALIGN_RIGHT_MID, -20, 0);
        lv_obj_set_style_bg_color(next_btn, COLOR_BUTTON, 0);
        lv_obj_set_style_radius(next_btn, 20, 0);
        
        lv_obj_t *next_label = lv_label_create(next_btn);
        lv_label_set_text(next_label, LV_SYMBOL_RIGHT);
        lv_obj_center(next_label);
        lv_obj_set_style_text_color(next_label, COLOR_BUTTON_TEXT, 0);
        
        /* 添加事件处理 */
        lv_obj_add_event_cb(next_btn, file_list_next_page, LV_EVENT_CLICKED, NULL);
        
        /* 显示页码信息 */
        lv_obj_t *page_info = lv_label_create(nav_bar);
        char page_text[32];
        snprintf(page_text, sizeof(page_text), "%zu/%zu", file_list_current_page + 1, file_list_total_pages);
        lv_label_set_text(page_info, page_text);
        lv_obj_align(page_info, LV_ALIGN_CENTER, 0, 0);
        if (reader_font != NULL) {
            lv_obj_set_style_text_font(page_info, reader_font, 0);
        } else {
            lv_obj_set_style_text_font(page_info, &lv_font_montserrat_16, 0);
        }
    }
    
    printf("文件列表：显示第 %zu 页，共 %zu 页，当前页 %zu 文件\n", 
           file_list_current_page + 1, file_list_total_pages, end_idx - start_idx);
}

/**
 * @brief 文件列表上一页按钮回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void file_list_prev_page(lv_event_t *e)
{
    if (file_list_current_page > 0) {
        file_list_current_page--;
        show_file_grid();
    }
}

/**
 * @brief 文件列表下一页按钮回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void file_list_next_page(lv_event_t *e)
{
    if (file_list_current_page < file_list_total_pages - 1) {
        file_list_current_page++;
        show_file_grid();
    }
}

/**
 * @brief 书本点击事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void book_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == NULL) return;
    
    /* 获取文件名 */
    char *file_name = lv_obj_get_user_data(target);
    if (file_name == NULL) return;
    
    /* 构建完整文件路径 */
    snprintf(current_file_path, sizeof(current_file_path), "0:/%s", file_name);
    
    /* 打开阅读器 */
    open_reader(current_file_path);
}

/**
 * @brief 返回按钮事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void back_btn_event_cb(lv_event_t *e)
{
    /* 释放文本缓冲区 */
    if (text_buffer != NULL) {
        free(text_buffer);
        text_buffer = NULL;
        text_buffer_size = 0;
    }
    
    /* 关闭文件 */
    if (file_opened) {
        lv_fs_close(&novel_file);
        file_opened = false;
    }
    
    /* 返回文件列表屏幕 */
    lv_scr_load(file_list_screen);
    
    /* 释放文件列表资源，然后重新加载 */
    free_text_file_list_resources();
    
    /* 刷新文件列表 */
    show_file_grid();
}

/**
 * @brief 阅读器手势事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void reader_event_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    
    if (dir == LV_DIR_LEFT) {
        /* 向左滑动，显示下一页 */
        if (current_page < total_pages - 1) {
            current_page++;
            printf("向左滑动，切换到下一页: %zu\n", current_page + 1);
            display_current_page();
        } else {
            printf("已经是最后一页\n");
        }
    } else if (dir == LV_DIR_RIGHT) {
        /* 向右滑动，显示上一页 */
        if (current_page > 0) {
            current_page--;
            printf("向右滑动，切换到上一页: %zu\n", current_page + 1);
            display_current_page();
        } else {
            printf("已经是第一页\n");
        }
    }
}

/**
 * @brief 计算页面布局参数
 * 
 * @return 无
 */
static void calculate_page_layout(void)
{
    if (reader_font == NULL) {
        chars_per_page = 500; /* 默认值 */
        return;
    }

    /* 获取容器尺寸 */
    lv_obj_t *reader_cont = lv_obj_get_parent(reader_label);
    lv_coord_t cont_width = lv_obj_get_width(reader_cont);
    lv_coord_t cont_height = lv_obj_get_height(reader_cont);
    
    /* 获取字体信息 */
    lv_coord_t font_height = reader_font->line_height;
    
    /* 使用测试文本估算平均字符宽度 */
    const char *test_text = "测试文字ABCabc123";
    lv_point_t size;
    lv_txt_get_size(&size, test_text, reader_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_coord_t font_width = size.x / strlen(test_text);
    
    /* 确保字体宽度有合理的值 */
    if (font_width <= 0) font_width = 16;
    
    /* 计算每行字符数和行数 */
    uint16_t chars_per_line = (cont_width - 20) / font_width; /* 减少每行字符数，留出边距 */
    uint16_t lines_per_page = (cont_height - 20) / (font_height + 2); /* 考虑行间距 */
    
    /* 计算每页字符数 - 添加安全边距 */
    chars_per_page = chars_per_line * lines_per_page;
    
    /* 减少5%的字符数以确保页面不会溢出 */
    chars_per_page = (chars_per_page * 95) / 100;
    
    /* 确保有合理的值 */
    if (chars_per_page < 100) chars_per_page = 500;
    
    printf("页面布局计算: 宽度=%d, 高度=%d, 字体高=%d, 字体宽=%d\n", 
           cont_width, cont_height, font_height, font_width);
    printf("每行字符数=%d, 每页行数=%d, 每页字符数=%zu\n", 
           chars_per_line, lines_per_page, chars_per_page);
}

/**
 * @brief 打开阅读器
 * 
 * @param file_path 文件路径
 * @return 无
 */
static void open_reader(const char *file_path)
{
    lv_fs_res_t res;
    
    /* 关闭之前可能打开的文件 */
    if (file_opened) {
        lv_fs_close(&novel_file);
        file_opened = false;
    }
    
    /* 释放之前的缓冲区 */
    if (text_buffer != NULL) {
        free(text_buffer);
        text_buffer = NULL;
        text_buffer_size = 0;
    }
    
    /* 重置加载页面范围 */
    loaded_start_page = 0;
    loaded_end_page = 0;
    
    /* 打开文件 */
    res = lv_fs_open(&novel_file, file_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        printf("打开文件失败: %s, 错误码: %d\n", file_path, res);
        return;
    }
    file_opened = true;
    
    /* 获取文件大小 */
    uint32_t file_size_u32 = 0;
    lv_fs_seek(&novel_file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&novel_file, &file_size_u32);
    file_size = (size_t)file_size_u32;
    lv_fs_seek(&novel_file, 0, LV_FS_SEEK_SET);
    
    printf("文件大小: %zu 字节\n", file_size);
    
    /* 计算页面布局 */
    calculate_page_layout();
    
    /* 计算总页数 */
    total_pages = (file_size + chars_per_page - 1) / chars_per_page;
    if (total_pages == 0) total_pages = 1;
    
    printf("每页字符数: %zu, 总页数: %zu\n", chars_per_page, total_pages);
    
    /* 显示第一页 */
    current_page = 0;
    
    /* 确保加载第一页内容 */
    if (load_page_content(current_page)) {
        display_current_page();
    } else {
        printf("加载第一页失败\n");
        /* 显示加载失败信息 */
        lv_label_set_text(reader_label, "加载文件失败，请检查文件是否有效。");
    }
    
    /* 更新进度条位置 */
    lv_obj_t *jump_cont = lv_obj_get_child(reader_screen, 2);
    if (jump_cont) {
        lv_obj_t *slider = lv_obj_get_child(jump_cont, 0);
        if (slider) {
            lv_slider_set_value(slider, 0, LV_ANIM_OFF);
        }
    }
    
    /* 显示阅读屏幕 */
    lv_scr_load(reader_screen);
}

/**
 * @brief 加载指定页及其前后几页的内容
 * 
 * @param page_num 页码
 * @return 成功返回true，失败返回false
 */
static bool load_page_content(size_t page_num)
{
    if (!file_opened) {
        printf("文件未打开，无法加载内容\n");
        return false;
    }
    
    /* 检查页码是否已经在缓存区域内 */
    if (text_buffer != NULL && page_num >= loaded_start_page && page_num <= loaded_end_page) {
        /* 当前页已加载，无需重新加载 */
        printf("当前页 %zu 已在缓存中，无需重新加载\n", page_num + 1);
        return true;
    }
    
    /* 计算要加载的起始页和结束页 - 减少预加载页数以节省内存 */
    size_t start_page = (page_num > 1) ? (page_num - 1) : 0;
    size_t end_page = (page_num + 1 < total_pages) ? (page_num + 1) : (total_pages - 1);
    
    /* 计算文件中的起始位置和结束位置 */
    size_t start_pos = start_page * chars_per_page;
    size_t end_pos = (end_page + 1) * chars_per_page;
    if (end_pos > file_size) end_pos = file_size;
    
    /* 计算要读取的字节数 */
    size_t bytes_to_read = end_pos - start_pos;
    
    printf("加载页面 %zu 到 %zu，文件位置 %zu 到 %zu，读取 %zu 字节\n", 
           start_page + 1, end_page + 1, start_pos, end_pos, bytes_to_read);
    
    if (bytes_to_read == 0) {
        printf("警告：要读取的字节数为0\n");
        return false;
    }
    
    /* 先释放旧缓冲区 */
    if (text_buffer != NULL) {
        free(text_buffer);
        text_buffer = NULL;
        text_buffer_size = 0;
    }
    
    /* 分配新缓冲区，使用PSRAM如果可用 */
    text_buffer = heap_caps_malloc(bytes_to_read + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (text_buffer == NULL) {
        printf("PSRAM分配失败，尝试使用普通内存\n");
        text_buffer = malloc(bytes_to_read + 1);
    }
    
    if (text_buffer == NULL) {
        printf("内存分配失败，请求大小: %zu 字节\n", bytes_to_read + 1);
        return false;
    }
    
    text_buffer_size = bytes_to_read;
    printf("调整缓冲区大小为 %zu 字节\n", text_buffer_size);
    
    /* 定位到文件中的起始位置 */
    lv_fs_res_t res = lv_fs_seek(&novel_file, start_pos, LV_FS_SEEK_SET);
    if (res != LV_FS_RES_OK) {
        printf("文件定位失败, 错误码: %d, 位置: %zu\n", res, start_pos);
        return false;
    }
    
    /* 读取内容 */
    uint32_t bytes_read = 0;
    res = lv_fs_read(&novel_file, text_buffer, bytes_to_read, &bytes_read);
    if (res != LV_FS_RES_OK) {
        printf("读取文件失败, 错误码: %d\n", res);
        return false;
    }
    
    if (bytes_read != bytes_to_read) {
        printf("警告: 实际读取字节数 (%lu) 与请求字节数 (%zu) 不符\n", bytes_read, bytes_to_read);
    }
    
    /* 添加字符串结束符 */
    text_buffer[bytes_read] = '\0';
    
    /* 存储当前加载的页面范围信息 */
    loaded_start_page = start_page;
    loaded_end_page = end_page;
    
    printf("已成功加载页面 %zu 到 %zu (总页数: %zu)，读取了 %lu 字节\n", 
           loaded_start_page + 1, loaded_end_page + 1, total_pages, bytes_read);
    return true;
}

/**
 * @brief 显示当前页内容
 * 
 * @return 无
 */
static void display_current_page(void)
{
    if (!file_opened) {
        printf("文件未打开，无法显示内容\n");
        return;
    }
    
    if (text_buffer == NULL || text_buffer_size == 0) {
        printf("缓冲区为空，无法显示内容\n");
        return;
    }
    
    /* 检查页码是否有效 */
    if (current_page >= total_pages) {
        printf("页码超出范围: %zu/%zu\n", current_page + 1, total_pages);
        current_page = total_pages - 1;
    }
    
    /* 检查是否需要加载新内容 */
    if (current_page < loaded_start_page || current_page > loaded_end_page) {
        printf("当前页 %zu 不在已加载范围 %zu-%zu 内，重新加载...\n", 
               current_page + 1, loaded_start_page + 1, loaded_end_page + 1);
        
        if (!load_page_content(current_page)) {
            printf("加载页面失败\n");
            lv_label_set_text(reader_label, "加载页面失败，请尝试返回重新打开。");
            return;
        }
    }
    
    /* 计算当前页在缓冲区中的偏移 */
    size_t buffer_offset = (current_page - loaded_start_page) * chars_per_page;
    
    /* 确保偏移量不超过缓冲区大小 */
    if (buffer_offset >= text_buffer_size) {
        printf("错误：缓冲区偏移量 (%zu) 超出缓冲区大小 (%zu)\n", buffer_offset, text_buffer_size);
        lv_label_set_text(reader_label, "缓冲区偏移错误，请尝试返回重新打开。");
        return;
    }
    
    /* 计算当前页的内容长度 */
    size_t content_length = chars_per_page;
    if (buffer_offset + content_length > text_buffer_size) {
        content_length = text_buffer_size - buffer_offset;
    }
    
    printf("当前页 %zu：缓冲区偏移 %zu，内容长度 %zu\n", 
           current_page + 1, buffer_offset, content_length);
    
    /* 使用堆内存分配存储页面内容，避免栈溢出 */
    char *page_content = heap_caps_malloc(content_length + 1, MALLOC_CAP_SPIRAM);
    if (page_content == NULL) {
        /* 如果PSRAM分配失败，尝试普通内存 */
        page_content = malloc(content_length + 1);
        if (page_content == NULL) {
            printf("页面内容内存分配失败\n");
            lv_label_set_text(reader_label, "内存不足，无法显示内容。");
            return;
        }
    }
    
    /* 确保不会溢出 */
    if (content_length >= 4096) {
        content_length = 4096 - 1;
    }
    
    /* 复制当前页内容 */
    memcpy(page_content, text_buffer + buffer_offset, content_length);
    page_content[content_length] = '\0';
    
    /* 优化文本显示 - 尝试在段落末尾截断而不是随机位置 */
    if (content_length > 10) {
        size_t end_pos = content_length - 1;
        
        /* 从后向前查找段落结束位置 */
        size_t i;
        for (i = 0; i < 50 && end_pos > content_length/2; i++) {
            unsigned char c = (unsigned char)page_content[end_pos];
            /* 检查ASCII标点符号 */
            if (page_content[end_pos] == '\n' || 
                page_content[end_pos] == '.') {
                end_pos++;  /* 包含结束符号 */
                break;
            }
            /* 检查UTF-8中文标点符号 */
            else if (c >= 0xE0 && end_pos >= 2) {
                /* 可能是中文标点 */
                if ((page_content[end_pos-2] == '\xe3' && page_content[end_pos-1] == '\x80' && page_content[end_pos] == '\x82') || /* 句号 */
                    (page_content[end_pos-2] == '\xe9' && page_content[end_pos-1] == '\x97' && page_content[end_pos] == '\x9c') || /* 感叹号 */
                    (page_content[end_pos-2] == '\xef' && page_content[end_pos-1] == '\xbc' && page_content[end_pos] == '\x9f')) { /* 问号 */
                    end_pos++;  /* 包含结束符号 */
                    break;
                }
            }
            end_pos--;
        }
        
        /* 如果找到合适的结束位置，则截断文本 */
        if (i < 50 && end_pos > content_length/2) {
            page_content[end_pos] = '\0';
        }
    }
    
    /* 在控制台打印一部分内容，以便调试 */
    printf("页面内容预览: %.50s...\n", page_content);
    
    /* 显示当前页内容 */
    if (reader_label != NULL) {
        lv_label_set_text(reader_label, page_content);
        
        /* 显示页码信息 - 这里可能导致崩溃，使用更安全的方式创建和更新页码标签 */
        static lv_obj_t *page_label = NULL;
        
        if (page_label == NULL) {
            /* 首次创建页码标签 */
            page_label = lv_label_create(reader_screen);
            lv_obj_align(page_label, LV_ALIGN_TOP_RIGHT, -10, 10);
            if (reader_font != NULL) {
                lv_obj_add_style(page_label, &reader_style, 0);
            }
        }
        
        /* 更新页码文本 */
        char page_info[32];
        snprintf(page_info, sizeof(page_info), "%zu/%zu", current_page + 1, total_pages);
        lv_label_set_text(page_label, page_info);
        
        /* 更新进度条位置 */
        lv_obj_t *jump_cont = lv_obj_get_child(reader_screen, 2);
        if (jump_cont) {
            lv_obj_t *slider = lv_obj_get_child(jump_cont, 0);
            if (slider) {
                int32_t slider_value = (current_page * 100) / (total_pages > 1 ? total_pages - 1 : 1);
                lv_slider_set_value(slider, slider_value, LV_ANIM_OFF);
            }
        }
        
        printf("显示第 %zu 页，总页数 %zu\n", current_page + 1, total_pages);
    } else {
        printf("错误：阅读标签为NULL\n");
    }
    
    /* 释放页面内容缓冲区 */
    free(page_content);
}

/**
 * @brief 从SD卡加载字体
 * 
 * @return 加载成功返回字体指针，失败返回NULL
 */
static lv_font_t *load_font_from_sd(void)
{
    /* 加载配置文件 */
    load_font_config();
    
    /* 加载指定大小的字体 */
    return load_font_with_size(font_size);
}

/**
 * @brief 保存字体设置到配置文件
 * 
 * @return 成功返回true，失败返回false
 */
static bool save_font_config(void)
{
    lv_fs_file_t file;
    lv_fs_res_t res;
    char buffer[512];
    uint32_t bytes_written = 0;
    
    /* 打开配置文件用于写入 */
    res = lv_fs_open(&file, font_config_path, LV_FS_MODE_WR);
    if (res != LV_FS_RES_OK) {
        printf("无法打开配置文件进行写入: %s\n", font_config_path);
        return false;
    }
    
    /* 将字体大小和字体名称写入文件 */
    snprintf(buffer, sizeof(buffer), "%d\n%s", font_size, current_font_name);
    res = lv_fs_write(&file, buffer, strlen(buffer), &bytes_written);
    lv_fs_close(&file);
    
    if (res != LV_FS_RES_OK || bytes_written != strlen(buffer)) {
        printf("写入配置文件失败\n");
        return false;
    }
    
    printf("字体设置已保存: 大小=%d, 字体=%s\n", font_size, current_font_name);
    return true;
}

/**
 * @brief 从配置文件加载字体设置
 * 
 * @return 成功返回true，失败返回false
 */
static bool load_font_config(void)
{
    lv_fs_file_t file;
    lv_fs_res_t res;
    char buffer[512] = {0};
    uint32_t bytes_read = 0;
    
    /* 打开配置文件用于读取 */
    res = lv_fs_open(&file, font_config_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        printf("配置文件不存在，使用默认字体设置\n");
        return false;
    }
    
    /* 读取配置内容 */
    res = lv_fs_read(&file, buffer, sizeof(buffer) - 1, &bytes_read);
    lv_fs_close(&file);
    
    if (res != LV_FS_RES_OK || bytes_read == 0) {
        printf("读取配置文件失败，使用默认字体设置\n");
        return false;
    }
    
    /* 确保字符串以NULL结尾 */
    buffer[bytes_read] = '\0';
    
    /* 解析字体大小和字体名称 */
    char *line = strtok(buffer, "\n");
    if (line) {
        int temp_size = atoi(line);
        if (temp_size > 0) {
            /* 验证字体大小是否在有效范围内 */
            bool valid_size = false;
            for (uint8_t i = 0; i < font_sizes_count; i++) {
                if (temp_size == font_sizes[i]) {
                    valid_size = true;
                    break;
                }
            }
            
            if (valid_size) {
                font_size = temp_size;
                printf("从配置文件加载字体大小: %d\n", font_size);
            }
        }
        
        /* 读取字体名称 */
        line = strtok(NULL, "\n");
        if (line) {
            strncpy(current_font_name, line, sizeof(current_font_name) - 1);
            current_font_name[sizeof(current_font_name) - 1] = '\0';
            printf("从配置文件加载字体名称: %s\n", current_font_name);
            return true;
        }
    }
    
    printf("配置文件格式无效，使用默认字体设置\n");
    return false;
}

/**
 * @brief 扫描字体文件
 * 
 * @return 无
 */
static void scan_font_files(void)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res;
    char fn[256];
    
    /* 释放之前的字体名列表 */
    free_font_list_resources();
    
    /* 打开SD卡根目录 */
    res = lv_fs_dir_open(&dir, "0:/");
    if (res != LV_FS_RES_OK) {
        printf("无法打开根目录, 错误码: %d\n", res);
        return;
    }
    
    /* 先计算字体文件总数 */
    size_t font_count = 0;
    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK || fn[0] == '\0') {
            break; /* 没有更多文件或出错 */
        }
        
        /* 检查是否为字体文件 */
        size_t len = strlen(fn);
        if (len > 4) {
            if ((strcmp(&fn[len-4], ".ttf") == 0) || 
                (strcmp(&fn[len-4], ".TTF") == 0) ||
                (strcmp(&fn[len-4], ".otf") == 0) || 
                (strcmp(&fn[len-4], ".OTF") == 0) ||
                (strcmp(&fn[len-4], ".ttc") == 0) ||
                (strcmp(&fn[len-4], ".TTC") == 0)) {
                font_count++;
            }
        }
    }
    
    /* 分配字体名列表内存 */
    if (font_count > 0) {
        font_file_list = (char **)malloc(sizeof(char *) * font_count);
        if (font_file_list == NULL) {
            printf("内存分配失败\n");
            lv_fs_dir_close(&dir);
            return;
        }
        memset(font_file_list, 0, sizeof(char *) * font_count);
    }
    
    /* 重新打开目录 */
    lv_fs_dir_close(&dir);
    res = lv_fs_dir_open(&dir, "0:/");
    if (res != LV_FS_RES_OK) {
        printf("无法打开根目录, 错误码: %d\n", res);
        return;
    }
    
    /* 收集所有字体文件名 */
    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK || fn[0] == '\0') {
            break; /* 没有更多文件或出错 */
        }
        
        /* 检查是否为字体文件 */
        size_t len = strlen(fn);
        if (len > 4) {
            if ((strcmp(&fn[len-4], ".ttf") == 0) || 
                (strcmp(&fn[len-4], ".TTF") == 0) ||
                (strcmp(&fn[len-4], ".otf") == 0) || 
                (strcmp(&fn[len-4], ".OTF") == 0) ||
                (strcmp(&fn[len-4], ".ttc") == 0) ||
                (strcmp(&fn[len-4], ".TTC") == 0)) {
                
                /* 保存字体名 */
                font_file_list[font_file_count] = (char *)malloc(len + 1);
                if (font_file_list[font_file_count] != NULL) {
                    strcpy(font_file_list[font_file_count], fn);
                    font_file_count++;
                }
            }
        }
    }
    
    /* 关闭目录 */
    lv_fs_dir_close(&dir);
    
    printf("找到 %zu 个字体文件\n", font_file_count);
    
    /* 如果配置的字体名为空，使用第一个找到的字体 */
    if (current_font_name[0] == '\0' && font_file_count > 0) {
        strcpy(current_font_name, font_file_list[0]);
        current_font_index = 0;
        printf("使用默认字体: %s\n", current_font_name);
    } else if (font_file_count > 0) {
        /* 查找配置的字体在列表中的索引 */
        current_font_index = -1;
        for (size_t i = 0; i < font_file_count; i++) {
            if (strcmp(current_font_name, font_file_list[i]) == 0) {
                current_font_index = i;
                break;
            }
        }
        
        /* 如果找不到配置的字体，使用第一个 */
        if (current_font_index < 0) {
            strcpy(current_font_name, font_file_list[0]);
            current_font_index = 0;
            printf("配置的字体不存在，使用默认字体: %s\n", current_font_name);
        }
    }
}

/**
 * @brief 释放字体列表资源
 * 
 * @return 无
 */
static void free_font_list_resources(void)
{
    /* 释放字体名列表 */
    if (font_file_list != NULL) {
        for (size_t i = 0; i < font_file_count; i++) {
            if (font_file_list[i] != NULL) {
                free(font_file_list[i]);
            }
        }
        free(font_file_list);
        font_file_list = NULL;
        font_file_count = 0;
    }
}

/**
 * @brief 字体下拉框事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void font_dd_event_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    int selected = lv_dropdown_get_selected(dd);
    
    /* 更新当前字体 */
    if (selected >= 0 && selected < font_file_count) {
        strcpy(current_font_name, font_file_list[selected]);
        current_font_index = selected;
        printf("选择字体: %s (索引: %d)\n", current_font_name, current_font_index);
    }
}

/**
 * @brief 加载指定大小的字体
 * 
 * @param size 字体大小
 * @return 成功返回字体指针，失败返回NULL
 */
static lv_font_t *load_font_with_size(uint8_t size)
{
    /* 如果已有字体，先释放 */
    if (font_data != NULL) {
        free(font_data);
        font_data = NULL;
    }
    
    /* 如果没有选择字体或字体不存在，尝试扫描字体 */
    if (current_font_name[0] == '\0' || current_font_index < 0) {
        scan_font_files();
    }
    
    /* 如果仍然没有字体，返回NULL */
    if (current_font_name[0] == '\0' || font_file_count == 0) {
        printf("未找到任何字体文件\n");
        return NULL;
    }
    
    /* 尝试加载当前选择的字体文件 */
    char font_path[270] = {0};
    snprintf(font_path, sizeof(font_path), "0:/%s", current_font_name);
    printf("尝试加载字体: %s, 大小: %d\n", font_path, size);
    
    /* 读取字体文件 */
    lv_fs_file_t fd;
    lv_fs_res_t res = lv_fs_open(&fd, font_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        printf("打开字体文件失败, 错误码: %d\n", res);
        return NULL;
    }
    
    /* 获取文件大小 */
    uint32_t file_size = 0;
    lv_fs_seek(&fd, 0, LV_FS_SEEK_END);
    lv_fs_tell(&fd, &file_size);
    lv_fs_seek(&fd, 0, LV_FS_SEEK_SET);
    
    printf("字体文件大小: %lu 字节\n", file_size);
    
    /* 分配内存读取字体文件 - 必须使用PSRAM */
    font_data = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);
    if (font_data == NULL) {
        printf("PSRAM分配失败，尝试更小的字体\n");
        lv_fs_close(&fd);
        return NULL;
    }
    printf("内存分配成功\n");
    
    /* 读取字体文件到内存 */
    uint32_t bytes_read = 0;
    printf("开始读取字体文件\n");
    res = lv_fs_read(&fd, font_data, file_size, &bytes_read);
    lv_fs_close(&fd);
    printf("结束读取字体文件\n");
    
    if (res != LV_FS_RES_OK || bytes_read != file_size) {
        printf("读取字体文件失败, 已读: %lu, 应读: %lu\n", bytes_read, file_size);
        free(font_data);
        font_data = NULL;
        return NULL;
    }
    
    printf("初始化FreeType\n");
    /* 初始化FreeType - 增加缓存大小 */
    if (!lv_freetype_init(16, 4, 0)) {
        printf("FreeType初始化失败\n");
        free(font_data);
        font_data = NULL;
        return NULL;
    }
    printf("FreeType初始化成功\n");
    
    /* 加载字体 */
    static lv_ft_info_t info;
    memset(&info, 0, sizeof(info));
    info.name = "novel_font";
    info.mem = font_data;
    info.mem_size = file_size;
    info.weight = size;  /* 使用传入的字体大小 */
    info.style = FT_FONT_STYLE_NORMAL;
    
    printf("开始加载字体\n");
    if (!lv_ft_font_init(&info)) {
        printf("FreeType字体加载失败\n");
        free(font_data);
        font_data = NULL;
        return NULL;
    }
    
    printf("字体加载成功! 大小: %d\n", size);
    return info.font;
}

/**
 * @brief 创建设置界面
 * 
 * @return 无
 */
static void create_settings_screen(void)
{
    /* 创建设置屏幕 */
    settings_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_screen, COLOR_BACKGROUND, 0);
    
    /* 创建顶部状态栏 */
    lv_obj_t *top_bar = lv_obj_create(settings_screen);
    lv_obj_set_size(top_bar, lv_pct(100), 60);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    
    /* 创建标题 */
    lv_obj_t *title = lv_label_create(top_bar);
    lv_label_set_text(title, "字体设置");
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(title, reader_font, 0);
    }
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_center(title);
    
    /* 创建设置内容容器 */
    lv_obj_t *settings_cont = lv_obj_create(settings_screen);
    lv_obj_set_size(settings_cont, lv_pct(90), lv_pct(70));
    lv_obj_align(settings_cont, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(settings_cont, lv_color_white(), 0);
    lv_obj_set_style_radius(settings_cont, 10, 0);
    lv_obj_set_style_border_width(settings_cont, 0, 0);
    lv_obj_set_style_shadow_width(settings_cont, 15, 0);
    lv_obj_set_style_shadow_ofs_y(settings_cont, 5, 0);
    lv_obj_set_style_shadow_opa(settings_cont, LV_OPA_20, 0);
    
    /* 扫描字体文件 */
    scan_font_files();
    
    /* 创建字体选择标签 */
    lv_obj_t *font_select_label = lv_label_create(settings_cont);
    lv_label_set_text(font_select_label, "选择字体");
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(font_select_label, reader_font, 0);
    }
    lv_obj_align(font_select_label, LV_ALIGN_TOP_LEFT, 20, 20);
    
    /* 创建字体下拉列表 */
    lv_obj_t *font_dd = lv_dropdown_create(settings_cont);
    lv_obj_set_size(font_dd, lv_pct(80), 40);
    lv_obj_align(font_dd, LV_ALIGN_TOP_MID, 0, 50);
    
    /* 填充字体列表 */
    if (font_file_count > 0) {
        /* 创建字体名称字符串 */
        char *font_options = (char *)malloc(font_file_count * 64);
        if (font_options) {
            font_options[0] = '\0';
            for (size_t i = 0; i < font_file_count; i++) {
                strcat(font_options, font_file_list[i]);
                if (i < font_file_count - 1) {
                    strcat(font_options, "\n");
                }
            }
            lv_dropdown_set_options(font_dd, font_options);
            free(font_options);
        }
        
        /* 设置当前选择的字体 */
        lv_dropdown_set_selected(font_dd, current_font_index);
    } else {
        lv_dropdown_set_options(font_dd, "无可用字体");
        lv_obj_add_state(font_dd, LV_STATE_DISABLED);
    }
    
    /* 添加下拉列表事件 */
    lv_obj_add_event_cb(font_dd, font_dd_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    /* 创建字体大小标签 */
    lv_obj_t *font_size_label = lv_label_create(settings_cont);
    lv_label_set_text(font_size_label, "字体大小");
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(font_size_label, reader_font, 0);
    }
    lv_obj_align(font_size_label, LV_ALIGN_TOP_LEFT, 20, 110);
    
    /* 创建字体大小数值标签 */
    lv_obj_t *font_size_value = lv_label_create(settings_cont);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d px", font_size);
    lv_label_set_text(font_size_value, buf);
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(font_size_value, reader_font, 0);
    }
    lv_obj_align(font_size_value, LV_ALIGN_TOP_MID, 0, 140);
    
    /* 创建字体大小滑块 */
    lv_obj_t *font_size_slider = lv_slider_create(settings_cont);
    lv_obj_set_width(font_size_slider, lv_pct(80));
    lv_obj_align(font_size_slider, LV_ALIGN_TOP_MID, 0, 180);
    lv_slider_set_range(font_size_slider, 0, font_sizes_count - 1);
    
    /* 设置当前字体大小对应的滑块位置 */
    int current_index = 0;
    for (uint8_t i = 0; i < font_sizes_count; i++) {
        if (font_size == font_sizes[i]) {
            current_index = i;
            break;
        }
    }
    lv_slider_set_value(font_size_slider, current_index, LV_ANIM_OFF);
    
    /* 添加滑块事件 */
    lv_obj_add_event_cb(font_size_slider, font_size_slider_event_cb, LV_EVENT_VALUE_CHANGED, font_size_value);
    
    /* 创建按钮容器 */
    lv_obj_t *btn_cont = lv_obj_create(settings_screen);
    lv_obj_set_size(btn_cont, lv_pct(90), 60);
    lv_obj_align(btn_cont, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    
    /* 创建保存按钮 */
    lv_obj_t *save_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(save_btn, 120, 50);
    lv_obj_align(save_btn, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_color(save_btn, COLOR_BUTTON, 0);
    lv_obj_set_style_radius(save_btn, 10, 0);
    lv_obj_add_event_cb(save_btn, settings_save_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "保存");
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(save_label, reader_font, 0);
    }
    lv_obj_set_style_text_color(save_label, COLOR_BUTTON_TEXT, 0);
    lv_obj_center(save_label);
    
    /* 创建取消按钮 */
    lv_obj_t *cancel_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(cancel_btn, 120, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xE74C3C), 0);
    lv_obj_set_style_radius(cancel_btn, 10, 0);
    lv_obj_add_event_cb(cancel_btn, settings_cancel_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "取消");
    if (reader_font != NULL) {
        lv_obj_set_style_text_font(cancel_label, reader_font, 0);
    }
    lv_obj_set_style_text_color(cancel_label, COLOR_BUTTON_TEXT, 0);
    lv_obj_center(cancel_label);
}

/**
 * @brief 字体大小滑块事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void font_size_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    
    /* 获取字体大小索引 */
    uint8_t index = (uint8_t)value;
    if (index >= font_sizes_count) {
        index = font_sizes_count - 1;
    }
    
    /* 更新字体大小 */
    font_size = font_sizes[index];
    
    /* 更新显示 */
    lv_obj_t *value_label = lv_event_get_user_data(e);
    if (value_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d px", font_size);
        lv_label_set_text(value_label, buf);
    }
}

/**
 * @brief 设置保存按钮事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void settings_save_btn_event_cb(lv_event_t *e)
{
    /* 保存设置 */
    save_font_config();
    
    /* 重新加载字体并应用到界面 */
    if (reader_font != NULL) {
        /* 释放之前的字体资源 */
        /* 注意：LVGL不提供直接释放字体的API，这里我们只是替换引用 */
    }
    
    /* 重新加载字体 */
    reader_font = load_font_with_size(font_size);
    
    /* 更新各个样式中的字体 */
    if (reader_font != NULL) {
        lv_style_set_text_font(&reader_style, reader_font);
        lv_style_set_text_font(&book_style, reader_font);
        lv_style_set_text_font(&title_style, reader_font);
        
        /* 更新标题标签字体 */
        if (title_label != NULL) {
            lv_obj_refresh_style(title_label, LV_PART_MAIN, LV_STYLE_TEXT_FONT);
        }
        
        /* 更新阅读标签字体 */
        if (reader_label != NULL) {
            lv_obj_refresh_style(reader_label, LV_PART_MAIN, LV_STYLE_TEXT_FONT);
        }
        
        /* 如果当前在阅读屏幕，重新计算布局并显示内容 */
        if (lv_scr_act() == reader_screen) {
            calculate_page_layout();
            /* 重新计算总页数 */
            if (file_opened && file_size > 0) {
                total_pages = (file_size + chars_per_page - 1) / chars_per_page;
                if (total_pages == 0) total_pages = 1;
                
                /* 确保当前页有效 */
                if (current_page >= total_pages) {
                    current_page = total_pages - 1;
                }
                
                /* 重新加载并显示当前页 */
                if (load_page_content(current_page)) {
                    display_current_page();
                }
            }
        }
        
        printf("字体已重新加载并应用: %s, 大小: %d\n", current_font_name, font_size);
    } else {
        printf("重新加载字体失败\n");
    }
    
    /* 返回文件列表屏幕 */
    lv_scr_load(file_list_screen);
}

/**
 * @brief 设置取消按钮事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void settings_cancel_btn_event_cb(lv_event_t *e)
{
    /* 取消设置，重新加载配置 */
    load_font_config();
    
    /* 返回文件列表屏幕 */
    lv_scr_load(file_list_screen);
}

/**
 * @brief 设置按钮事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void settings_btn_event_cb(lv_event_t *e)
{
    /* 显示设置屏幕 */
    lv_scr_load(settings_screen);
}

/**
 * @brief 释放文本文件列表资源
 * 
 * @return 无
 */
static void free_text_file_list_resources(void)
{
    /* 释放文件名列表 */
    if (file_name_list != NULL) {
        for (size_t i = 0; i < file_count; i++) {
            if (file_name_list[i] != NULL) {
                free(file_name_list[i]);
            }
        }
        free(file_name_list);
        file_name_list = NULL;
        file_count = 0;
    }
}

/**
 * @brief 进度条事件回调
 * 
 * @param e 事件指针
 * @return 无
 */
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    
    /* 计算对应的页码 */
    size_t new_page = (value * (total_pages - 1)) / 100;
    
    /* 如果页码改变，跳转到新页面 */
    if (new_page != current_page) {
        current_page = new_page;
        display_current_page();
    }
} 