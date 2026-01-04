#include "coil_cal.h"
#include "drive_linearization.h"
#include "coils.h"
#include "TMAG5273.h"
#include "Setup.h"
#include "SEGGER_RTT.h"

// Precompute calibration drive values

int32_t calibration_drive_values[CALIBRATION_NUM_STEPS];
int32_t linearized_calibration_drive_values[CALIBRATION_NUM_STEPS];

int32_t duty_vs_mag_per_coil[4][CALIBRATION_NUM_STEPS][3]; // [coil][step][drive,x,y]
float fit_params[4][2][2]; // [coil][0=x, 1=y][k,b]

void coil_cal_ZeroBias(int16_t offsets[2]) {
    int64_t offset_sum[2] = {0};
    int average_count = 500;

    for (int i = 0; i < average_count; i++) {
        int16_t x, y, z;
        TMAG5273_ReadXYZ(&x, &y, &z);
        offset_sum[0] += x;
        offset_sum[1] += y;
        delay_20us(1);
    }
    offsets[0] = offset_sum[0] / average_count;
    offsets[1] = offset_sum[1] / average_count;
}

void coil_cal_LinearFit(int16_t *x, int16_t *y, int num_points, float *k, float *b)
{
    int32_t n = num_points;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_xy = 0;
    int32_t sum_x2 = 0;

    for (int i = 0; i < num_points; i++)
    {
        sum_x += (int32_t) x[i];
        sum_y += (int32_t) y[i];
        sum_xy += (int32_t) x[i] * (int32_t) y[i];
        sum_x2 += (int32_t) x[i] * (int32_t) x[i];
    }

    *k = (float)(n * sum_xy - sum_x * sum_y) / (float)(n * sum_x2 - sum_x * sum_x);
    *b = ((float)sum_y - (*k) * (float)sum_x) / (float)n;
}

void coil_cal_CoilOffsets(float fit_params[4][2][2], int16_t drive_xp, int16_t drive_xn, int16_t drive_yp, int16_t drive_yn, int32_t *mag_x, int32_t *mag_y)
{
    // Using the linear model from calibration to calculate magnetic field
    *mag_x = 0;
    *mag_y = 0;

    // X Positive Coil
    *mag_x += (int32_t)(fit_params[0][0][0] * drive_xp + fit_params[0][0][1]);
    *mag_y += (int32_t)(fit_params[0][1][0] * drive_xp + fit_params[0][1][1]);

    // X Negative Coil
    *mag_x += (int32_t)(fit_params[1][0][0] * drive_xn + fit_params[1][0][1]);
    *mag_y += (int32_t)(fit_params[1][1][0] * drive_xn + fit_params[1][1][1]);

    // Y Positive Coil
    *mag_x += (int32_t)(fit_params[2][0][0] * drive_yp + fit_params[2][0][1]);
    *mag_y += (int32_t)(fit_params[2][1][0] * drive_yp + fit_params[2][1][1]);

    // Y Negative Coil
    *mag_x += (int32_t)(fit_params[3][0][0] * drive_yn + fit_params[3][0][1]);
    *mag_y += (int32_t)(fit_params[3][1][0] * drive_yn + fit_params[3][1][1]);
}

void coil_cal_Init(void)
{
    for (int i = 0; i < CALIBRATION_NUM_STEPS; i++)
    {
        calibration_drive_values[i] = -CALIBRATION_MAX_DRIVE + (2 * CALIBRATION_MAX_DRIVE * i) / (CALIBRATION_NUM_STEPS - 1);
        linearized_calibration_drive_values[i] = linearize_drive(calibration_drive_values[i]);
    }

}

int8_t coil_cal_MapPWM(float fit_params[4][2][2], int16_t offsets[2])
{
    // Calibration routine to map coil drive duty to magnetic field
    // Uses linearized drive values for calibration

    #ifdef DEBUG_RTT
    SEGGER_RTT_printf(0, "Starting coil calibration routine\r\n");
    #endif

    for (int coil_index = 0; coil_index < 4; coil_index++)
    {
        #ifdef DEBUG_RTT
        SEGGER_RTT_printf(0, "Calibrating Coil %d\r\n", coil_index);
        #endif
        for (int step = 0; step < CALIBRATION_NUM_STEPS; step++)
        {
            int32_t drive = linearized_calibration_drive_values[step];

            // Set coil drive
            coils_SetDuty(drive, coil_index);

            // Wait for settling
            delay_20us(500); // 10 ms

            // Take multiple measurements and average
            int64_t sum_x = 0;
            int64_t sum_y = 0;
            // int32_t sum_z = 0;
            for (int meas = 0; meas < CALIBRATION_NUM_MEASUREMENTS_PER_STEP; meas++)
            {
                int16_t x, y, z;
                TMAG5273_ReadXYZ(&x, &y, &z);
                sum_x += x - offsets[0];
                sum_y += y - offsets[1];
                // sum_z += z;
                // delay_20us(13); // 260 us, about 4 kHz sampling
                delay_20us(1); // 260 us, about 4 kHz sampling
            }
            int32_t avg_x = sum_x / CALIBRATION_NUM_MEASUREMENTS_PER_STEP;
            int32_t avg_y = sum_y / CALIBRATION_NUM_MEASUREMENTS_PER_STEP;
            // int32_t avg_z = sum_z / 50;

            duty_vs_mag_per_coil[coil_index][step][0] = drive;
            duty_vs_mag_per_coil[coil_index][step][1] = avg_x;
            duty_vs_mag_per_coil[coil_index][step][2] = avg_y;
        }

        // Turn off coil after calibration
        coils_SetDuty(0, coil_index);
    }

    #ifdef DEBUG_RTT

    SEGGER_RTT_printf(0, "Calibration data:\r\n");
    for (int coil_index = 0; coil_index < 4; coil_index++)
    {
        SEGGER_RTT_printf(0, "Coil %d:\r\n", coil_index);
        SEGGER_RTT_printf(0, "Drive\tMagX\tMagY\r\n");
        for (int step = 0; step < CALIBRATION_NUM_STEPS; step++)
        {
            SEGGER_RTT_printf(0, "%d\t%d\t%d\r\n", 
                // duty_vs_mag_per_coil[coil_index][step][0], // linearized drive
                calibration_drive_values[step], // original drive 
                duty_vs_mag_per_coil[coil_index][step][1], 
                duty_vs_mag_per_coil[coil_index][step][2]);
        }
    }

    #endif


    // fit_params is passed as parameter

    for (int coil_index = 0; coil_index < 4; coil_index++)
    {
        int16_t drive_values[CALIBRATION_NUM_STEPS];
        int16_t magx_values[CALIBRATION_NUM_STEPS];
        int16_t magy_values[CALIBRATION_NUM_STEPS];

        for (int step = 0; step < CALIBRATION_NUM_STEPS; step++)
        {
            drive_values[step] = (int16_t) duty_vs_mag_per_coil[coil_index][step][0];
            magx_values[step] = (int16_t) duty_vs_mag_per_coil[coil_index][step][1];
            magy_values[step] = (int16_t) duty_vs_mag_per_coil[coil_index][step][2];
        }

        float kx, bx, ky, by;
        coil_cal_LinearFit(drive_values, magx_values, CALIBRATION_NUM_STEPS, &kx, &bx);
        fit_params[coil_index][0][0] = kx;
        fit_params[coil_index][0][1] = bx;
        coil_cal_LinearFit(drive_values, magy_values, CALIBRATION_NUM_STEPS, &ky, &by);
        fit_params[coil_index][1][0] = ky;
        fit_params[coil_index][1][1] = by;

        // Print as integer micro units
        #ifdef DEBUG_RTT
        SEGGER_RTT_printf(0, "Coil %d: MagX = %d * Duty + %d (micro units)\r\n", coil_index, (int32_t)(kx * 1e6), (int32_t)(bx * 1e6));
        SEGGER_RTT_printf(0, "Coil %d: MagY = %d * Duty + %d (micro units)\r\n", coil_index, (int32_t)(ky * 1e6), (int32_t)(by * 1e6));
        #endif
    }

    // We are only interested in one of the two directions per coil for levitation
    // So we can zero out the other direction's fit parameters
    for (int coil_index = 0; coil_index < 4; coil_index++)
    {
        if (coil_index == 0 || coil_index == 2) // X coils
        {
            fit_params[coil_index][1][0] = 0.0f;
            fit_params[coil_index][1][1] = 0.0f;
        }
        else // Y coils
        {
            fit_params[coil_index][0][0] = 0.0f;
            fit_params[coil_index][0][1] = 0.0f;
        }
    }

    // Check for validity
    // If magnetic field is too large during calibration, return error
    // (e.g. external magnet present)

    for (int coil_index = 0; coil_index < 4; coil_index++)
    {
        for (int step = 0; step < CALIBRATION_NUM_STEPS; step++)
        {
            int32_t mag_x = duty_vs_mag_per_coil[coil_index][step][1];
            int32_t mag_y = duty_vs_mag_per_coil[coil_index][step][2];
            if (abs(mag_x) > 2000 || abs(mag_y) > 2000)
            {
                #ifdef DEBUG_RTT
                SEGGER_RTT_printf(0, "Calibration error: While testing coil %d, unexpected high magnetic field measured at drive %d: MagX=%d, MagY=%d\r\n",
                    coil_index, calibration_drive_values[step], mag_x, mag_y);
                #endif
                return -1; // Error
            }
        }
    }

    #ifdef DEBUG_RTT
    SEGGER_RTT_printf(0, "Coil calibration routine completed\r\n");
    #endif
    return 0;
}