//
// Created by Admin on 11/22/2025.
//

#ifndef WIRESTRIPPER_TASK_SAFETY_H
#define WIRESTRIPPER_TASK_SAFETY_H
#include "BQ7692006PWR.h"

extern BQ76920_t BMS;
extern float packCurrent;

void vSafetyTask();

#endif //WIRESTRIPPER_TASK_SAFETY_H