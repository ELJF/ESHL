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

#include "ESHL_protocol.h"
#include "crc.h"


//将1个float类型浮点数转为uint8数组
void float_to_uint8(const float f,uint8_t* u8) {
    uint8_t *p = (uint8_t*)&f;
    u8[0] = p[3];
    u8[1] = p[2];
    u8[2] = p[1];
    u8[3] = p[0];
}


//将uint8数组转为float类型浮点数
void uint8_to_float(const uint8_t* u8,float* f) {
    uint8_t *p = (uint8_t*)f;
    p[3] = u8[0];
    p[2] = u8[1];
    p[1] = u8[2];
    p[0] = u8[3];
}



//将数据打包成ESHL协议格式
//  pack 为数据包数组
//  dat  为要打包的数据数组,地址放在第一位,地址后面接命令码,命令码后面接数据,程序会根据命令码打包
//  dat_direction  为数据发送方向
void ESHL_ProtocolPackMake(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* dat_str,ESHL_PROTOCOL_ENUM dat_direction) {

    if ((pack == NULL) || (dat_str == NULL)) {return;}//输入处理

    uint8_t dat_len = 0;//数据包长

    if (dat_direction == ESHL_DATA_HOST_TO_ESC) {//帧头
        pack[0] = ESHL_PROTOCOL_HEAD_HOST_TO_ESC;
    }
    else if (dat_direction == ESHL_DATA_ESC_TO_HOST) {
        pack[0] = ESHL_PROTOCOL_HEAD_ESC_TO_HOST;
    }

    dat_len += 1;

    pack[1] = (dat_str->addr >> 8) & 0xFF;//地址高8位
    pack[2] = dat_str->addr & 0xFF;//地址低8位

    dat_len += 2;

    //pack[3] = dat_len
    dat_len += 1;

    pack[4] = dat_str->cmd;//命令码
    dat_len += 1;

    switch (dat_str->cmd) {

        case ESHL_PROTOCOL_CMD_THR://油门百分比
            float_to_uint8(dat_str->throttle,&pack[dat_len]);
            dat_len += 4;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_ESC_OFF://关闭电调
            pack[dat_len] = dat_str->data;//电调是否成功关闭标志
            dat_len += 1;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_ESC_ON://启动电调
            pack[dat_len] = dat_str->data;//启动方向
            dat_len += 1;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_BREAK://电调刹车
            pack[dat_len] = dat_str->data;//刹车标志
            dat_len += 1;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_CURRENT_LIMIT://更改电调运行电流限制
            pack[dat_len] = (dat_str->current_limit >> 8) & 0xFF;
            pack[dat_len+1] = dat_str->current_limit & 0xFF;
            dat_len += 2;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_ERROR://电调异常
            pack[dat_len] = dat_str->data;//电调错误码
            dat_len += 1;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        case ESHL_PROTOCOL_CMD_CHANGE_ADDR://更改电调地址
            pack[dat_len] =(dat_str->addr_dat >> 8) & 0xFF;
            pack[dat_len+1] = dat_str->addr_dat & 0xFF;
            dat_len += 2;
            dat_len += 2;
            pack[3] = dat_len;
            append_crc16_check_sum(pack,dat_len);//CRC16校验码
            break;

        default:
            break;
    }
}


//提取数据包地址和帧头和包长
//返回CRC校验结果0或1,帧头不匹配返回0
uint8_t ESHL_ProtocolAnalysisAddr(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str) {
    if ((pack == NULL) || (analysis_str == NULL)) {return 0;}

    if (pack[0] != ESHL_PROTOCOL_HEAD_ESC_TO_HOST && pack[0] != ESHL_PROTOCOL_HEAD_HOST_TO_ESC) {
        return 0;
    }

    if(verify_crc16_check_sum(pack,pack[3])) {//CRC16校验
        analysis_str->head = pack[0];
        analysis_str->addr = (pack[1] << 8) | pack[2];
        analysis_str->len = pack[3];
        return 1;
    }
    return 0;
}


//提取完帧头,地址,和包长后,接着提取数据,运行此函数前应先运行ESHL_ProtocolAnalysisAddr
void ESHL_ProtocolAnalysisData(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str) {

    analysis_str->cmd = pack[4];//获取命令码

    switch (analysis_str->cmd) {

        case ESHL_PROTOCOL_CMD_THR://油门命令
            uint8_to_float(&pack[5],&analysis_str->throttle);//获取油门百分比
            analysis_str->data = 0;
            analysis_str->addr_dat = 0;
            analysis_str->current_limit = 0;
            break;

        case ESHL_PROTOCOL_CMD_ESC_OFF://关闭电调
            analysis_str->data = pack[5];
            analysis_str->addr_dat = 0;
            analysis_str->current_limit = 0;
            analysis_str->throttle = 0;
            break;

        case ESHL_PROTOCOL_CMD_ESC_ON://启动电调
            analysis_str->data = pack[5];
            analysis_str->addr_dat = 0;
            analysis_str->current_limit = 0;
            analysis_str->throttle = 0;
            break;

        case ESHL_PROTOCOL_CMD_BREAK://电调刹车
            analysis_str->data = pack[5];
            analysis_str->addr_dat = 0;
            analysis_str->current_limit = 0;
            analysis_str->throttle = 0;
            break;

        case ESHL_PROTOCOL_CMD_CURRENT_LIMIT://更改电调运行电流限制ADC值
            analysis_str->current_limit = (pack[5] << 8) | pack[6];
            analysis_str->data = 0;
            analysis_str->addr_dat = 0;
            analysis_str->throttle = 0;
            break;

        case ESHL_PROTOCOL_CMD_ERROR://电调异常
            analysis_str->data = pack[5];
            analysis_str->addr_dat = 0;
            analysis_str->current_limit = 0;
            analysis_str->throttle = 0;
            break;

        case ESHL_PROTOCOL_CMD_CHANGE_ADDR://更改电调地址
            analysis_str->addr_dat = (pack[5] << 8) | pack[6];
            analysis_str->data = 0;
            analysis_str->current_limit = 0;
            analysis_str->throttle = 0;
            break;

        default:
            break;
    }
}


//解析ESHL协议数据包,全流程一步到位
void ESHL_ProtocolPackAnalysis(uint8_t* pack,ESHL_PROTOCOL_PACK_ANALYSIS_T* analysis_str) {
    if ((pack == NULL) || (analysis_str == NULL)) {return;}

    if(ESHL_ProtocolAnalysisAddr(pack,analysis_str)) {//提取帧头和地址

        ESHL_ProtocolAnalysisData(pack,analysis_str);//提取数据
    }
    else {
        analysis_str->head = 0;
        analysis_str->addr = 0;
        analysis_str->current_limit = 0;
        analysis_str->throttle = 0;
        analysis_str->addr_dat = 0;
        analysis_str->cmd = 0;
        analysis_str->data = 0;
        analysis_str->len = 0;
    }

}


