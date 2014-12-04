#!/usr/bin/env python

BLOCK_SIZE=512
HASH_ITERATIONS=1000

from Crypto.Cipher import AES
from Crypto.Hash import SHA256
from Crypto import Random

import getpass
import binascii
import struct
import sys

import argparse

parser = argparse.ArgumentParser(description="Encrypt and decrypt disk images (for usage with enstix).", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-i', '--input', dest='input_imgfile', nargs='?', default='image.bin', help='Path to input disk image file.')
parser.add_argument('-o', '--output', dest='output_imgfile', nargs='?', default='image.bin.out', help='Path to output image file (will be overwritten!).')
parser.add_argument('-k', '--key', dest='key', help='Encrypted main AES key, 16 hexified bytes (32 chars). If no supplied, a new random one will be generated.')
parser.add_argument('-d', '--decrypt', dest='decrypt', action='store_true', help="Decrypt (instead of the default encrypting) the input file; supplying a key is mandatory.")
parser.add_argument('-N', '--no-eeprom', dest='no_eeprom', action='store_true', help="Do not update the eeprom C source file with the generated password.")
parser.add_argument('-e', '--eeprom-file', dest='eeprom_file', nargs='?', default='eeprom_contents.c', help="C source file for eeprom variables (gets overwritten!).")

args = parser.parse_args()

if(args.decrypt and not args.key):
    print("Error: A key needs to be supplied for decrypting.")
    exit(1)

aes128_key_encr = ''
if args.key:
    if len(args.key) == 32:
        try:
            aes128_key_encr = binascii.unhexlify(args.key)
        except TypeError as e:
            print("Error: Couldn't unhexify the supplied key.")
            exit(1)
    else:
        print("Error: The supplied encrypted AES key has wrong length.")
        exit(1)

passphrase = getpass.getpass("Passphrase: ")
pass_check = getpass.getpass("Repeat passphrase: ")

if(passphrase != pass_check):
    print("The passphrases don't match!")
    exit(0)

print("Passphrases match, continuing...")

pp_hash = SHA256.new(passphrase)
for i in range(1,HASH_ITERATIONS):
    pp_hash = SHA256.new(pp_hash.digest())
print("SHA256 hash^"+str(HASH_ITERATIONS)+" of the passphrase: "+pp_hash.hexdigest())
pp_hash_hash = SHA256.new(pp_hash.digest())
for i in range(1,HASH_ITERATIONS):
    pp_hash_hash = SHA256.new(pp_hash_hash.digest())
print("SHA256^"+str(HASH_ITERATIONS)+" hash of the hash^"+str(HASH_ITERATIONS)+" of the passphrase (saved to EEPROM, used to check if the passphrase is correct: "+pp_hash_hash.hexdigest())

# Use 1st 16 bytes of SHA256 hash of passphrase a key for AES128 used for encrypting the actual key
aes_hashkey = AES.new(pp_hash.digest()[0:16], AES.MODE_ECB) # only used to encr/decr 1 block, so ECB is appropriate
                                                            # also cut the length of the key to 16 bytes, so that AES128 is used

if len(aes128_key_encr) > 0:
    aes128_key = aes_hashkey.decrypt(aes128_key_encr)
else:
    aes128_key = Random.new().read(16)
    aes128_key_encr = aes_hashkey.encrypt(aes128_key)

print("AES128 key (the main key; random): "+binascii.hexlify(aes128_key))
print("The main key encrypted with AES128; the key is the passphrase hash (saved to EEPROM): "+binascii.hexlify(aes128_key_encr))

# encrypt in chunks, to get "sector number"
sect_num = 0
if args.decrypt:
    print("Decrypting image '"+args.input_imgfile+"' to '"+args.output_imgfile+"' now:")
else:
    print("Encrypting image '"+args.input_imgfile+"' to '"+args.output_imgfile+"' now:")
aes_iv = AES.new(SHA256.new(aes128_key).digest()[0:16], AES.MODE_ECB) # iv = ESSIV: encrypt sect_num with AES(key=hash_of_main_key)
outimage = open(args.output_imgfile, "wb")
with open(args.input_imgfile, "rb") as f:
    while True:
        chunk = f.read(BLOCK_SIZE)
        if chunk:
            # encrypt the sector number to get iv (the ESSIV way) (careful with struct.pack: endianness matters!)
            iv = aes_iv.encrypt(struct.pack('l', sect_num).ljust(16, '\x00'))
            aes_block = AES.new(aes128_key, AES.MODE_CBC, iv)
            if args.decrypt:
                outimage.write(aes_block.decrypt(chunk))
            else:
                outimage.write(aes_block.encrypt(chunk))
            sect_num += 1;
            sys.stdout.write('.')
        else:
            print "Finished."
            break
    f.close()

outimage.close()

if not args.no_eeprom:
    print("Saving the data to eeprom C source file: "+args.eeprom_file)
    with open(args.eeprom_file, "w") as f:
        f.write('uint8_t EEMEM aes_key_encrypted[] = {')
        for c in aes128_key_encr:
            f.write(str(ord(c)))
            f.write(',')
        f.write('}; // ')
        f.write(binascii.hexlify(aes128_key_encr));
        f.write('\n');
        f.write('uint8_t EEMEM passphrase_hash_hash[] = {')
        for c in pp_hash_hash.digest():
            f.write(str(ord(c)))
            f.write(',')
        f.write('}; // ')
        f.write(pp_hash_hash.hexdigest());
        f.write('\n');
        f.close()

