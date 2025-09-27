import math



# calculate and generate a uint8_t sin table for degrees 0-90
# we are using degrees here as its easier than storing radians in fixed16_8_t
sintable = []

for d in range(91):
    rad = math.radians(d)
    
    q7_8_sin_val = int(round(math.sin(rad) * 255))
    
    sintable.append(q7_8_sin_val)
    

cpp_sin_table = "constexpr uint16_t sin[] = {"

for i, v in enumerate(sintable):
    cpp_sin_table += f"{v}"
    if i != len(sintable) - 1:
        cpp_sin_table += ", "

cpp_sin_table += "};"

print(cpp_sin_table)