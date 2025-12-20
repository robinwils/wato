#!/usr/bin/env python3

import struct
import sys
import os
from pathlib import Path

CORPUS_DIR = "new_corpus"


def write_test_data(
    filename,
    i32_val,
    i32_min,
    i32_max,
    ui32_val,
    ui32_min,
    ui32_max,
    f32_val,
    f32_min,
    f32_max,
    bool_val,
    ui64_val,
    ui64_min,
    ui64_max,
):
    """
    format (177 bytes total):
    - int32_t value + int64_t min + max (20 bytes)
    - uint32_t value + uint64_t min + max (20 bytes)
    - float value + min + max (12 bytes)
    - bool (1 byte)
    - glm::vec3 float + min + max (20 bytes)
    - glm::vec2 uint32_t + uint64_t min + max (24 bytes)
    - glm::vec4 int32_t + int64_t min + max (32 bytes)
    - uint64_t value + uint64_t min + max (24 bytes)
    - std::vector 3 floats value + float min + max + size (24 bytes)
    """
    data = struct.pack(
        "<i q q I Q Q f f f B f f f f f I I Q Q i i i i q q Q Q Q I f f f f f",
        i32_val,
        i32_min,
        i32_max,  # ValueWithBounds<int32_t, int64_t>
        ui32_val,
        ui32_min,
        ui32_max,  # ValueWithBounds<uint32_t, uint64_t>
        f32_val,
        f32_min,
        f32_max,  # ValueWithBounds<float, float>
        1 if bool_val else 0,  # bool
        f32_val,
        f32_min,
        f32_max,
        f32_min,
        f32_max,  # GLMVecWithBounds<3, float, float>
        ui32_val,
        ui32_max,
        ui32_min,
        ui32_max,  # GLMVecWithBounds<2, uint32_t, uint64_t>
        i32_val,
        i32_min,
        i32_max,
        int(i32_max / 2),
        i32_min,
        i32_max,  # GLMVecWithBounds<4, int32_t, int64_t>
        ui64_val,
        ui64_min,
        ui64_max,  # ValueWithBounds<uint64_t, uint64_t>
        3,  # vector size
        f32_val,
        f32_min,
        f32_max,
        f32_min,
        f32_max,  # VectorWithBounds<float, float>
    )

    Path(filename).parent.mkdir(parents=True, exist_ok=True)
    with open(filename, "wb") as f:
        f.write(data)
    print(f"✓ Generated: {filename} ({len(data)} bytes)")


def main():
    write_test_data(
        f"{CORPUS_DIR}/basic",
        42,
        0,
        100,  # int32
        50,
        0,
        100,  # uint32
        3.14,
        0.0,
        10.0,  # float
        True,
        42,
        0,
        42000,
    )

    write_test_data(
        f"{CORPUS_DIR}/negative",
        -42,
        -100,
        100,
        50,
        0,
        100,
        -3.14,
        -10.0,
        10.0,
        False,
        42,
        0,
        42000,
    )

    write_test_data(
        f"{CORPUS_DIR}/type_limits",
        2147483647,
        -2147483648,
        2147483647,
        4294967295,
        0,
        4294967295,
        3.40282e38,
        -3.40282e38,
        3.40282e38,
        True,
        18446744073709551615,
        0,
        18446744073709551615,
    )

    write_test_data(
        f"{CORPUS_DIR}/zero", 0, 0, 0, 0, 0, 0, 0.0, 0.0, 0.0, False, 0, 0, 0
    )

    write_test_data(
        f"{CORPUS_DIR}/at_min_max",
        -100,
        -100,
        100,
        0,
        0,
        10,
        10.0,
        -30.0,
        10.0,
        False,
        0,
        0,
        1,
    )

    write_test_data(
        f"{CORPUS_DIR}/float_negative",
        50,
        0,
        100,
        50,
        0,
        100,
        -99.9,
        -100.0,
        100.0,
        True,
        0,
        0,
        1,
    )


if __name__ == "__main__":
    main()
    print(f"\n✅ Generated {len(list(Path(CORPUS_DIR).glob('*')))} corpus files")
