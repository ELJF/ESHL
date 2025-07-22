//
// Created by E_LJF on 25-7-10.
//

#ifndef WS2812B_H
#define WS2812B_H

#include "tim.h"




#define WS_H           41   // 1 码相对计数值
#define WS_L           19   // 0 码相对计数值
#define WS2812_RST_NUM 100   // 官方复位时间为50us（40个周期），保险起见使用50个周期





typedef struct  WS2812B_OBJ {
    TIM_HandleTypeDef *htim;//使用的定时器
    uint32_t Channel;       //定时器通道
    uint16_t num;            //灯珠数量
    uint16_t* rgb_buff;     //显存
}WS2812B_OBJ_T;


uint8_t WS2812_Init(
    WS2812B_OBJ_T* ws2812b, //WS2812B对象
    TIM_HandleTypeDef *htim,//使用的定时器
    uint32_t Channel,       //定时器通道
    uint16_t num            //灯珠数量
    );
void WS2812_Set(WS2812B_OBJ_T* ws2812b,uint16_t which,uint8_t R,uint8_t G,uint8_t B);
void WS2812_SetAll(WS2812B_OBJ_T* ws2812b,uint8_t R,uint8_t G,uint8_t B);
void WS2812_SetFromTo(WS2812B_OBJ_T* ws2812b,uint16_t from,uint16_t to,uint8_t rgb_arr[]);
void WS2812_OBJ_DEL(WS2812B_OBJ_T* ws2812b);
void WS2812_DISABLE(WS2812B_OBJ_T* ws2812b);
void WS2812_ENABLE(WS2812B_OBJ_T* ws2812b);

#endif //WS2812B_H
