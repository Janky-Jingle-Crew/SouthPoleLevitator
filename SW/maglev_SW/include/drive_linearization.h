#include "stdint.h"

#ifndef DRIVE_LINEARIZATION_H
#define DRIVE_LINEARIZATION_H


// Drivers are quite non-linear.
// Modelling with a third order polynomial gives a decent fit.
// To use this we want a function that takes in our wanted drive level (pid output for example)
// and returns a new drive level.
// The output drive level has a max of +/- max_output_drive (1100 for now).
// The linearization can be scaled in different ways, for now we will scale such that the
// max output drive is mapped to the same input drive level.
// i.e. f(1100) = 1100

// Coefficients are calculated with a python script on measured data.
#define DRIVE_LINEARIZATION_COEFF_0 3.0786919093e-06
#define DRIVE_LINEARIZATION_COEFF_1 -6.1257903678e-03
#define DRIVE_LINEARIZATION_COEFF_2 4.4918853584e+00
#define DRIVE_LINEARIZATION_COEFF_3 -5.5168933353e+02

int32_t linearize_drive(int32_t input_drive);


#endif