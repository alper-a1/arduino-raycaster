#include "fixedpoint.h"

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
    return (sinTable[lookupIndex] * sign) << Q7_8_SHIFT;
}

inline fixed_q7_8 cos_q7_8(fixed_q7_8 angleDeg) {
    return sin_q7_8((90 << Q7_8_SHIFT) - angleDeg);
}

inline fixed_q7_8 q7_8_mult(fixed_q7_8 a, fixed_q7_8 b) {
    return (fixed_q7_8)(((int32_t)a * b) >> Q7_8_SHIFT);
}

inline fixed_q7_8 q7_8_div(fixed_q7_8 a, fixed_q7_8 b) {
    return (fixed_q7_8)(((int32_t)a << Q7_8_SHIFT) / b);
}
