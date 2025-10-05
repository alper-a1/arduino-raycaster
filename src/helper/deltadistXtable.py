# deltadistXtable.py
# generate a table of deltadist values for raycasting
# 128 values precomuted for 1/x from x=1/128 to x=1
# stored as Q7.8 fixed point (value * 256 rounded to nearest int)

deltadistTable = []

for i in range(1, 129):
    x = i / 128.0
    deltadist = 1.0 / x
    ddfixed8_8 = int(deltadist * 256 + 0.5)
    deltadistTable.append(ddfixed8_8)

deltadistTable[0] = 0x7FFF  # make sure we dont overflow int16 into negative

# unfortunately, must use int16_t, however works nicely for Q7.8 math later.  
c_deltadistXtable = "constexpr int16_t deltaDistTable[] = {"
c_deltadistXtable += ",".join(str(x) for x in deltadistTable)
c_deltadistXtable += "};"

# print the generated table cpp copy ready
print(c_deltadistXtable)