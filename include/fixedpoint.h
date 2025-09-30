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
constexpr fixed_q7_8 Q7_8_MAX = 32767;  //  127.99609375
constexpr fixed_q7_8 Q7_8_MIN = -32768; // -128.00000000

// PROGMEM this ?
constexpr uint8_t sinTable[] = {0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 49, 53, 57, 62, 66, 70, 75, 79, 83, 87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 127, 131, 135, 139, 143, 146, 150, 153, 157, 160, 164, 167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 198, 201, 204, 206, 209, 211, 214, 216, 219, 221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255};

inline fixed_q7_8 sin_q7_8(fixed_q7_8 angleDeg) {
    // get only the int part of the degree
    int16_t angleInt = angleDeg >> Q7_8_SHIFT; 
    
    angleInt %= 360;
    if (angleInt < 0) angleInt += 360;

    uint8_t lookupIndex;
    int8_t sign = 1; // 1 for positive, -1 for negative

    if (angleInt >= 0 && angleInt <= 90) {
        lookupIndex = angleInt;

    } else if (angleInt > 90 && angleInt <= 180) {
        lookupIndex = 180 - angleInt;

    } else if (angleInt > 180 && angleInt <= 270) {
        lookupIndex = angleInt - 180;
        sign = -1;

    } else { // angleInt > 270 and <= 360
        lookupIndex = 360 - angleInt;
        sign = -1;
    }

    // Lookup the value and apply the sign
    return (sinTable[lookupIndex] * sign);
}

inline fixed_q7_8 cos_q7_8(fixed_q7_8 angleDeg) {
    return sin_q7_8((90 << Q7_8_SHIFT) - angleDeg);
}

inline fixed_q7_8 q7_8_mult(fixed_q7_8 a, fixed_q7_8 b) {
    return (fixed_q7_8)(((int32_t)a * b) >> Q7_8_SHIFT);
}

inline fixed_q7_8 q7_8_div(fixed_q7_8 a, fixed_q7_8 b) {
    if (b == 0) return Q7_8_MAX; // avoid division by zero

    int32_t result = ((int32_t)a << Q7_8_SHIFT) / b;
    
    if (result > Q7_8_MAX) return Q7_8_MAX; // clamp to max value if overflow
    if (result < Q7_8_MIN) return Q7_8_MIN; // clamp to min value if underflow

    return (fixed_q7_8)result;
}

// if the numerator is a 32bit int, use this version to avoid overflow in intermediate calculations
inline fixed_q7_8 q7_8_div_32b(int32_t a, fixed_q7_8 b) {
    if (b == 0) return Q7_8_MAX; // avoid division by zero

    int32_t result = (a << Q7_8_SHIFT) / b;
    
    if (result > Q7_8_MAX) return Q7_8_MAX; // clamp to max value if overflow
    if (result < Q7_8_MIN) return Q7_8_MIN; // clamp to min value if underflow

    return (fixed_q7_8)result;
}

// like q7_8_div but always returns a positive result, needed for cases where large values of a and/or b can cause overflow in intermediate calculations
inline fixed_q7_8 q7_8_div_abs(fixed_q7_8 a, fixed_q7_8 b) {
    if (b == 0) return Q7_8_MAX; // avoid division by zero

    // this is a specific case where we want the absolute value of the result in case the 32bit intermediate is truncated to 0 in 16bit
    // eg, if result is 0xFFFF0000 due to being large and b being small, normal division would truncate to 0 in 16bit, but abs would make it 0x00010000 which when truncated to 16bit becomes 1
    // this is needed for the raycaster to avoid issues with very small deltaDist values causing
    int32_t result = abs(((int32_t)a << Q7_8_SHIFT) / b);
    
    if (result > Q7_8_MAX) return Q7_8_MAX; // clamp to max value if overflow

    return (fixed_q7_8)result;
}

// compile time conversion ONLY
inline fixed_q7_8 double_to_q7_8(double val) {
    return (fixed_q7_8)(val * Q7_8_ONE);
}


/*
    OTHER MATH FUNCTIONS
*/

// integer square root for 16 bit numbers, returns floor(sqrt(n))
// from https://en.wikipedia.org/wiki/Square_root_algorithms#Binary_numeral_system_(base_2)
inline uint32_t isqrt_32(uint32_t n) {
    if (n < 2) {
        return n;
    }

    uint32_t root = 0;
    uint32_t bit = (uint32_t)1 << 30;

    // Find the largest power of 4 (bit pair) <= n
    while (bit > n) {
        bit >>= 2;
    }

    // Perform the bit-by-bit calculation
    while (bit != 0) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit; 
        } else {
            root >>= 1; 
        }
        
        bit >>= 2; // Move to the next lower bit pair
    }
    
    return root; 
}

inline void normalise_2d_q7_8(fixed_q7_8 *x, fixed_q7_8 *y) {

    int32_t magSq = ((int32_t)(*x) * (*x)) + ((int32_t)(*y) * (*y));
    if (magSq == 0) return;

    // sqrt of a q14.16 number, result is q7.8
    fixed_q7_8 mag = (fixed_q7_8)isqrt_32((uint32_t)magSq);
    
    if (mag == 0) return;

    *x = q7_8_div(*x, mag);
    *y = q7_8_div(*y, mag);
}



#endif