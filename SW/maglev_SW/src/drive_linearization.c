#include "drive_linearization.h"

#ifndef CLAMP
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, lower, upper) (MIN((upper), MAX((x), (lower))))
#endif

// Fixed-point coefficients (scaled by 2^32 for 32-bit fractional precision)
// Calculated from:
// C0 = 3.0786919093e-06 * 2^32 = 13222
// C1 = -6.1257903678e-03 * 2^32 = -26310069
// C2 = 4.4918853584e+00 * 2^32 = 19292500711
// C3 = -5.5168933353e+02 * 2^32 = -2369487645063
// #define COEFF_0_FP  13222LL
// #define COEFF_1_FP  -26310069LL
// #define COEFF_2_FP  19292500711LL
// #define COEFF_3_FP  -2369487645063LL

// int32_t linearize_drive(int32_t input_drive)
// {
//     // DEBUG
//     // return input_drive;

//     if (input_drive == 0) return 0;

//     int8_t drive_sign;
//     int32_t abs_drive;
//     // If negative, work with positive and reapply sign at the end
//     if (input_drive < 0)
//     {
//         drive_sign = -1;
//         abs_drive = -input_drive;
//     }
//     else
//     {
//         drive_sign = 1;
//         abs_drive = input_drive;
//     }

//     int64_t x = abs_drive;
//     int64_t x2 = x * x;
//     int64_t x3 = x2 * x;

//     // Fixed-point polynomial: all arithmetic in 64-bit integers
//     // We sum the terms first, then shift down by 32 bits.
//     int64_t output_fp = COEFF_0_FP * x3 +
//                         COEFF_1_FP * x2 +
//                         COEFF_2_FP * x +
//                         COEFF_3_FP;

//     int32_t output = (int32_t)(output_fp >> 32);

//     // Clamp to int32 range
//     output = CLAMP(output, -2147483648, 2147483647);

//     return output * drive_sign;
// }


// Linearization using inverse of y = C2*x^2 + C0

// Inverse function coefficients for microcontroller (y = sqrt((x - C0) / C2)):
// C2: 1.3712750945e-03
// C0: 3.9305570985e+01

// Fixed-point (scaled by 2^32):
// COEFF_2_FP: 5889581LL
// COEFF_0_FP: 168816141929LL



int32_t linearize_drive(int32_t input_drive)
{
    // DEBUG
    // return input_drive;

    if (input_drive == 0) return 0;

    int8_t drive_sign;
    int32_t abs_drive;
    // If negative, work with positive and reapply sign at the end
    if (input_drive < 0)
    {
        drive_sign = -1;
        abs_drive = -input_drive;
    }
    else
    {
        drive_sign = 1;
        abs_drive = input_drive;
    }

    // Fixed-point inverse polynomial: all arithmetic in 64-bit integers
    int64_t x = abs_drive;

    // Subtract C0 in fixed-point: (x - C0) * 2^32
    // int64_t x_minus_c0_fp = (x << 32) - 168816141929LL;
    // Do not remove offset
    int64_t x_minus_c0_fp = (x << 32);
    if (x_minus_c0_fp <= 0) return 0;

    // Divide by C2 in fixed-point: ((x - C0) * 2^32) / (C2 * 2^32) = (x - C0) / C2
    int64_t division = x_minus_c0_fp / 5889581LL;

    if (division <= 0) return 0;

    // Approximate square root using Newton's method
    int64_t approx = 300; // Reasonable initial guess for expected range
    for (int i = 0; i < 8; i++) // 10 iterations for good precision
    {
        int64_t next_approx = (approx + division / approx) >> 1;
        if (next_approx == approx) break;
        approx = next_approx;
    }

    // Clamp to int32 range
    approx = CLAMP(approx, -2147483648LL, 2147483647LL);

    return (int32_t)approx * drive_sign;
}