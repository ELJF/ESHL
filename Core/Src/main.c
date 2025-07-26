/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "comp.h"
#include "dma.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <string.h>
#include "ESHL_driver.h"
#include "ws2812b.h"
#include "communication_management.h"
#include "ESHL_protocol.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

extern uint16_t ESHL_RunPWMBuff;   //电调运行pwm缓存
extern ESHL_PROTOCOL_PACK_ANALYSIS_T recv_str;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_COMP2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_TIM16_Init();
  MX_ADC_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  uint8_t ESHL_OpenLoopRestartNum = 0;//电调重开环启动计数

  WS2812B_OBJ_T ESHL_StateLed;                                    //创建电调ws2812b指示灯对象
  WS2812_Init(&ESHL_StateLed,&htim16,TIM_CHANNEL_1,1);//初始化
  WS2812_Set(&ESHL_StateLed,0,15,15,0);            //默认为黄色

  //电调指示灯                   内容
  //--------------------------------------------------
  //绿色常亮               表示电调准备就绪,正常运行
  //--------------------------------------------------
  //黄色常亮               表示正在初始化
  //--------------------------------------------------
  //红色闪烁               表示MOS异常,闪烁次数代表错误代码
  //--------------------------------------------------
  //橙色闪烁               表示电流异常
  //--------------------------------------------------
  //蓝色闪烁               表示主机离线,数据接收超时
  //--------------------------------------------------
  //白色闪烁               表示电调开环启动超过最大次数,开环启动失败
  //--------------------------------------------------
  //紫色常亮               表示电调正在设置地址
  //--------------------------------------------------
  //紫色闪烁               表示地址设置完毕,闪烁次数代表电调地址
  //--------------------------------------------------
  //浅蓝闪烁               表示电池电压异常
  //--------------------------------------------------
  //绿色与蓝色交替闪烁       表示电调串口接收到数据,但不是发给本电调的
  //--------------------------------------------------

  ESHL_ESC_Init();

  uint8_t mos_error_id = MOS_SelfTest();//MOS自检,并获取错误代码
  if (mos_error_id != 0) {
    while (1) {
      for (uint8_t i = 0;i < mos_error_id;i++) {
        WS2812_SetAll(&ESHL_StateLed,15,0,0);//红灯闪烁,闪烁次数代表错误码
        HAL_Delay(200);
        WS2812_SetAll(&ESHL_StateLed,0,0,0);
        HAL_Delay(200);
        ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_ERROR,0xE0);
      }
      HAL_Delay(800);
    }
  }

  //----------开机音乐----------
  ESHL_Beep(ESHL_BEEP_SHORT);//E
  HAL_Delay(100);

  ESHL_Beep(ESHL_BEEP_SHORT);//S
  ESHL_Beep(ESHL_BEEP_SHORT);
  ESHL_Beep(ESHL_BEEP_SHORT);
  HAL_Delay(100);

  ESHL_Beep(ESHL_BEEP_LONG);//C
  ESHL_Beep(ESHL_BEEP_SHORT);
  ESHL_Beep(ESHL_BEEP_LONG);
  ESHL_Beep(ESHL_BEEP_SHORT);
  HAL_Delay(100);

  // ESHL_Beep(ESHL_BEEP_SHORT);//E
  // HAL_Delay(100);
  //
  // ESHL_Beep(ESHL_BEEP_SHORT);//S
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // HAL_Delay(100);
  //
  // ESHL_Beep(ESHL_BEEP_SHORT);//H
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // HAL_Delay(100);
  //
  // ESHL_Beep(ESHL_BEEP_SHORT);//L
  // ESHL_Beep(ESHL_BEEP_LONG);
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // ESHL_Beep(ESHL_BEEP_SHORT);
  // HAL_Delay(100);

  //----------end of 开机音乐----------

  if (ESHL_GetState() == ESHL_STATE_OFF) {//初始化无错误
    ESHL_SetState(EShl_STATE_READY);//将电调状态设置为准备就绪
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    ESHL_RuningCurrentVBATChack();//电流和电池电压检测
    switch (ESHL_GetState()) {

      case ESHL_STATE_OFF://电调关闭
        ESHL_CloseMOSComp();//关闭所有MOS管并且关闭比较器
        WS2812_SetAll(&ESHL_StateLed,0,0,0);
        break;

      case EShl_STATE_READY://电调准备就绪
        WS2812_SetAll(&ESHL_StateLed,0,15,0);//状态指示灯设为绿色
        ESHL_CommunicationStart();//开始通信
        break;

      case ESHL_STATE_START://启动电机
        WS2812_SetAll(&ESHL_StateLed,0,15,0);//状态指示灯设为绿色
        ESHL_Start(ESHL_GetDirection());
        break;

      case ESHL_STATE_OPEN_LOOP_START_FAIL://电调开环启动失败
        if (ESHL_OpenLoopRestartNum > ESHL_OPEN_LOOP_RESTART_MAX_NUM - 2) {
          ESHL_SetState(ESHL_STATE_OPEN_LOOP_START_ERROR);//设置电调状态为无法开环启动
        }
        else{
          ESHL_OpenLoopRestartNum++;
          ESHL_Start(ESHL_GetDirection());
        }
        break;

      case ESHL_STATE_OPEN_LOOP_START_ERROR://电调开环启动重试超过最大次数,无法开环启动
          ESHL_CloseMOSComp();//关闭所有MOS管并且关闭比较器
          WS2812_SetAll(&ESHL_StateLed,15,15,15);
          HAL_Delay(200);
          WS2812_SetAll(&ESHL_StateLed,0,0,0);
          HAL_Delay(200);
          ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_ERROR,0XE2);//发送电调错误码
        break;

      case ESHL_STATE_RUN_CLOCKWISE://顺时针方向运动中
        ESHL_OpenLoopRestartNum = 0;//开环重启动计数清零
        ESHL_RuningChack();       //堵转检测
        ESHL_SET_PWM(ESHL_RunPWMBuff);//更新运行PWM值
        WS2812_SetAll(&ESHL_StateLed,0,15,0);//状态指示灯设为绿色
        break;

       case ESHL_STATE_RUN_COUNTER_CLOCKWISE://逆时针方向运动中
        ESHL_OpenLoopRestartNum = 0;//开环重启动计数清零
        ESHL_RuningChack();       //堵转检测
        ESHL_SET_PWM(ESHL_RunPWMBuff);//更新运行PWM值
        WS2812_SetAll(&ESHL_StateLed,0,15,0);//状态指示灯设为绿色
        break;

      case ESHL_STATE_MOTOR_RUNING_STOP://电机运行时停转
        ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_ERROR,0XE3);//发送电调错误码
        ESHL_CommunicationStart();//开启接收
        ESHL_Start(ESHL_GetDirection());//开环启动
        break;

      case ESHL_STATE_CURRENT_ERROR://电流异常
          ESHL_CloseMOSComp();//关闭所有MOS管并且关闭比较器
          WS2812_SetAll(&ESHL_StateLed,15,5,0);
          HAL_Delay(200);
          WS2812_SetAll(&ESHL_StateLed,0,0,0);
          HAL_Delay(200);
          ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_ERROR,0XE1);//发送电调错误码
        break;

      case ESHL_STATE_SET_HOST_TIMEOUT://主机离线
          ESHL_CloseMOSComp();//关闭所有MOS管并且关闭比较器
          WS2812_SetAll(&ESHL_StateLed,0,0,15);
          HAL_Delay(200);
          WS2812_SetAll(&ESHL_StateLed,0,0,0);
          HAL_Delay(200);
        break;

      case ESHL_STATE_SET_ADDR://电调设置地址中
        ESHL_SetAddr(recv_str.addr_dat);
        WS2812_SetAll(&ESHL_StateLed,15,0,15);
        break;

      case ESHL_STATE_SET_ADDR_OK://电调地址设置成功
        while (1) {
          for (uint8_t i = 0;i < (uint8_t)(ESHL_GetAddr() & 0xff);i++) {
            WS2812_SetAll(&ESHL_StateLed,15,0,15);
            HAL_Delay(200);
            WS2812_SetAll(&ESHL_StateLed,0,0,0);
            HAL_Delay(200);
            ESHL_CommunicationAddressSend();//广播电调地址
          }
          HAL_Delay(800);
        }
        break;

      case ESHL_STATE_BATTERY_VOLTAGE_ERROR://电池电压异常
        ESHL_CloseMOSComp();//关闭所有MOS管并且关闭比较器
        WS2812_SetAll(&ESHL_StateLed,0,15,15);
        HAL_Delay(200);
        WS2812_SetAll(&ESHL_StateLed,0,0,0);
        HAL_Delay(200);
        ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_ERROR,0XE4);//发送电调错误码
        break;

      case ESHL_STATE_BRAKE:///刹车中
        ESHL_Break();
        break;

      case ESHL_STATE_BRAKE_OK://刹车完毕
        ESHL_CommunicationSendCode(ESHL_PROTOCOL_CMD_BREAK,0xA3);
        ESHL_SetState(EShl_STATE_READY);
        break;

      default:
        break;
    }
    ESHL_CommunicationDataProcessing();//通信数据处理
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14
                              |RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
