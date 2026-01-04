#pragma once
#include "stdint.h"

#define COIL_FREQ 50000
#define CORE_CLOCK 64000000
#define TIMER_ARR (CORE_CLOCK/COIL_FREQ) - 1
#define MAX_TIMER_VAL TIMER_ARR

// Timer ARR is max 16 bits. Lowest frequency is 261 Hz -> ARR = CORE_CLOCK/261*(PSC+1) < 2^16
// PSC = 3 or 4 works
#define TONE_PSC 4

#define RMS_WINDOW        200    // 200 ms at 1 kHz

#define RMS_LIMIT_HIGH   (1700U * 1700U)   // ≈ upper limit
#define RMS_LIMIT_LOW    (1600U * 1600U)   // ≈ lower limit

#define PWM_MIN_DYNAMIC   1200        // never fully disable control

#define MAX_PWM_TOTAL      1400       // Default value if not using "moving RMS"

#define SQRT_TABLE_BITS    8
#define SQRT_TABLE_SIZE    (1 << SQRT_TABLE_BITS)

/* mag_sq max ≈ 3.28e6 → shift to 8-bit index */
#define SQRT_SHIFT         15   // (3,271,682 >> 15 ≈ 99)

#define Q15_ONE            32768


typedef struct
{
    uint8_t note;
    uint16_t duration;
} note_tt;


typedef enum {
    COIL_XP = 0,
    COIL_YP,
    COIL_XN,
    COIL_YN
} coil_pos_t;

//void coils_Tone(uint32_t *freq, uint32_t *ms, uint32_t len);
void coils_Tone(uint32_t jingle_idx);
void coils_SetDuty(int16_t duty, coil_pos_t coil);
void coils_ZeroDuty(void);
void coils_TIM1_Init(void);
void coils_TIM3_Init(void);
void coils_LimitDuty(int32_t *x, int32_t *y);
void coils_CalculateRMS(int32_t x, int32_t y);

static const uint16_t sqrt_table[SQRT_TABLE_SIZE] =
{
    0, 181, 256, 313, 362, 404, 443, 478, 

    512, 543, 572, 600, 627, 652, 677, 701, 

    724, 746, 768, 789, 809, 829, 849, 868, 

    886, 905, 923, 940, 957, 974, 991, 1007, 

    1024, 1039, 1055, 1070, 1086, 1101, 1115, 1130, 

    1144, 1159, 1173, 1187, 1200, 1214, 1227, 1241, 

    1254, 1267, 1280, 1292, 1305, 1317, 1330, 1342, 

    1354, 1366, 1378, 1390, 1402, 1413, 1425, 1436, 

    1448, 1459, 1470, 1481, 1492, 1503, 1514, 1525, 

    1536, 1546, 1557, 1567, 1578, 1588, 1598, 1608, 

    1619, 1629, 1639, 1649, 1659, 1668, 1678, 1688, 

    1698, 1707, 1717, 1726, 1736, 1745, 1755, 1764, 

    1773, 1782, 1792, 1801, 1810, 1819, 1828, 1837, 

    1846, 1854, 1863, 1872, 1881, 1889, 1898, 1907, 

    1915, 1924, 1932, 1941, 1949, 1958, 1966, 1974, 

    1982, 1991, 1999, 2007, 2015, 2023, 2031, 2039, 

    2048, 2055, 2063, 2071, 2079, 2087, 2095, 2103, 

    2111, 2118, 2126, 2134, 2141, 2149, 2157, 2164, 

    2172, 2179, 2187, 2194, 2202, 2209, 2217, 2224, 

    2231, 2239, 2246, 2253, 2260, 2268, 2275, 2282, 

    2289, 2296, 2304, 2311, 2318, 2325, 2332, 2339, 

    2346, 2353, 2360, 2367, 2374, 2380, 2387, 2394, 

    2401, 2408, 2415, 2421, 2428, 2435, 2442, 2448, 

    2455, 2462, 2468, 2475, 2482, 2488, 2495, 2501, 

    2508, 2514, 2521, 2527, 2534, 2540, 2547, 2553, 

    2560, 2566, 2572, 2579, 2585, 2591, 2598, 2604, 

    2610, 2616, 2623, 2629, 2635, 2641, 2648, 2654, 

    2660, 2666, 2672, 2678, 2684, 2691, 2697, 2703, 

    2709, 2715, 2721, 2727, 2733, 2739, 2745, 2751, 

    2757, 2763, 2769, 2774, 2780, 2786, 2792, 2798, 

    2804, 2810, 2816, 2821, 2827, 2833, 2839, 2844, 

    2850, 2856, 2862, 2867, 2873, 2879, 2884, 2890, 
};