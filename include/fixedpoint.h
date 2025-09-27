/*
    Fixed Point Number implementation
    using 16 bit ints with 8 fractional bits (-128 to 127.99609375) 

*/


#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <Arduino.h>

using fixed_q7_8 = int16_t;
constexpr uint8_t Q7_8_SHIFT = 8;
constexpr fixed_q7_8 Q7_8_ONE = (1 << Q7_8_SHIFT); 


inline fixed_q7_8 q7_8_mult(fixed_q7_8 a, fixed_q7_8 b);

// if b is zero this is unsafe
inline fixed_q7_8 q7_8_div(fixed_q7_8 a, fixed_q7_8 b);

// PROGMEM this ?
constexpr uint8_t sinTable[] = {0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 49, 53, 57, 62, 66, 70, 75, 79, 83, 87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 127, 131, 135, 139, 143, 146, 150, 153, 157, 160, 164, 167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 198, 201, 204, 206, 209, 211, 214, 216, 219, 221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255};

inline fixed_q7_8 sin_q7_8(fixed_q7_8 angleDeg);

inline fixed_q7_8 cos_q7_8(fixed_q7_8 angleDeg);


#endif