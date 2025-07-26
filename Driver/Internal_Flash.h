//
// Created by E_LJF on 25-7-17.
//

#ifndef INTERNAL_FLASH_H
#define INTERNAL_FLASH_H

#include "main.h"

void InternalFlashWriteUint_64(uint32_t Address,uint64_t dat);
void InternalFlashWriteUint_32(uint32_t Address,uint32_t dat);
void InternalFlashWriteUint_16(uint32_t Address,uint16_t dat);

void InternalFlashWriteMoreUint_16(uint32_t Address,uint16_t* dat,uint16_t dat_len);
void InternalFlashWriteMoreUint_32(uint32_t Address,uint32_t* dat,uint16_t dat_len);
void InternalFlashWriteMoreUint_64(uint32_t Address,uint64_t* dat,uint16_t dat_len);

uint16_t InternalFLASH_Read16(uint32_t address);
uint32_t InternalFLASH_Read32(uint32_t address);
uint64_t InternalFLASH_Read64(uint32_t address);

void InternalFLASH_ReadMore16(uint32_t address,uint16_t* read_dat,uint16_t dat_len);
void InternalFLASH_ReadMore32(uint32_t address,uint32_t* read_dat,uint16_t dat_len);
void InternalFLASH_ReadMore64(uint32_t address,uint64_t* read_dat,uint16_t dat_len);

#endif //INTERNAL_FLASH_H
