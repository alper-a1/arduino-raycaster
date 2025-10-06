"""
Generates a C++ lookup table for raycaster line height calculations.

This script pre-computes a lookup table (`lhTableApprox`) containing line height values
for integer distances of `perpWallDist`. The values are stored in Q7.8 fixed-point
format and formatted as a C++ `constexpr` array, ready to be copied into the
raycaster's source code.

The table is used to perform fast line height approximations through linear interpolation.
For a detailed explanation of the approximation algorithm, the fixed-point math,
and how the optimal value for the first element (30840) was determined, please
refer to the accompanying `lineHeightOptimisation.py` script.
"""

MAX_PERPWALLDIST_INT = 12

lhTableIntDiv = []

# pre computation line height at integer distances of perpWallDist
for i in range(1, MAX_PERPWALLDIST_INT + 1):
    lh = 128/i
    iq78 = int(lh * 256 + 0.5)
    lhTableIntDiv.append(iq78)
    
# This optimal value is found by the script `lineHeightCalculations.py`
# to minimize visual artifacts when the perpendicular wall distance is between 1.0 and 2.0.
lhTableIntDiv[0] = 30840 


c_lineHeightTable = "constexpr int16_t lhTableApprox[] = {"
c_lineHeightTable += ",".join(str(x) for x in lhTableIntDiv)
c_lineHeightTable += "};"

# print the generated table cpp copy ready
print(c_lineHeightTable)