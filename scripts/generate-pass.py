#!/usr/bin/env python

from Crypto.Cipher import AES
from Crypto.Hash import SHA256
from Crypto import Random

import getpass
import binascii
import struct
import sys

import argparse

parser = argparse.ArgumentParser(description="Generate a random AES key and write eeprom C source file (for usage with enstix).", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-k', '--key', dest='key', help='Encrypted main AES key, 16 hexified bytes (32 chars). If no supplied, a new random one will be generated.')
parser.add_argument('-N', '--no-eeprom', dest='no_eeprom', action='store_true', help="Do not update the eeprom C source file with the generated password.")
parser.add_argument('-e', '--eeprom-file', dest='eeprom_file', nargs='?', default='eeprom_contents.c', help="C source file for eeprom variables (gets overwritten!).")

args = parser.parse_args()

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
print("SHA256 hash of the passphrase: "+pp_hash.hexdigest())
pp_hash_hash = SHA256.new(pp_hash.digest())
print("SHA256 hash of the hash of the passphrase (saved to EEPROM, used to check if the passphrase is correct: "+pp_hash_hash.hexdigest())

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

