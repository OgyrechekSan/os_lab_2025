#include <stdio.h>
#include <stdint.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}

uint64_t FactorialMod(uint64_t k, uint64_t mod) {
    uint64_t result = 1;
    for (uint64_t i = 1; i <= k; i++) {
        result = MultModulo(result, i, mod);
    }
    return result;
}

int main() {
    // Тестовые случаи
    uint64_t test_cases[][2] = {
        {10, 1000},
        {5, 100},
        {20, 100000},
        {15, 10000},
        {25, 1000},
        {8, 500},
        {0, 0} // маркер конца
    };
    
    printf("Ожидаемые результаты \n");
    for (int i = 0; test_cases[i][0] != 0; i++) {
        uint64_t k = test_cases[i][0];
        uint64_t mod = test_cases[i][1];
        uint64_t result = FactorialMod(k, mod);
        printf("%lu! mod %lu = %lu\n", k, mod, result);
    }
    
    return 0;
}