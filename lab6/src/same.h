#ifndef SAME_H
#define SAME_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Общая структура для передачи данных
struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

// Общие функции
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif