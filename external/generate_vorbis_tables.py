from vorbis_headers import lookup
from binascii import hexlify

VORB_STRUCT = """
typedef struct VorbHeader{
    uint32 crc;
    uint32 data_size;
    const uint8* data;
}VorbHeader;

"""


def c_hex(data: bytes) -> str:
    return r"\x" + r"\x".join(hexlify(data, " ").decode().split(" "))


def main():
    """Convert vorbis headers bytes into C arrays and generate static C array of structs{crc, bytes ptr, bytes len} sorted by crc"""
    c_output = "#include <int_def.h>\n"

    c_output += VORB_STRUCT
    for crc, data in lookup.items():
        c_output += f"static const uint8 vorb_{crc}[] = \"{c_hex(data)}\";\n\n"

    c_output += "static const VorbHeader vorb_headers[] = {\n"
    for crc in sorted(lookup.keys()):
        data = lookup[crc]
        c_output += f"    {{0x{crc:08x}, sizeof(vorb_{crc}), vorb_{crc}}},\n"
    c_output += "};\n"
    c_output += "static const uint32 vorb_headers_count = sizeof(vorb_headers) / sizeof(VorbHeader);\n"
    with open("./../include/vorbis_headers.h", "w") as f:
        f.write(c_output)


if __name__ == '__main__':
    main()
