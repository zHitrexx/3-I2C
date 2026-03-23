#ifndef BitOps
#define BitOps

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t Nastavit(uint8_t data, int bit);
uint8_t Vynulovat(uint8_t data, int bit);
uint8_t Zmenit(uint8_t data, int bit);
uint8_t Negovat(uint8_t data);
bool JeNastaven(uint8_t data, int bit);
bool JePrazdny(uint8_t data, int bit);


#ifdef __cplusplus
}
#endif

#endif