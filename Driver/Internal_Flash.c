//
// Created by E_LJF on 25-7-17.
//

#include "Internal_Flash.h"
#include "stm32f0xx_hal_flash.h"


//按uint64大小写stm32内部Flash
void InternalFlashWriteUint_64(uint32_t Address,uint64_t dat) {

    if (Address == 0) {
        return;
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };

    uint32_t page_error = 0;

    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,Address,dat);

    HAL_FLASH_Lock();
}


//按uint32大小写stm32内部Flash
void InternalFlashWriteUint_32(uint32_t Address,uint32_t dat) {

    if (Address == 0) {
        return;
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };

    uint32_t page_error = 0;

    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Address,dat);

    HAL_FLASH_Lock();
}


//按uint16大小写stm32内部Flash
void InternalFlashWriteUint_16(uint32_t Address,uint16_t dat) {

    if (Address == 0) {
        return;
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };

    uint32_t page_error = 0;

    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,Address,dat);

    HAL_FLASH_Lock();
}


//按uint16大小往stm32内部Flash写多个值
void InternalFlashWriteMoreUint_16(uint32_t Address,uint16_t* dat,uint16_t dat_len) {
    if (Address == 0) {return;}
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    for (uint16_t index = 0; index < dat_len; index++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,(Address + (2U*index)),dat[index]);
    }
    HAL_FLASH_Lock();
}


//按uint32大小往stm32内部Flash写多个值
void InternalFlashWriteMoreUint_32(uint32_t Address,uint32_t* dat,uint16_t dat_len) {
    if (Address == 0) {return;}
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    for (uint16_t index = 0; index < dat_len; index++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,(Address + (4U*index)),dat[index]);
    }
    HAL_FLASH_Lock();
}


//按uint64大小往stm32内部Flash写多个值
void InternalFlashWriteMoreUint_64(uint32_t Address,uint64_t* dat,uint16_t dat_len) {
    if (Address == 0) {return;}
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = Address,
        .NbPages = 1
    };
    uint32_t page_error = 0;
    HAL_FLASHEx_Erase(&EraseInit,&page_error);
    for (uint16_t index = 0; index < dat_len; index++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(Address + (8U*index)),dat[index]);
    }
    HAL_FLASH_Lock();
}


//读取指定地址的半字(16位数据)
//也是按照半字读出，即每次读2个字节数据返回
uint16_t InternalFLASH_Read16(uint32_t address)
{
    if (address == 0) {return 0;}
    return *(__IO uint16_t*)address;
}


//读取指定地址的字(32位数据)
//也是按照字读出，即每次读4个字节数据返回
uint32_t InternalFLASH_Read32(uint32_t address) {
    if (address == 0) {return 0;}
    return *(__IO uint32_t*)address;
}


//读取指定地址的两字(64位数据)
//也是按照两字读出，即每次读8个字节数据返回
uint64_t InternalFLASH_Read64(uint32_t address) {
    if (address == 0) {return 0;}
    return *(__IO uint64_t*)address;
}


//读取指定地址多个uint16数据
void InternalFLASH_ReadMore16(uint32_t address,uint16_t* read_dat,uint16_t dat_len) {
    if (address == 0) {return ;}
    for (uint16_t index = 0; index < dat_len; index++) {
        read_dat[index] = *(__IO uint16_t*)(address+index*2);
    }
}


//读取指定地址多个uint32数据
void InternalFLASH_ReadMore32(uint32_t address,uint32_t* read_dat,uint16_t dat_len) {
    if (address == 0) {return ;}
    for (uint16_t index = 0; index < dat_len; index++) {
        read_dat[index] = *(__IO uint32_t*)(address+index*4);
    }
}


//读取指定地址多个uint64数据
void InternalFLASH_ReadMore64(uint32_t address,uint64_t* read_dat,uint16_t dat_len) {
    if (address == 0) {return ;}
    for (uint16_t index = 0; index < dat_len; index++) {
        read_dat[index] = *(__IO uint64_t*)(address+index*8);
    }
}

