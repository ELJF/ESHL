//
// Created by E_LJF on 25-6-7.
//

#include "ESHL_driver.h"
#include "tim.h"
#include "comp.h"
#include "adc.h"
#include "Internal_Flash.h"


#define ESHL_MAX_CURRENT_ADC_MAX	2191		//电调设计最大电流ADC值

uint16_t ESHL_Data[2] = {0};					//电调需要离线保存的数据,[0]为电调地址(默认EC00,可更改),[1]为电调运行电流限制ADC值,默认1158
static  uint8_t ESHL_step = 0;					//六步换向步骤
static  uint16_t ESHL_run_pwm = ESHL_RUN_MIN_PWM;//电调运行pwm
ESHL_DIRECTION_ENUM_T ESHL_direction;			//电调运行方向
ESHL_STATE_ENUM_T ESHL_state;					//电调状态
ESHL_BAT_ENUM_T ESHL_BatType;					//电调使用的电池类型

uint16_t adc_val_buff[5] = {0};  //ADC值缓存
uint16_t ESHL_MOS_Current_ADC_Value = 0;		//测得的MOS电流
uint16_t ESHL_LeakageCurrent_ADC_Value = 0;		//电调总漏电流



//us延时,需要传入一个指向我们所配置的定时器的指针变量(句柄)
void delay_us(TIM_HandleTypeDef *usTIMER,uint16_t us)
{
    __HAL_TIM_SET_COUNTER(usTIMER, 0);  // 把TIMER的counter设为0
    while (__HAL_TIM_GET_COUNTER(usTIMER) < us); //读取定时器的counter值
}
//由于定时器的频率是1MHz，所以其counter每次加1的时候，就表示过了1us
//每次调用时都把定时器的counter先置为0


//关闭所有MOS管
static inline void MOS_CloseAll() {
	ESHL_AD_DISABLE();//关闭A相MOS管
	ESHL_AU_DISABLE();

	ESHL_BU_DISABLE();//关闭B相MOS管
	ESHL_BD_DISABLE();

	ESHL_CU_DISABLE();//关闭C相MOS管
	ESHL_CD_DISABLE();

	delay_us(&ESHL_US_TIM,20);
}


static void ESHL_U_D_Ctrl(uint16_t pwm)
{
    switch (ESHL_step) {
//--------------正转-------------
        case 0://BC MOS
			ESHL_AU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_BD_DISABLE();

			ESHL_BU_ENABLE(pwm);
			ESHL_CD_ENABLE();
            break;

        case 1://BA MOS
			ESHL_AU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_BD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_BU_ENABLE(pwm);
			ESHL_AD_ENABLE();
            break;

        case 2://CA MOS
			ESHL_AU_DISABLE();
			ESHL_BU_DISABLE();
		   ESHL_BD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_CU_ENABLE(pwm);
			ESHL_AD_ENABLE();
            break;

        case 3://CB MOS
			ESHL_AU_DISABLE();
			ESHL_BU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_CU_ENABLE(pwm);
			ESHL_BD_ENABLE();
            break;

        case 4://AB MOS
			ESHL_BU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_AU_ENABLE(pwm);
			ESHL_BD_ENABLE();
            break;

        case 5://AC MOS
			ESHL_BU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_BD_DISABLE();

			ESHL_AU_ENABLE(pwm);
			ESHL_CD_ENABLE();
            break;


//--------------反转-------------
		  case 6://AC MOS
			ESHL_BU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_BD_DISABLE();

			ESHL_AU_ENABLE(pwm);
			ESHL_CD_ENABLE();
			  break;

		  case 7://AB MOS
			ESHL_BU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_AU_ENABLE(pwm);
			ESHL_BD_ENABLE();
            break;

		  case 8://CB MOS
			ESHL_AU_DISABLE();
			ESHL_BU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_CU_ENABLE(pwm);
			ESHL_BD_ENABLE();
            break;

		  case 9://CA MOS
			ESHL_AU_DISABLE();
			ESHL_BU_DISABLE();
		   ESHL_BD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_CU_ENABLE(pwm);
			ESHL_AD_ENABLE();
            break;

		  case 10://BA MOS
			ESHL_AU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_BD_DISABLE();
		   ESHL_CD_DISABLE();

			ESHL_BU_ENABLE(pwm);
			ESHL_AD_ENABLE();
            break;

		  case 11://BC MOS
			ESHL_AU_DISABLE();
			ESHL_CU_DISABLE();
		   ESHL_AD_DISABLE();
		   ESHL_BD_DISABLE();

			ESHL_BU_ENABLE(pwm);
			ESHL_CD_ENABLE();
            break;

        default:
            break;
    }
}


//电调初始化
void ESHL_ESC_Init()
{

	uint16_t read_buff[2] = {0};
	float Vbat = 0;

	ESHL_AD_DISABLE();//关闭A相MOS管
	ESHL_AU_DISABLE();

	ESHL_BU_DISABLE();//关闭B相MOS管
	ESHL_BD_DISABLE();

	ESHL_CU_DISABLE();//关闭C相MOS管
	ESHL_CD_DISABLE();

	ESHL_state = ESHL_STATE_OFF;
	ESHL_direction = ESHL_CLOCKWISE;

    HAL_TIM_PWM_Start(&ESHL_A_TIM,ESHL_A_TIM_CH);//开启MOS上管定时器
    HAL_TIM_PWM_Start(&ESHL_B_TIM,ESHL_B_TIM_CH);
    HAL_TIM_PWM_Start(&ESHL_C_TIM,ESHL_C_TIM_CH);

    HAL_TIM_Base_Start(&ESHL_US_TIM);//开启us延时定时器

	HAL_ADCEx_Calibration_Start(&ESHL_Current_ADC);//ADC校准

	for (uint8_t i = 0; i < 10; i++) {
		HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);//获取电压ADC值
		Vbat += (float)adc_val_buff[1];
	}

	Vbat = (Vbat/10) * 3.3f / 4096 * 10;//ADC值转为电压
	Vbat += 2.18f;	//加是因为ADC不准,有偏差

 	if (Vbat > 22.2f && Vbat < 25.2f) {
		ESHL_BatType = ESHL_BAT_6S;
	}else if (Vbat > 18.5f && Vbat < 21.0f) {
		ESHL_BatType = ESHL_BAT_5S;
	}else if (Vbat > 14.8f && Vbat < 16.8f) {
		ESHL_BatType = ESHL_BAT_4S;
	}else if (Vbat > 11.1f && Vbat < 12.6f) {
		ESHL_BatType = ESHL_BAT_3S;
	} else{
		ESHL_state = ESHL_STATE_BATTERY_VOLTAGE_ERROR;
	}


	InternalFLASH_ReadMore16(ESHL_ADDR_FLASH_ADD,read_buff,2);
	if ((read_buff[0] >> 8) != 0xEC) {//如果没有存储地址
		ESHL_Data[0] = ESHL_DEFALT_ADDR;//默认地址
		ESHL_Data[1] = ESHL_RUN_Current_Limit_ADC_DEFALT;			//默认限制电流ADC值
		InternalFlashWriteMoreUint_16(ESHL_ADDR_FLASH_ADD,ESHL_Data,2);//写入内部Flash
	}
	else {//已经储存地址
		ESHL_Data[0] = read_buff[0];//复制电调地址
		ESHL_Data[1] = read_buff[1];//复制限制电流ADC值
	}

}


//电调开环启动
void ESHL_Start(ESHL_DIRECTION_ENUM_T direction)
{
    static uint16_t time = 100;
	uint8_t BMEF_num = 0;//开环启动时累计过零事件数量
	ESHL_direction = direction;
	ESHL_run_pwm = ESHL_START_PWM;

    HAL_COMP_Stop_IT(&ESHL_COMP);//关闭比较器中断
	HAL_COMP_Start(&ESHL_COMP);  //打开比较器
	MOS_CloseAll();

	ESHL_state = ESHL_STATE_OPEN_LOOP_START;//电调状态更新为开环启动状态
	ESHL_step = (ESHL_direction == ESHL_CLOCKWISE) ? 0 : 6;

	ESHL_U_D_Ctrl(19);//转子定位
    for (uint8_t i = 0; i < 200; i++){
    	HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
    	if (adc_val_buff[0] >= ESHL_RotoCurrent_ADC_MAX) {
    		MOS_CloseAll();
    		HAL_COMP_Stop(&ESHL_COMP);
    		ESHL_run_pwm = 0;
    		ESHL_state = ESHL_STATE_CURRENT_ERROR;//电调状态更新为电流异常
    		return ;
    	}
    	HAL_Delay(1);
    }

    while (1)
    {
        for (uint16_t i = 0; i < time; ++i) {
            delay_us(&ESHL_US_TIM,55);
        }

        if (time < 20)
        {
            time = 100;

            MOS_CloseAll();
        	if ((BMEF_num >= 18) && (BMEF_num <= 35))//开环启动成功
        	{
        		HAL_COMP_Stop(&ESHL_COMP);
        		HAL_COMP_Start_IT(&ESHL_COMP);
        		ESHL_state = (ESHL_direction == ESHL_CLOCKWISE) ? ESHL_STATE_RUN_CLOCKWISE : ESHL_STATE_RUN_COUNTER_CLOCKWISE;//电调状态更新为对应方向运动状态
            }
        	else {			//开环启动失败
        		HAL_COMP_Stop(&ESHL_COMP);
        		HAL_COMP_Stop_IT(&ESHL_COMP);
        		MOS_CloseAll();
        		ESHL_run_pwm = 0;
        		ESHL_state = ESHL_STATE_OPEN_LOOP_START_FAIL;//电调状态更新为开环启动失败
        	}
            break;
        }

    	HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);//检测开环过渡期电流值
    	if (adc_val_buff[0] >= ESHL_OPEN_LOOP_Transition_Period_ADC_MAX) {
    		HAL_COMP_Stop(&ESHL_COMP);
    		MOS_CloseAll();
    		ESHL_run_pwm = 0;
    		ESHL_state = ESHL_STATE_CURRENT_ERROR;//电调状态更新为电流异常
    		return ;
    	}

        time -= time/85+1;
        ESHL_step++;

        if(ESHL_direction == ESHL_COUNTER_CLOCKWISE)
		  {
        	ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
		  }else
		  {
		  	ESHL_step %= 6;
		  }
		  ESHL_U_D_Ctrl(ESHL_run_pwm);

    	if (SENSE_H) {
    		BMEF_num ++;
    	}

    }
}


void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
	uint8_t sense = 0;
	__disable_irq();
	do
	{
		if(SENSE_H) sense = 1; else sense = 0;
	switch(ESHL_step)
	{
//--------------正转-------------
		case 0:
			if(sense)
			{
				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

		    	ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x05 << 4U);//下一步检测C相电动势,切换比较器输入端为PA5
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 1:
			if(!sense)
			{

				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x04 << 4U);//下一步检测B相电动势,切换比较器输入端为PA4
		    	SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 2:
			if(sense)
			{

				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x06 << 4U);//下一步检测A相电动势,切换比较器输入端为PA2
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 3:
			if(!sense)
			{


				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x05 << 4U);//下一步检测C相电动势,切换比较器输入端为PA5
				SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 4:
			if(sense)
			{


				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x04 << 4U);//下一步检测B相电动势,切换比较器输入端为PA4
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 5:
			if(!sense)
			{

				ESHL_step++;
				ESHL_step %= 6;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x06 << 4U);//下一步检测A相电动势,切换比较器输入端为PA2
				SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

//--------------反转-------------

			case 6:
			if(sense)
			{


				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

		    	ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x05 << 4U);//下一步检测C相电动势,切换比较器输入端为PA5
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 7:
			if(!sense)
			{

				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x06 << 4U);//下一步检测A相电动势,切换比较器输入端为PA2
				SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 8:
			if(sense)
			{


				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x04 << 4U);//下一步检测B相电动势,切换比较器输入端为PA4
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 9:
			if(!sense)
			{

				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x05 << 4U);//下一步检测C相电动势,切换比较器输入端为PA5
				SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 10:
			if(sense)
			{


				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x06 << 4U);//下一步检测A相电动势,切换比较器输入端为PA2
				CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);//下降沿触发
				SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		case 11:
			if(!sense)
			{


				ESHL_step++;
				ESHL_step = (ESHL_step > 11) ? 6 : ESHL_step;
				ESHL_U_D_Ctrl(ESHL_run_pwm);

				ESHL_COMP.Instance->CSR = (ESHL_COMP.Instance->CSR & ~(0x7 << 4U)) | (0x04 << 4U);//下一步检测B相电动势,切换比较器输入端为PA4
				SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2);   //上升沿触发
				CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2);
			}
			else
			{
				ESHL_U_D_Ctrl(ESHL_run_pwm);
			}
			break;

		default:
			MOS_CloseAll();
			break;
	}
} while((SENSE_L && sense) || (SENSE_H && !sense));

	__enable_irq();
}




//ESHL_Beep(ESHL_BEEP_SHORT);//E
//
//	ESHL_Beep(ESHL_BEEP_SHORT);//S
//	ESHL_Beep(ESHL_BEEP_SHORT);
//	ESHL_Beep(ESHL_BEEP_SHORT);
//
//	ESHL_Beep(ESHL_BEEP_SHORT);//H
//	ESHL_Beep(ESHL_BEEP_SHORT);
//	ESHL_Beep(ESHL_BEEP_SHORT);
//	ESHL_Beep(ESHL_BEEP_SHORT);
//
//	ESHL_Beep(ESHL_BEEP_SHORT);//L
//	ESHL_Beep(ESHL_BEEP_LONG);
//	ESHL_Beep(ESHL_BEEP_SHORT);
//	ESHL_Beep(ESHL_BEEP_SHORT);




//电机发声函数
void ESHL_Beep(ESHL_BEEP_ENUM_T beep)
{
	if (ESHL_state == ESHL_STATE_OFF) {
		MOS_CloseAll();

		ESHL_step = 0;

		switch (beep) {
			case ESHL_BEEP_SHORT:
				for (uint16_t i = 0; i < 250; i++) {
					ESHL_step = 0;
					ESHL_U_D_Ctrl(100);
					delay_us(&ESHL_US_TIM, 20);
					ESHL_step = 1;
					ESHL_U_D_Ctrl(100);
					delay_us(&ESHL_US_TIM, 180);
				}
				break;

			case ESHL_BEEP_LONG:
				for (uint16_t i = 0; i < 500; i++) {
					ESHL_step = 0;
					ESHL_U_D_Ctrl(100);
					delay_us(&ESHL_US_TIM, 50);
					ESHL_step = 1;
					ESHL_U_D_Ctrl(100);
					delay_us(&ESHL_US_TIM, 180);
				}
				break;

				//		case ESHL_BEEP_DOU:
				//			for(uint16_t i = 0;i < 50;i++)
				//			{
				//				ESHL_step = 0;
				//				ESHL_U_D_Ctrl(100);
				//				delay_us(&ESHL_US_TIM,100);
				//				ESHL_step = 1;
				//				ESHL_U_D_Ctrl(100);
				//				delay_us(&ESHL_US_TIM,500);
				//			}
				//			break;

			default:
				break;
		}

		ESHL_U_D_Ctrl(0);

		MOS_CloseAll();
		HAL_Delay(20);
	}
}


//设置电调运行PWM值
void ESHL_SET_PWM(uint16_t pwm) {
	if (ESHL_state == ESHL_STATE_RUN_CLOCKWISE || ESHL_state == ESHL_STATE_RUN_COUNTER_CLOCKWISE) {
		pwm = (pwm <= ESHL_RUN_MAX_PWM) ? pwm : ESHL_RUN_MAX_PWM;//限制最大值
		pwm = (pwm >= ESHL_RUN_MIN_PWM) ? pwm : ESHL_RUN_MIN_PWM;//限制最小值

		if (pwm > ESHL_run_pwm) {
			ESHL_run_pwm += ESHL_RUN_PWM_STEP;
			ESHL_run_pwm = (ESHL_run_pwm <= 991) ? ESHL_run_pwm : 991;
		}
		else if (pwm < ESHL_run_pwm) {
			if (ESHL_run_pwm <= ESHL_RUN_PWM_STEP) {
				ESHL_run_pwm = 0;
			}
			else {
				ESHL_run_pwm -= ESHL_RUN_PWM_STEP;
			}
		}
		else if (ESHL_run_pwm == pwm){
			ESHL_run_pwm = pwm;
		}
	}
}



//检测MOS管是否短路
//返回问题MOS管编号,0为无异常,10为未知错误
//				 12为漏电流过大
//MOS管编号如下:
//  1(A+)    3(B+)    5(C+)
//  2(A-)    4(B-)    6(C-)
uint8_t MOS_SelfTest() {

    uint8_t error_mos_id = 0;   //异常MOS id
    uint8_t step = 0;           //电流检测步骤
    uint8_t flag = 1;           //电流检测标志

		HAL_COMP_Stop_IT(&ESHL_COMP);//关闭比较器
		MOS_CloseAll();//关闭所有MOS管


		while (flag){
			switch (step) {

				case 0://测总漏电流
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测总漏电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_LeakageCurrent_ADC_Value = (adc_val_buff[0] > ESHL_LeakageCurrent_ADC_Value) ? adc_val_buff[0] : ESHL_LeakageCurrent_ADC_Value;
					}
					if (ESHL_LeakageCurrent_ADC_Value >= 6) {//大于200ma触发
						ESHL_state = ESHL_STATE_CURRENT_ERROR;
						error_mos_id = 12;
						flag = 0;
					}else {
						step = 1;
					}
					break;


				case 1://测A相上管电流
					ESHL_AD_ENABLE();//打开A相下管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测A相上管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 1;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll(); //关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						step = 2;
					}
					break;


				case 2://测A相下管电流
					ESHL_AU_ENABLE(ESHL_MOS_TestPWM);//打开A相上管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测A相下管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 2;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll(); //关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						step = 3;
					}
					break;


				case 3://测B相上管电流
					ESHL_BD_ENABLE();//打开B相下管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测B相上管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 3;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll(); //关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						step = 4;
					}
					break;


				case 4:
					ESHL_BU_ENABLE(ESHL_MOS_TestPWM);//打开B相上管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测B相下管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 4;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll(); //关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						step = 5;
					}
					break;


				case 5:
					ESHL_CD_ENABLE();//打开C相下管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测C相上管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 5;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll(); //关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						step = 6;
					}
					break;



				case 6:
					ESHL_CU_ENABLE(ESHL_MOS_TestPWM);//打开C相上管
					for (uint8_t i = 0; i < ESHL_MOS_Current_Test_num; i++) {   //测C相下管电流
						HAL_ADC_Start_DMA(&ESHL_Current_ADC,adc_val_buff,2);
						ESHL_MOS_Current_ADC_Value = (adc_val_buff[0] > ESHL_MOS_Current_ADC_Value) ? adc_val_buff[0] : ESHL_MOS_Current_ADC_Value;
					}
					if (ESHL_MOS_Current_ADC_Value > ESHL_MOS_Current_ADC_MAX) {
						MOS_CloseAll();//关闭所有MOS管
						error_mos_id = 6;
						ESHL_state = ESHL_STATE_MOS_ERROR;
						flag = 0;
					}
					else{
						MOS_CloseAll();//关闭所有MOS管
						ESHL_MOS_Current_ADC_Value = 0;
						// step = 6;
						flag = 0;
					}
					break;


				default:
					MOS_CloseAll();
					error_mos_id = 10;
					break;
			}
		}
	return error_mos_id;
}



//电机运行过程中检测电机是否停转
void ESHL_RuningChack() {
	static uint32_t last_tick = 0;
	static uint8_t last_step = 0;
	if ((ESHL_state == ESHL_STATE_RUN_CLOCKWISE) || (ESHL_state == ESHL_STATE_RUN_COUNTER_CLOCKWISE) /*|| (ESHL_state == ESHL_STATE_MOTOR_RUNING_STOP)*/) {
		if (ESHL_step != last_step) {
			last_step = ESHL_step;
			last_tick = HAL_GetTick();
		}
		else if (HAL_GetTick() - last_tick >= ESHL_MOTOR_TIMEOUT) {
			HAL_COMP_Stop_IT(&ESHL_COMP);
			MOS_CloseAll();
			ESHL_run_pwm = 0;
			last_tick = 0;
			last_step = 0;
			ESHL_state = ESHL_STATE_MOTOR_RUNING_STOP;//电调状态更新为电机运行时停转
		}
	}
}


//电机运行时定时检测电流和电池电压
void ESHL_RuningCurrentVBATChack() {

	static uint32_t last_tick = 0;

#if ESHL_VBAT_CHACK_EN
		float Vbat = 0;
#endif


		if (HAL_GetTick() - last_tick >= ESHL_RUNING_CURRENT_VBAT_CHACK_TIMOUT) {
			HAL_ADC_Start_DMA(&ESHL_Current_ADC, adc_val_buff, 2);//检测电调电流
			last_tick = HAL_GetTick();
		}

		if (adc_val_buff[0] >= ESHL_Data[1]) {
			HAL_COMP_Stop_IT(&ESHL_COMP);
			MOS_CloseAll();
			ESHL_run_pwm = 0;
			last_tick = 0;
			ESHL_state = ESHL_STATE_CURRENT_ERROR;//电调状态更新为电流异常
		}

#if ESHL_VBAT_CHACK_EN
		Vbat = (float)adc_val_buff[1] * 3.3f / 4096 * 10 + 2.18f;//ADC值转为电压,加是因为ADC不准,有偏差
		Vbat += 1.1f;	//加1.1是因为ADC不准,有偏差

		switch (ESHL_BatType) {

			case ESHL_BAT_3S:
				if ((Vbat - 11.1f) <= ESHL_VBAT_LIMIT) {
					HAL_COMP_Stop_IT(&ESHL_COMP);
					MOS_CloseAll();
					ESHL_run_pwm = 0;
					last_tick = 0;
					ESHL_state = ESHL_STATE_BATTERY_VOLTAGE_ERROR;//电调状态更新为电池电压异常
				}
				break;

			case ESHL_BAT_4S:
				if ((Vbat - 14.8f) <= ESHL_VBAT_LIMIT) {
					HAL_COMP_Stop_IT(&ESHL_COMP);
					MOS_CloseAll();
					ESHL_run_pwm = 0;
					last_tick = 0;
					ESHL_state = ESHL_STATE_BATTERY_VOLTAGE_ERROR;//电调状态更新为电池电压异常
				}
				break;

			case ESHL_BAT_5S:
				if ((Vbat - 18.5f) <= ESHL_VBAT_LIMIT) {
					HAL_COMP_Stop_IT(&ESHL_COMP);
					MOS_CloseAll();
					ESHL_run_pwm = 0;
					last_tick = 0;
					ESHL_state = ESHL_STATE_BATTERY_VOLTAGE_ERROR;//电调状态更新为电池电压异常
				}
				break;

			case ESHL_BAT_6S:
				if ((Vbat - 22.2f) <= ESHL_VBAT_LIMIT) {
					HAL_COMP_Stop_IT(&ESHL_COMP);
					MOS_CloseAll();
					ESHL_run_pwm = 0;
					last_tick = 0;
					ESHL_state = ESHL_STATE_BATTERY_VOLTAGE_ERROR;//电调状态更新为电池电压异常
				}
				break;

			default:
				break;
		}
#endif
}


//更改电调运行限制电流
void ESHL_ChangeRuningCurrentLimit(uint16_t current) {
	ESHL_Data[1] = (current >= ESHL_MAX_CURRENT_ADC_MAX) ? ESHL_MAX_CURRENT_ADC_MAX : current;
	InternalFlashWriteMoreUint_16(ESHL_ADDR_FLASH_ADD,ESHL_Data,2);//限制电流ADC值写入内部Flash
}


//外部电调状态设置接口
void ESHL_SetState(ESHL_STATE_ENUM_T state) {
	ESHL_state = state;
}


//获取电调当前状态
ESHL_STATE_ENUM_T ESHL_GetState() {
	return ESHL_state;
}


//电调刹车
void ESHL_Break() {

	static  uint32_t BMEF_NUM = 0;//过零事件计数
	static uint32_t BMEF_NUM_Last = 0;//上一次过零事件计数
	static uint16_t Same_BMEF_NUM = 0;//过零事件计数相同的次数

	if (ESHL_state == ESHL_STATE_BRAKE) {
		ESHL_run_pwm = 0;
		MOS_CloseAll();

		HAL_COMP_Start(&ESHL_COMP);

		if (SENSE_H) {
			BMEF_NUM ++;
		}

		if ((BMEF_NUM - BMEF_NUM_Last) != 0) {//电机还没停转
			BMEF_NUM_Last = BMEF_NUM;
		}else if (BMEF_NUM - BMEF_NUM_Last == 0) {
			Same_BMEF_NUM ++;
		}

		if (Same_BMEF_NUM >= ESHL_BREAK_OK_SAME_BMEF_NUM) {//判定为电机停转
			MOS_CloseAll();
			HAL_COMP_Stop(&ESHL_COMP);
			ESHL_state = ESHL_STATE_BRAKE_OK;//更新状态为刹车成功
			BMEF_NUM = 0;
			BMEF_NUM_Last = 0;
			Same_BMEF_NUM = 0;
			return;//退出函数
		}

#if ESHL_BREAK_MOD == 1//启用三相短路刹车
		ESHL_AU_ENABLE(ESHL_BREAK_PWM);
		ESHL_BU_ENABLE(ESHL_BREAK_PWM);
		ESHL_CU_ENABLE(ESHL_BREAK_PWM);

		for (uint8_t i = 0;i < 25; i++) {
			HAL_ADC_Start_DMA(&ESHL_Current_ADC, adc_val_buff, 2); //检测电调电流
			if (adc_val_buff[0] > ESHL_Data[1]) {//过流保护
				MOS_CloseAll();
				break;
			}
		}
#endif
	}
}


//获取电调地址
uint16_t ESHL_GetAddr() {
	return ESHL_Data[0];
}


//设置电调地址
//addr为接收到的地址,不是要设置的地址
/*主机用广播地址发送更改地址命令,更改地址时,所有已连接的电调地址都需要重新设置
地址设置方法: 快速旋转已经连接上电调的电机,此时电调将自动将自身地址设为EC00
同时电调状态指示灯会闪烁,闪烁次数代表电调地址
此时旋转第二个电调的电机,此时电调将自动将自身地址设为EC01,以此类推,直到所有电调地址设置完毕
电调设置地址后需要重新上电才能使用*/
void ESHL_SetAddr(uint16_t addr) {

	static uint16_t max_addr = 0;//当前最大地址
	static uint8_t BMEF_Num = 0;//过零事件计数

	HAL_COMP_Start(&ESHL_COMP);

	if (SENSE_H) {
		BMEF_Num ++;
	}

	if (((addr >> 8) != 0xEC) && (addr != 0x0000)) {//输入处理
		BMEF_Num = 0;
		return;
	}
	if ((addr & 0xff) > (max_addr & 0xff)) {
		max_addr = addr;//更新地址最大值
	}

	if (BMEF_Num >= 12) {
		if (((max_addr & 0xff) + 1) >= 0xFF) {//达到最大地址
			ESHL_Data[0] = 0xECFF;//最大地址
		}
		else if (addr == 0x0000) {
			ESHL_Data[0] = 0xEC01;//第一个设置地址的电调
		}
		else {
			ESHL_Data[0] = (max_addr + 1);
		}
		InternalFlashWriteMoreUint_16(ESHL_ADDR_FLASH_ADD,ESHL_Data,2);//电调地址写入内部Flash
		ESHL_state = ESHL_STATE_SET_ADDR_OK;//更新电调状态
		HAL_COMP_Stop(&ESHL_COMP);
		max_addr = 0;
		BMEF_Num = 0;
	}
}


//更改电调运行方向
void ESHL_SetDirection(ESHL_DIRECTION_ENUM_T direction) {
	if ((ESHL_state != ESHL_STATE_RUN_CLOCKWISE) && (ESHL_state != ESHL_STATE_RUN_COUNTER_CLOCKWISE)) {//电调非运行状态才能更改方向
		ESHL_direction = direction;
	}
}


//获取电调当前运行方向外部接口
ESHL_DIRECTION_ENUM_T ESHL_GetDirection() {
	return ESHL_direction;
}


//关闭电调,可被通信唤醒
// void ESHL_OFF() {
// 	ESHL_run_pwm = 0;
// 	HAL_COMP_Stop_IT(&ESHL_COMP);
// 	HAL_COMP_Stop(&ESHL_COMP);
// 	MOS_CloseAll();
// 	ESHL_state = ESHL_STATE_OFF;
// }


//关闭所有MOS管并且关闭比较器
void ESHL_CloseMOSComp() {
	ESHL_run_pwm = 0;
	HAL_COMP_Stop_IT(&ESHL_COMP);
	HAL_COMP_Stop(&ESHL_COMP);
	MOS_CloseAll();
}


