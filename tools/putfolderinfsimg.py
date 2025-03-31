#!/usr/bin/env python
import os
import pyfatfs
from pyfatfs.PyFatFS import PyFatFS
import pyfatfs.PyFatFS as fs
from argparse import ArgumentParser
from io import BytesIO


VERSION = "V1.0"
parser = ArgumentParser(description='A script to put a folders contents into a FAT32 fs image (' + VERSION + '), provided by LeyLux group')

parser.add_argument('infolder', help='Input folder', metavar='INFILE')
parser.add_argument('outfile', help='file system image file', metavar='OUTFILE')

args = parser.parse_args()


def __main__(args) -> None:
    pf = PyFatFS(args.outfile)
    pf.makedir("bin")
    allFiles = []
    for root, dirs, files in os.walk(args.infolder):
        for file in files:
            allFiles.append(os.path.join(root, file))


    allFilesNoMainDir = [x[(len(args.infolder)+1):] for x in allFiles]

    for file in allFilesNoMainDir:

        with pf.open(file, "wb") as dst:
            with open(os.path.join(args.infolder, file), "rb") as src:
                dst.write(src.read())



if __name__ == "__main__":
        __main__(args)
