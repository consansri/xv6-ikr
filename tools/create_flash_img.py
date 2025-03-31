#!/usr/bin/env python
from argparse import ArgumentParser
import math
import os
import binascii
from intelhex import IntelHex

parser = ArgumentParser(description='A script to create .vmf files for the cfg flash to boot the IKR-RV64 from')


parser.add_argument('infile', help='Input .bin file', metavar='INFILE')
parser.add_argument('outfile', help='Output .vmf file', metavar='OUTFILE')
parser.add_argument('fl_offset', help='Offset of the binary data in the .vmf file (hex)', type=str, default="2000000", nargs="?")
parser.add_argument('im_offset', help='SoC address to copy the .bin file to (hex)', type=str, default="00000", nargs="?")
parser.add_argument('boot_addr', help='SoC address to start execution from', type=str, default="00000", nargs="?")
parser.add_argument('-p','--payload',nargs=2,metavar=('offset', 'filename'),help='add payload file to flash image not to be included in the bootloader image')


args = parser.parse_args()

def __main__(args) -> None:

    with open(args.infile, "rb") as binfile:
        content = binfile.read()
        content_str = str(binascii.hexlify(content))[2:-1]
        pad_nibbles_num = len(content_str) % 16
        content_str = content_str + "0" * pad_nibbles_num

    size_str = hex(math.ceil(len(content_str) / 16) * 8)[2:].rjust(16, "0")

    with open(args.outfile, "w") as vmffile:
        # write start address in flash
        vmffile.write("@{}\n".format(args.fl_offset))

        # write bootloader image header
        tgt_addr_str = args.im_offset.rjust(16, "0")
        boot_addr_str = args.boot_addr.rjust(16, "0")
        vmffile.write("{7}\n{6}\n{5}\n{4}\n{3}\n{2}\n{1}\n{0}\n\n".format(tgt_addr_str[0:2], tgt_addr_str[2:4], tgt_addr_str[4:6], tgt_addr_str[6:8], tgt_addr_str[8:10], tgt_addr_str[10:12], tgt_addr_str[12:14], tgt_addr_str[14:16]))
        vmffile.write("{7}\n{6}\n{5}\n{4}\n{3}\n{2}\n{1}\n{0}\n\n".format(boot_addr_str[0:2], boot_addr_str[2:4], boot_addr_str[4:6], boot_addr_str[6:8], boot_addr_str[8:10], boot_addr_str[10:12], boot_addr_str[12:14], boot_addr_str[14:16]))
        

        vmffile.write("{7}\n{6}\n{5}\n{4}\n{3}\n{2}\n{1}\n{0}\n\n".format(size_str[0:2], size_str[2:4], size_str[4:6], size_str[6:8], size_str[8:10], size_str[10:12], size_str[12:14], size_str[14:16]))
        
        chksum = 0xFFFFFFFFFFFFFFFF
        chksum = chksum ^ int(tgt_addr_str, 16)
        chksum = chksum ^ int(boot_addr_str, 16)
        chksum = chksum ^ int(size_str, 16)


        for dword_str in [content_str[i:i+16] for i in range(0, len(content_str), 16)]:
            chksum = chksum ^ int("".join(reversed([dword_str[i:i+2] for i in range(0, len(dword_str), 2)])), 16)

        chksum_str = hex(chksum)[2:].rjust(16, "0")
        # write payload checksum 
        vmffile.write("{7}\n{6}\n{5}\n{4}\n{3}\n{2}\n{1}\n{0}\n\n".format(chksum_str[0:2], chksum_str[2:4], chksum_str[4:6], chksum_str[6:8], chksum_str[8:10], chksum_str[10:12], chksum_str[12:14], chksum_str[14:16]))
        
        # write bootloader payload
        for byte_str in [content_str[i:i+2] for i in range(0, len(content_str), 2)]:
            vmffile.write("{}\n".format(byte_str))


        if args.payload != None:
            # write additional payload
            tgt_addr_str = args.payload[0].rjust(8, "0")
            vmffile.write("\n\n\n\n\n@{}\n".format(tgt_addr_str))

            with open(args.payload[1], "rb") as binfile:
                payload_content = binfile.read()
                payload_str = str(binascii.hexlify(payload_content))[2:-1]

            for byte_str in [payload_str[i:i+2] for i in range(0, len(payload_str), 2)]:
                vmffile.write("{}\n".format(byte_str))

    hex_cnt = IntelHex()
    off = int(args.fl_offset, 16)
    for i in range(8):
        hex_cnt[off + i] = int(args.im_offset.rjust(16, "0")[(14 - 2*i):(14 - 2*i + 2)], 16)
        hex_cnt[off + i + 8] = int(args.boot_addr.rjust(16, "0")[(14 - 2*i):(14 - 2*i + 2)], 16)
        hex_cnt[off + i + 16] = int(size_str[(14 - 2*i):(14 - 2*i + 2)], 16)
        hex_cnt[off + i + 24] = int(chksum_str[(14 - 2*i):(14 - 2*i + 2)], 16)

    for i in range(math.ceil(len(content_str) / 2)):
        hex_cnt[off + 32 + i] = int(content_str[(2*i):(2*i + 2)], 16)

    if args.payload != None:
        off = int(args.payload[0], 16)

        for i in range(math.ceil(len(payload_str) / 2)):
            hex_cnt[off + i] = int(payload_str[(2*i):(2*i + 2)], 16)

    hex_cnt.write_hex_file("".join(args.outfile.split(".")[:-1]) + ".hex")

if __name__ == "__main__":
        __main__(args)
