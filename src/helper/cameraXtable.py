# cameraXtable.py
# Generates a table of cameraX values in Q1.7 format for screenX values from 80 to 160 
# (the right half of the screen). The left half can be derived by negating these values.
# this table is used to avoid division in the raycaster loop and to speed up calculations.

SCREEN_WIDTH = 160

cameraXtable = []

for x in range(80, 161):
    cameraX = (2*x / SCREEN_WIDTH) - 1
    camXQ1_7 = int((cameraX * 2**7) + 0.5)
    cameraXtable.append(camXQ1_7)
    
cameraXtable[-1] -= 1  # fit the last value into int8    

c_cameraXtable = "constexpr int8_t cameraXtable[] = {"
c_cameraXtable += ",".join(str(x) for x in cameraXtable)
c_cameraXtable += "};"

# print the generated table cpp copy ready
print(c_cameraXtable)


# testing to make sure that the table produces the correct values

# testX = 50
# for currentScreenX in range(0, 161):
#     if currentScreenX < 80:
#         cameraX = ~cameraXtable[80 - currentScreenX] + 1; 
#     else: 
#         cameraX = cameraXtable[currentScreenX - 80]; 
    
#     if currentScreenX == testX: 
#         print(cameraX)