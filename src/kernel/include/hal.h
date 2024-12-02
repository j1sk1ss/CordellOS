#ifndef HAL_H_
#define HAL_H_

#include "isr.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "stdio.h"
#include "tss.h"


void  HAL_initialize();

#endif