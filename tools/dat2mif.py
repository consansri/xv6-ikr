#!/usr/bin/env python
from argparse import ArgumentParser
import math
import os

VERSION = "V1.1"
parser = ArgumentParser(description='A script to convert a .dat/.bin file to a .mif file (' + VERSION + '), provided by LeyLux group')

parser.add_argument('infile', help='Input .dat/.bin file', metavar='INFILE')
parser.add_argument('outfile', help='Output .mif file', metavar='OUTFILE')

parser.add_argument('-d', '--depth', help="depth of .mif file", type=int, default=1024)
parser.add_argument('-p', '--pad', help="word to use for padding (not given -> no padding)", type=str, default="")
parser.add_argument('-w', '--width', help="width of an output word in bits", type=int, default=32)
parser.add_argument('-or', '--outradix', type=str, default="HEX")
parser.add_argument('-ir', '--inradix', help="radix of input file (ignored for binary files)", type=str, default="HEX")
parser.add_argument('-ar', '--addrradix', type=str, default="HEX")
parser.add_argument('-rbo', '--reverse', nargs="?", type=int, help="Reverse byte order of each block with given size (32 for word) output file")
args = parser.parse_args()

def radixStrToInt(s) -> int:
    if s == "HEX":
        return 16
    elif s == "DEC":
        return 10
    elif s == "OCT":
        return 8
    else:
        try:
            return int(s, base=10)
        except ValueError:
            print("unrecognised radix '{0}', exiting".format(s))
            exit(-1)

def toBase(number, base, paddedlen=None) -> str:
    assert base != 0, "base of 0 doesnt make sense!"
    if paddedlen is not None:
        res = ["0"] * paddedlen
        res_i = 0
    else:
        res = ""
    base_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    while number != 0:
        if paddedlen is not None:
            res[res_i] = base_chars[number % base]
            res_i += 1
        else:
            res += base_chars[number % base]
        number //= base

    if paddedlen is not None:
        return "".join(res[::-1]) or "0" * paddedlen
    else:
        return res[::-1] or "0"

def reverseBits(inword) -> str:
    assert len(inword) % 8 == 0, "to reverse byte order, word size has to be a multiple of 8 bits"
    outword = ""
    inw = inword[:]
    while inw != '':
        outword = inw[:8] + outword
        inw = inw[8:]

    return outword

# reverses order of atom characters in each block
def reverseRegions(indata, atom_size, block_size) -> str:
    assert len(indata) % block_size == 0, "in data has to be a multiple of block size"
    assert block_size % atom_size == 0, "block size has to be a multiple of atom size"
    outdata = ""
    ind = indata[:]
    while ind != '':
        inword = ind[:block_size]
        ind = ind[block_size:]
        outword = ""
        while inword != '':
            outword = inword[:atom_size] + outword
            inword = inword[atom_size:]

        outdata += outword
    return outdata


def __main__(args) -> None:
    parsed_content = ''
    if args.infile.split(".")[:1] == "bin":
        with open(args.infile, 'r') as dat_content:
            for line in dat_content:
                parsed_content += bin(int(line, radixStrToInt(args.inradix)))[2:]
    else:
        with open(args.infile, 'rb') as bin_content:
                parsed_content = bin(int(bin_content.read().hex(), base=16))[2:]
                fsize = os.path.getsize(args.infile)
                parsed_content = "0" * (fsize * 8 - len(parsed_content)) + parsed_content
                parsed_content = parsed_content + "0" * (((args.width - (len(parsed_content) % args.width))))

    assert len(parsed_content) <= args.depth * args.width, "ERROR: input size {} does not fit given depth {}, width {}!".format(len(parsed_content), args.depth, args.width)

    with (open(args.outfile, 'w') as mif_file):
        mif_file.write('-- converted from {0} using dat/bin2mif {1} by LeyLux group\n'.format(args.infile, VERSION))
        mif_file.write('DEPTH = {};\n'.format(args.depth))
        mif_file.write('WIDTH = {};\n'.format(args.width))
        mif_file.write('ADDRESS_RADIX = {};\n'.format(args.addrradix))
        mif_file.write('DATA_RADIX = {};\n\n'.format(args.outradix))
        mif_file.write('CONTENT\n')
        mif_file.write('BEGIN\n')

        words_num = int(math.ceil((len(parsed_content)) / args.width))

        required_digits = len(toBase(2**args.width - 1, radixStrToInt(args.outradix)))
        for word_index in range(0, words_num):
            bits = parsed_content[(word_index*args.width):((word_index+1)*args.width)]
            if args.reverse is not None:
                bits = reverseRegions(bits, 8, args.reverse)[:]

            word = int(bits, base=2)
            if word_index != words_num - 1:
                word_str = toBase(word, radixStrToInt(args.outradix), required_digits)
            else:
                missing_bits = len(parsed_content) - args.width * word_index
                missing_digits = len(toBase(2**missing_bits - 1, base=radixStrToInt(args.outradix)))
                word_str = toBase(word, radixStrToInt(args.outradix), missing_digits)

                word_str = word_str + "0" * (required_digits - missing_digits)

            mif_file.write('{0}: {1};\n'.format(toBase(word_index, radixStrToInt(args.addrradix)), word_str))

        if args.pad != "":
            try:
                int(args.pad, base=radixStrToInt(args.outradix))
            except ValueError:
                print("WARNING: pad data '{0}' does not match radix ’{1}’".format(args.pad, args.outradix))

            if len(args.pad) != required_digits:
                print("WARNING: pad data did not reach word width and got padded!")

            pad_word = args.pad + "0" * (required_digits - len(args.pad))
            if args.reverse:
                pad_word = reverseBits(pad_word)
            mif_file.write('[{0}..{1}] : {2};\n'.format(toBase(words_num, radixStrToInt(args.addrradix)), toBase(args.depth - 1, radixStrToInt(args.addrradix)), pad_word))

        mif_file.write("END;")

        print("converted a total of {} bits".format(len(parsed_content)))


if __name__ == "__main__":
        __main__(args)