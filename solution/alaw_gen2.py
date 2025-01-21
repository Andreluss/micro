# Python script to precompute a C array for A-law compression for 12-bit unsigned samples

def precompute_alaw_table():
    # ITU-T G.711 A-law constants
    ALAW_MAX = 32767
    aLawTable = []

    for i in range(-ALAW_MAX, ALAW_MAX + 1):
        # Step 1: Get absolute value
        abs_val = abs(i)

        # Step 2: Determine the segment
        if abs_val < 256:
            segment = 0
        elif abs_val < 512:
            segment = 1
        elif abs_val < 1024:
            segment = 2
        elif abs_val < 2048:
            segment = 3
        elif abs_val < 4096:
            segment = 4
        elif abs_val < 8192:
            segment = 5
        elif abs_val < 16384:
            segment = 6
        else:
            segment = 7

        # Step 3: Quantize the magnitude
        quantized = (abs_val >> (segment + 3)) & 0x0F  # 4 bits of quantization

        # Step 4: Combine segment and quantized magnitude
        alaw_byte = (segment << 4) | quantized

        # Step 5: Add sign bit
        if i < 0:
            alaw_byte |= 0x80

        # Final inversion for transmission format
        alaw_byte ^= 0xD5
        aLawTable.append(alaw_byte)

    return aLawTable

def generate_c_array_for_12bit_unsigned():
    # Precompute the A-law table for all 12-bit unsigned inputs
    alaw_table = precompute_alaw_table()
    c_array = []

    for i in range(4096):  # Iterate over all 12-bit unsigned values
        # Convert 12-bit unsigned to 16-bit signed
        signed_sample = (i - 2048) * 16
        # Map to the precomputed A-law table
        alaw_byte = alaw_table[signed_sample + 32768]
        c_array.append(alaw_byte)

    return c_array

# Generate the C array
c_array = generate_c_array_for_12bit_unsigned()

# Format the array into a C-style static array
c_code = "const uint8_t alaw_12bit_table[4096] = {\n"
for i, val in enumerate(c_array):
    c_code += f"0x{val:02X}, "
    if (i + 1) % 16 == 0:  # Line break every 16 values for readability
        c_code += "\n"
c_code = c_code.strip(", \n") + "\n};"

# Save the C array to a file
with open("alaw_12bit_table.c", "w") as f:
    f.write(c_code)

c_code
