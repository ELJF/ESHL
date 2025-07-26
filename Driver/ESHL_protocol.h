//
// Created by E_LJF on 25-7-17.
//


/*电调在通信过程中始终作为从机

构成:
帧头[0]+电调地址[1]+数据包长[3]+命令码[4]+数据[5]+整包CRC16校验码

帧头:
主机→电调:  0XEC
电调→主机:  0XCE

地址:
默认     EC00(可更改)
广播地址  AAAA

命令码:
0XC1:油门命令,表示数据包传输油门百分比,float类型，4字节
0XC2:关闭电调,电调数据0XD0表示电调成功关闭,主机不发送数据,填0,uint8_t,类型,1字节
0XC3:启动电调,数据0XA1表示顺时针启动,0XA2表示逆时针启动,uint8_t类型,1字节
0XC4:电调刹车,电调数据0XA3表示刹车完毕,主机不发送数据,填0,uint8_t,类型,1字节
0XC5:更改电调运行电流限制ADC值,uint16_t类型,电调回复0xAA表示更改成功
0XC6:电调异常,表示数据包传输电调异常代码，uint8_t类型，1字节
0XC7:更改电调地址,主机用广播地址发送此命令,更改地址时,所有已连接的电调地址都需要重新设置
     地址设置方法: 快速旋转已经连接上电调的电机,此时电调将自动将自身地址设为EC00
     同时电调状态指示灯会闪烁,闪烁次数代表电调地址
     此时旋转第二个电调的电机,此时电调将自动将自身地址设为EC01,以此类推,直到所有电调地址设置完毕
     电调设置地址后需要重新上电才能使用
     设置地址主机不发送数据,电调广播自身地址表示该地址已被占用

电调异常代码:
0XE0:MOS异常
0XE1:电流异常
0XE2:电调开环启动达到最大重启次数,开环启动失败
0XE3:电机运行过程中意外停转
0XE4:电池电压异常*/


#ifndef ESHL_PROTOCOL_H
#define ESHL_PROTOCOL_H

#include "main.h"

#define ESHL_PROTOCOL_HEAD_ESC_TO_HOST 0xCE     //数据从电调发往主机帧头
#define ESHL_PROTOCOL_HEAD_HOST_TO_ESC 0xEC     //数据从电调发往主机帧头


#define ESHL_PROTOCOL_BROADCAST_ADDR   0xAAAA   //ESHL协仪广播地址


#define ESHL_PROTOCOL_CMD_THR           0xC1     //油门命令,表示数据包传输油门百分比,float类型，4字节
#define ESHL_PROTOCOL_CMD_ESC_OFF       0xC2     //关闭电调,电调数据0XD0表示电调成功关闭,主机不发送数据,填0,uint8_t,类型,1字节
#define ESHL_PROTOCOL_CMD_ESC_ON        0xC3     //启动电调,数据0XA1表示顺时针启动,0XA2表示逆时针启动,uint8_t类型,1字节
#define ESHL_PROTOCOL_CMD_BREAK         0xC4     //电调刹车,电调数据0XA3表示刹车完毕,主机不发送数据,填0,uint8_t,类型,1字节
#define ESHL_PROTOCOL_CMD_CURRENT_LIMIT 0xC5     //更改电调运行电流限制ADC值,uint16_t类型
#define ESHL_PROTOCOL_CMD_ERROR         0xC6     //电调异常,表示数据包传输电调异常代码，uint8_t类型，1字节
#define ESHL_PROTOCOL_CMD_CHANGE_ADDR   0xC7     //更改电调地址,主机用广播地址发送此命令,更改地址时,所有已连接的电调地址都需要重新设置


#define ESHL_PROTOCOL_ERROR_CODE_MOS            0xE0     //MOS异常
#define ESHL_PROTOCOL_ERROR_CODE_CURRENT        0xE1     //电流异常
#define ESHL_PROTOCOL_ERROR_CODE_START_FAIL     0xE2     //电调开环启动达到最大重启次数,开环启动失败
#define ESHL_PROTOCOL_ERROR_CODE_RUNING_STOP    0xE3     //电机运行过程中意外停转




typedef enum{
    ESHL_DATA_ESC_TO_HOST,//数据从电调发往主机
    ESHL_DATA_HOST_TO_ESC,//数据从主机发往电调
}ESHL_PROTOCOL_ENUM;//ESHL协议枚举


typedef struct  ESHL_PROTOCOL_PACK_ANALYSIS_T
{
    uint8_t head;//数据包帧头
    uint16_t addr;//数据包地址
    uint8_t len;//数据包大小

    uint8_t cmd;//数据包命令码
    uint8_t data;//发送或接收的数据
    uint16_t addr_dat;//发送或接收地址
    uint16_t current_limit;//发送或接收电调运行电流限制
    float throttle;//发送或接收油门百分比
}ESHL_PROTOCOL_PACK_ANALYSIS_T;


void float_to_uint8(float f,uint8_t* u8);
void uint8_to_float(const uint8_t* u8,float* f);
void ESHL_ProtocolPackMake(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* dat_str,ESHL_PROTOCOL_ENUM dat_direction);
uint8_t ESHL_ProtocolAnalysisAddr(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str);
void ESHL_ProtocolAnalysisData(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str);
void ESHL_ProtocolPackAnalysis(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str);

#endif //ESHL_PROTOCOL_H
