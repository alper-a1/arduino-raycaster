# deltadistXtable.py

deltadistTable = []

for i in range(1, 128):
    x = i / 127.0
    deltadist = 1.0 / x
    ddfixed8_8 = int(deltadist * 256 + 0.5)
    deltadistTable.append(ddfixed8_8)

# unfortunately, must use int16_t, however works nicely for Q8.8 math later.  
c_deltadistXtable = "constexpr int16_t deltaDistTable[] = {"
c_deltadistXtable += ",".join(str(x) for x in deltadistTable)
c_deltadistXtable += "};"

# print the generated table cpp copy ready
# print(c_deltadistXtable)
print(deltadistTable[63])