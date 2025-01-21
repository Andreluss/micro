from math import log
def unsigned12bit_to_alaw(v12bit): 
    assert 0 <= v12bit <= 0xFFF

    A = 87.6
    

    x = -1.0 + 2.0 * v12bit / 4095.0
    assert -1.0 <= x <= 1.0

    sign_x = 1 if x > 0 else -1 if x < 0 else 0
    if abs(x) < (1 / A):
        return sign_x * (A * abs(x)) / (1 + log(A))
    else:
        return sign_x * (1 + log(A * abs(x))) / (1 + log(A))

f = [0] * 4096
f_byte = [0] * 4096
for x in range(0, 4096):
    f[x] = unsigned12bit_to_alaw(x)
    f_byte[x] = round((f[x] + 1.0) / 2.0 * 255.0)
    print(f[x], f_byte[x])
    
# save f[x] to alaw.h file 
with open("alaw.h", "w") as f:
    f.write("#include <stdint.h>\n")
    f.write("const uint8_t ALaw[4096] = {")
    for x in range(0, 4096):
        f.write(str(f_byte[x]))
        if x != 4095:
            f.write(", ")
        if x % 16 == 0:
            f.write("\n")
    f.write("};")
