/*
    Fixed16_16 (Q15.16 Format)
    --------------------------
    Storage:    int32_t (Signed)
    Resolution: ~0.0000152 (1/65536)
    Range:      [-32768.0, +32767.99998]
    
    NOTE: Division rounds toward zero (truncation).
    NOTE: Double constructor does NOT clamp overflow.
*/
   
#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H
   
#include <Arduino.h>
   
class Fixed16_16 {

    private:
        int32_t value; 
        static constexpr uint8_t FRAC_SHIFT = 16;
        static constexpr int32_t ONE = ((int32_t)1 << FRAC_SHIFT); 
        
        // from raw (pre scaled) -- bool in argument so compiler isnt confused on which constructor to use 
        constexpr explicit Fixed16_16(const int32_t raw, bool) : value(raw) {} 

        // constexpr compatible round for compile time double conversion
        static constexpr int32_t constexpr_round(double x) {
            return (x >= 0.0) ? static_cast<int32_t>(x + 0.5) : static_cast<int32_t>(x - 0.5);
        }

    public:
        Fixed16_16() : value(0) {};
        
        Fixed16_16(const Fixed16_16&) = default;
        Fixed16_16& operator=(const Fixed16_16&) = default;

        // from double !-- COMPILE TIME USE ONLY --!
        constexpr Fixed16_16(const double dVal) : value(static_cast<int32_t>(constexpr_round(dVal * ONE))) {} 
        
        // can overflow - care that iVal is <= INT16_MAX
        constexpr explicit Fixed16_16(int iVal) : value(static_cast<int32_t>(iVal) << FRAC_SHIFT) {} 
        
        static constexpr Fixed16_16 fromRaw(const int32_t raw) {
            return Fixed16_16(raw, true);
        }

        inline constexpr int16_t toInt() const {
            return static_cast<int16_t>(value >> FRAC_SHIFT);
        }

        inline constexpr int32_t toRaw() const {
            return value;
        }

        /* operator overloads */ 


        friend bool operator==(const Fixed16_16 a, const Fixed16_16 b);
        friend bool operator<(const Fixed16_16 a, const Fixed16_16 b);

        inline Fixed16_16& operator+=(const Fixed16_16 other) {
            value += other.value;
            return *this;
        }

        inline Fixed16_16& operator-=(const Fixed16_16 other) {
            value -= other.value;
            return *this;
        }
        
        inline Fixed16_16& operator*=(const Fixed16_16 other) {
            value = (static_cast<int64_t>(value) * other.value) >> FRAC_SHIFT;
            return *this;
        }

        inline Fixed16_16& operator/=(const Fixed16_16 other) {
            if (other == 0) {
                value = (value >= 0) ? INT32_MAX : INT32_MIN; // avoid division by zero, set to max value
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


inline Fixed16_16 operator+(const Fixed16_16 a, const Fixed16_16 b) {
    return Fixed16_16(a) += b;
}

inline Fixed16_16 operator-(const Fixed16_16 a, const Fixed16_16 b) {
    return Fixed16_16(a) -= b;
}

inline Fixed16_16 operator*(const Fixed16_16 a, const Fixed16_16 b) {
    return Fixed16_16(a) *= b;
}

inline Fixed16_16 operator/(const Fixed16_16 a, const Fixed16_16 b) {
    return Fixed16_16(a) /= b;
}

// comparison operators for fixed point class


inline bool operator==(const Fixed16_16 a, const Fixed16_16 b) {
    return a.value == b.value;
}

inline bool operator!=(const Fixed16_16 a, const Fixed16_16 b) {
    return !(a == b);
}

inline bool operator<(const Fixed16_16 a, const Fixed16_16 b) {
    return a.value < b.value;
}

inline bool operator>(const Fixed16_16 a, const Fixed16_16 b) {
    return b < a;
}

inline bool operator>=(const Fixed16_16 a, const Fixed16_16 b) {
    return !(a < b);
}

inline bool operator<=(const Fixed16_16 a, const Fixed16_16 b) {
    return !(a > b);
}




// boring int16 / fixedpoint overloads

inline Fixed16_16 operator+(const Fixed16_16 a, const int16_t b) {
    return a + Fixed16_16(b);
}

inline Fixed16_16 operator+(const int16_t a, const Fixed16_16 b) {
    return Fixed16_16(a) + b;
}

inline Fixed16_16 operator-(const Fixed16_16 a, const int16_t b) {
    return a - Fixed16_16(b);
}

inline Fixed16_16 operator-(const int16_t a, const Fixed16_16 b) {
    return Fixed16_16(a) - b;
}

inline Fixed16_16 operator/(const Fixed16_16 a, const int16_t b) {
    return a / Fixed16_16(b);
}

inline Fixed16_16 operator/(const int16_t a, const Fixed16_16 b) {
    return Fixed16_16(a) / b;
}

// minor optimisation for multiplication, assuming overflow-safe values, we can skip the casting of b into a Fixed Point, saving cycles of multiplication / division by Fixed16_16 ONE
inline Fixed16_16 operator*(const Fixed16_16 a, const int16_t b) {
    return Fixed16_16::fromRaw(a.toRaw() * b);
}

inline Fixed16_16 operator*(const int16_t a, const Fixed16_16 b) {
    return Fixed16_16::fromRaw(a * b.toRaw());
}




constexpr uint16_t sinTable[] = {0, 1144, 2287, 3430, 4572, 5712, 6850, 7987, 9121, 10252, 11380, 12505, 13626, 14742, 15855, 16962, 18064, 19161, 20252, 21336, 22415, 23486, 24550, 25607, 26656, 27697, 28729, 29753, 30767, 31772, 32768, 33754, 34729, 35693, 36647, 37590, 38521, 39441, 40348, 41243, 42126, 42995, 43852, 44695, 45525, 46341, 47143, 47930, 48703, 49461, 50203, 50931, 51643, 52339, 53020, 53684, 54332, 54963, 55578, 56175, 56756, 57319, 57865, 58393, 58903, 59396, 59870, 60326, 60764, 61183, 61584, 61966, 62328, 62672, 62997, 63303, 63589, 63856, 64104, 64332, 64540, 64729, 64898, 65048, 65177, 65287, 65376, 65446, 65496, 65526, 65535};

inline Fixed16_16 sin_q7_8(Fixed16_16 angleDeg) {
    // get only the int part of the degree
    int16_t angleInt = angleDeg.toInt(); 
    
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
    return Fixed16_16::fromRaw(sinTable[lookupIndex] * sign);
}

inline Fixed16_16 cos_q7_8(Fixed16_16 angleDeg) {
    return sin_q7_8((90 - angleDeg));
}


#endif