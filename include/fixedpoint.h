/*
    Fixed Point Number implementation
    using 16 bit ints with 8 fractional bits (-128 to 127.99609375) 

*/


#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <Arduino.h>

// PROGMEM this ?
constexpr uint8_t sinTable[] = {0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 49, 53, 57, 62, 66, 70, 75, 79, 83, 87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 127, 131, 135, 139, 143, 146, 150, 153, 157, 160, 164, 167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 198, 201, 204, 206, 209, 211, 214, 216, 219, 221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255};

// inline fixed_q7_8 sin_q7_8(fixed_q7_8 angleDeg) {
//     // get only the int part of the degree
//     int16_t angleInt = angleDeg >> Q7_8_SHIFT; 
    
//     angleInt %= 360;
//     if (angleInt < 0) angleInt += 360;

//     uint8_t lookupIndex;
//     int8_t sign = 1; // 1 for positive, -1 for negative

//     if (angleInt >= 0 && angleInt <= 90) {
//         lookupIndex = angleInt;

//     } else if (angleInt > 90 && angleInt <= 180) {
//         lookupIndex = 180 - angleInt;

//     } else if (angleInt > 180 && angleInt <= 270) {
//         lookupIndex = angleInt - 180;
//         sign = -1;

//     } else { // angleInt > 270 and <= 360
//         lookupIndex = 360 - angleInt;
//         sign = -1;
//     }

//     // Lookup the value and apply the sign
//     return (sinTable[lookupIndex] * sign);
// }

// inline fixed_q7_8 cos_q7_8(fixed_q7_8 angleDeg) {
//     return sin_q7_8((90 << Q7_8_SHIFT) - angleDeg);
// }

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

// inline void normalise_2d_q7_8(fixed_q7_8 *x, fixed_q7_8 *y) {

//     int32_t magSq = ((int32_t)(*x) * (*x)) + ((int32_t)(*y) * (*y));
//     if (magSq == 0) return;

//     // sqrt of a q14.16 number, result is q7.8
//     fixed_q7_8 mag = (fixed_q7_8)isqrt_32((uint32_t)magSq);
    
//     if (mag == 0) return;

//     *x = q7_8_div(*x, mag);
//     *y = q7_8_div(*y, mag);
// }

// constexpr uint16_t sin[] = {0, 1144, 2287, 3430, 4572, 5712, 6850, 7987, 9121, 10252, 11380, 12505, 13626, 14742, 15855, 16962, 18064, 19161, 20252, 21336, 22415, 23486, 24550, 25607, 26656, 27697, 28729, 29753, 30767, 31772, 32768, 33754, 34729, 35693, 36647, 37590, 38521, 39441, 40348, 41243, 42126, 42995, 43852, 44695, 45525, 46341, 47143, 47930, 48703, 49461, 50203, 50931, 51643, 52339, 53020, 53684, 54332, 54963, 55578, 56175, 56756, 57319, 57865, 58393, 58903, 59396, 59870, 60326, 60764, 61183, 61584, 61966, 62328, 62672, 62997, 63303, 63589, 63856, 64104, 64332, 64540, 64729, 64898, 65048, 65177, 65287, 65376, 65446, 65496, 65526, 65535};


// fixedpoint class implementation to clean up code
class Fixed16_16 {

    private:
        int32_t value; 
        static constexpr uint8_t FRAC_SHIFT = 16;
        static constexpr int32_t ONE = ((int32_t)1 << FRAC_SHIFT); 
        
        // from raw (pre scaled) 
        constexpr explicit Fixed16_16(const int32_t raw, bool) : value(raw) {} 

        // constexpr compatible round for compile time double conversion
        static constexpr int32_t constexpr_round(double x) {
            return (x >= 0.0) ? static_cast<int32_t>(x + 0.5) : static_cast<int32_t>(x - 0.5);
        }

    public:
        Fixed16_16() : value(0) {};
        
        Fixed16_16(const Fixed16_16&) = default;
        Fixed16_16& operator=(const Fixed16_16&) = default;

        // from double !-- USE COMPILE TIME ONLY --!
        constexpr Fixed16_16(const double dVal) : value(static_cast<int32_t>(constexpr_round(dVal * ONE))) {} 
        
        // can overflow - care
        constexpr explicit Fixed16_16(int iVal) : value(static_cast<int32_t>(iVal) << FRAC_SHIFT) {} 
        
        static constexpr Fixed16_16 fromRaw(const int32_t raw) {
            return Fixed16_16(raw, true);
        }

        inline int16_t toInt() const {
            return static_cast<int16_t>(value >> FRAC_SHIFT);
        }

        constexpr int32_t toRaw() const {
            return value;
        }

        /* operator overloads */ 


        friend bool operator==(const Fixed16_16 &a, const Fixed16_16 &b);
        friend bool operator<(const Fixed16_16 &a, const Fixed16_16 &b);

        inline Fixed16_16& operator+=(const Fixed16_16 &other) {
            value += other.value;
            return *this;
        }

        inline Fixed16_16& operator-=(const Fixed16_16 &other) {
            value -= other.value;
            return *this;
        }
        
        inline Fixed16_16& operator*=(const Fixed16_16 &other) {
            value = (static_cast<int64_t>(value) * other.value) >> FRAC_SHIFT;
            return *this;
        }

        inline Fixed16_16& operator/=(const Fixed16_16 &other) {
            if (other == 0) {
                value = INT32_MAX; // avoid division by zero, set to max value
                return *this;
            }

            value = ((static_cast<int64_t>(value) << FRAC_SHIFT) / other.value);
            return *this;
        }

        inline Fixed16_16 operator-() const {
            return Fixed16_16::fromRaw(-value);
        }
};

// math operators for fixed point class


inline Fixed16_16 operator+(const Fixed16_16 &a, const Fixed16_16 &b) {
    return Fixed16_16(a) += b;
}

inline Fixed16_16 operator-(const Fixed16_16 &a, const Fixed16_16 &b) {
    return Fixed16_16(a) -= b;
}

inline Fixed16_16 operator*(const Fixed16_16 &a, const Fixed16_16 &b) {
    return Fixed16_16(a) *= b;
}

inline Fixed16_16 operator/(const Fixed16_16 &a, const Fixed16_16 &b) {
    return Fixed16_16(a) /= b;
}

// comparison operators for fixed point class


inline bool operator==(const Fixed16_16 &a, const Fixed16_16 &b) {
    return a.value == b.value;
}

inline bool operator!=(const Fixed16_16 &a, const Fixed16_16 &b) {
    return !(a == b);
}

inline bool operator<(const Fixed16_16 &a, const Fixed16_16 &b) {
    return a.value < b.value;
}

inline bool operator>(const Fixed16_16 &a, const Fixed16_16 &b) {
    return b < a;
}

inline bool operator>=(const Fixed16_16 &a, const Fixed16_16 &b) {
    return !(a < b);
}

inline bool operator<=(const Fixed16_16 &a, const Fixed16_16 &b) {
    return !(a > b);
}




// boring int16 / fixedpoint overloads

inline Fixed16_16 operator+(const Fixed16_16 &a, const int16_t b) {
    return a + Fixed16_16(b);
}

inline Fixed16_16 operator+(const int16_t a, const Fixed16_16 &b) {
    return Fixed16_16(a) + b;
}

inline Fixed16_16 operator-(const Fixed16_16 &a, const int16_t b) {
    return a - Fixed16_16(b);
}

inline Fixed16_16 operator-(const int16_t a, const Fixed16_16 &b) {
    return Fixed16_16(a) - b;
}

inline Fixed16_16 operator*(const Fixed16_16 &a, const int16_t b) {
    return a * Fixed16_16(b);
}

inline Fixed16_16 operator*(const int16_t a, const Fixed16_16 &b) {
    return Fixed16_16(a) * b;
}

inline Fixed16_16 operator/(const Fixed16_16 &a, const int16_t b) {
    return a / Fixed16_16(b);
}

inline Fixed16_16 operator/(const int16_t a, const Fixed16_16 &b) {
    return Fixed16_16(a) / b;
}
#endif