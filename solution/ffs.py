def linear_to_alaw(pcm_val):
    """Convert a 16-bit linear PCM value to 8-bit A-Law."""
    # Get absolute value of the PCM
    abs_val = abs(pcm_val)
    
    # Clamp the value to the maximum range
    if abs_val > 32635:
        abs_val = 32635

    # Apply A-Law compression
    if abs_val >= 256:
        exponent = (abs_val.bit_length() - 8)  # Get the exponent (position of highest bit)
        mantissa = (abs_val >> (exponent + 3)) & 0x0F  # Get 4 bits of mantissa
        alaw_val = (exponent << 4) | mantissa  # Combine exponent and mantissa
    else:
        alaw_val = abs_val >> 4  # For values < 256, use 4 most significant bits

    # Invert bits and add the sign bit
    alaw_val ^= 0x55  # XOR with 0x55
    if pcm_val < 0:
        alaw_val |= 0x80  # Add the sign bit for negative values

    return alaw_val

def adc_to_alaw(adc_val):
    """Convert a 12-bit ADC value (0...4095) to A-Law encoded byte."""
    # Scale 12-bit ADC value to 16-bit signed PCM range (-32768 to 32767)
    pcm_val = int((adc_val - 2048) / 2048 * 32767)

    # Convert to A-Law
    return linear_to_alaw(pcm_val)

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
    print(f[x], f_byte[x], adc_to_alaw(x))
    
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
