//
// Created by E_LJF on 25-7-10.
//

#include "ws2812b.h"

#include <stdlib.h>
#include "string.h"

#define DATA_LEN       24   // WS2812数据长度，单个需要24个字节


/**
 * 函数：WS2812单灯设置函数
 * 参数：which:灯的位置，R、G、B分别为三个颜色通道的亮度，最大值为255
 * 作用：单独设置每一个WS2812的颜色
***/
void WS2812_Set(WS2812B_OBJ_T* ws2812b,uint16_t which,uint8_t R,uint8_t G,uint8_t B)
{
    which =  (which > ws2812b->num) ? ws2812b->num : which;

    uint32_t indexx=(which *DATA_LEN);

    for (uint8_t i = 0;i < 8;i++)
    {
        //填充数组
        ws2812b->rgb_buff[indexx+i]      = (G << i) & (0x80)?WS_H:WS_L;
        ws2812b->rgb_buff[indexx+i + 8]  = (R << i) & (0x80)?WS_H:WS_L;
        ws2812b->rgb_buff[indexx+i + 16] = (B << i) & (0x80)?WS_H:WS_L;
    }
}

//WS2812初始化函数
//返回 0 初始化成功
//返回 1 初始化失败
uint8_t WS2812_Init(
    WS2812B_OBJ_T* ws2812b, //WS2812B对象
    TIM_HandleTypeDef *htim,//使用的定时器
    uint32_t Channel,       //定时器通道
    uint16_t num            //灯珠数量
    )
{
    uint8_t retn = 1;

    ws2812b->htim = htim;
    ws2812b->Channel = Channel;
    ws2812b->num = num;

    ws2812b->rgb_buff = (uint16_t*)malloc((ws2812b->num*DATA_LEN+WS2812_RST_NUM) * sizeof(uint16_t) );

    if (ws2812b->rgb_buff != NULL) {
        retn = 0;
        //清空显存
        memset(ws2812b->rgb_buff,0,(ws2812b->num*DATA_LEN+WS2812_RST_NUM) * sizeof(uint16_t));
        //设置关闭所有灯
        for(int i=0;i<8;i++)
        {
            WS2812_Set(ws2812b,i, 0, 0, 0);
        }
        //作用：调用DMA将显存中的内容实时搬运至定时器的比较寄存器
        HAL_TIM_PWM_Start_DMA(ws2812b->htim,ws2812b->Channel,(uint32_t *)ws2812b->rgb_buff,(ws2812b->num*DATA_LEN+WS2812_RST_NUM));
    }
    return retn;
}

//删除WS2812b对象
void WS2812_OBJ_DEL(WS2812B_OBJ_T* ws2812b) {
    WS2812_SetAll(ws2812b,0,0,0);
    free(ws2812b->rgb_buff);
}

//关闭WS2812b对象
void WS2812_DISABLE(WS2812B_OBJ_T* ws2812b) {
    WS2812_SetAll(ws2812b,0,0,0);
    HAL_TIM_PWM_Stop_DMA(ws2812b->htim,ws2812b->Channel);
}

//重新开启WS2812b对象
void WS2812_ENABLE(WS2812B_OBJ_T* ws2812b) {
    HAL_TIM_PWM_Start(ws2812b->htim,ws2812b->Channel);
}

//设置所有灯珠颜色
//R,G,B为对应的RGB通道值
void WS2812_SetAll(WS2812B_OBJ_T* ws2812b,uint8_t R,uint8_t G,uint8_t B) {

    for (uint16_t j = 0;j < ws2812b->num;j++) {

        uint32_t indx = j * DATA_LEN;

        for (uint8_t i = 0;i < 8;i++)
        {
            //填充数组
            ws2812b->rgb_buff[indx+i]      = (G << i) & (0x80)?WS_H:WS_L;
            ws2812b->rgb_buff[indx+i + 8]  = (R << i) & (0x80)?WS_H:WS_L;
            ws2812b->rgb_buff[indx+i + 16] = (B << i) & (0x80)?WS_H:WS_L;
        }
    }
}

//点亮指定片段的灯珠
//from      起始位置
//to        末位置
//rgb_arr[] RGB数组,数组内容为[R1,G1,B1 , R2,G2,B2......]
void WS2812_SetFromTo(WS2812B_OBJ_T* ws2812b,uint16_t from,uint16_t to,uint8_t rgb_arr[]) {

    if (from > to) {
        uint16_t buff = from;
        from = to;
        to = buff;
    }
    to = (to > ws2812b->num) ? ws2812b->num : to;

    uint16_t num = 0;

    for (uint16_t j = from;j < (to + 1);j++) {

        uint32_t indx = j * DATA_LEN;

        for (uint8_t i = 0;i < 8;i++)
        {
            //填充数组
            ws2812b->rgb_buff[indx+i]      = (rgb_arr[num + 1] << i) & (0x80)?WS_H:WS_L;
            ws2812b->rgb_buff[indx+i + 8]  = (rgb_arr[num]     << i) & (0x80)?WS_H:WS_L;
            ws2812b->rgb_buff[indx+i + 16] = (rgb_arr[num + 2] << i) & (0x80)?WS_H:WS_L;
        }
        num += 3;
    }

}
