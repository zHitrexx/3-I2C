#include "BitOps.h"      

// --- FUNKCE ---

uint8_t Nastavit(uint8_t data, int bit) // Nastaví bit do logické 1
{
    data |= (uint8_t)(1 << bit);
    return data;
}

uint8_t Vynulovat(uint8_t data, int bit) /// Nastaví bit do logické 0
{
    data &= (uint8_t)~(1 << bit);
    return data;
}

uint8_t Zmenit(uint8_t data, int bit) // Změní bit na opačnou hodnotu
{
    data ^= (uint8_t)(1 << bit);
    return data;
}

uint8_t Negovat(uint8_t data) // Neguje všechny bity
{
    data = (uint8_t)~data;
    return data;
}

bool JeNastaven(uint8_t data, int bit) // Vrátí true pokud je bit nastaven do logické 1
{
    if ((data & (1 << bit)) != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool JePrazdny(uint8_t data, int bit) // Vrátí true pokud je bit nastaven do logické 0
{
    if ((data & (1 << bit)) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}