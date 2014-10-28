#!/usr/bin/env python

import argparse

parser = argparse.ArgumentParser(description="Attach a disk image to a .bin firmware.", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-f', dest='firmware_file', nargs='?', default='enstix.bin', help='Path to firmware file.')
parser.add_argument('-i', dest='image_file', nargs='?', default='image.bin.out', help='Path to disk image file.')
parser.add_argument('-o', dest='output_file', nargs='?', default='FIRMWARE.BIN', help='Path to output bin file (will be overwritten!).')
parser.add_argument('--start-address', dest='start_address', nargs='?', type=int, const=0x6000, default=0x6000, help='Address where the disk image begins in the resulting firmware.')

args = parser.parse_args()

with open(args.firmware_file, 'rb') as f:
    fw_bytes = f.read()
    f.close()

with open(args.image_file, 'rb') as f:
    img_bytes = f.read()
    f.close()

with open(args.output_file, 'wb') as f:
    f.write(fw_bytes.ljust(args.start_address, chr(0xFF)))
    f.write(img_bytes)
    f.close()

