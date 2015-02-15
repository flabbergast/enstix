#!/usr/bin/env python

HASH_ITERATIONS=1000

from Crypto.Cipher import AES
from Crypto.Hash import SHA256
from Crypto import Random

from intelhex import IntelHex

import getpass
import binascii
import struct
import sys

import argparse

parser = argparse.ArgumentParser(description="Generate a random AES key and write eeprom hex file (for usage with enstix).", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-k', '--key', dest='key', help='Encrypted main AES key, 16 hexified bytes (32 chars). If no supplied, a new random one will be generated.')
parser.add_argument('-N', '--no-eeprom', dest='no_eeprom', action='store_true', help="Do not update the eeprom hex file with the generated password.")
parser.add_argument('-e', '--eeprom-file', dest='eeprom_file', nargs='?', default='enstix.eep', help="Eeprom file in intel hex format (gets overwritten!).")

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

def checksum(num):
    return binascii.hexlify(chr( ((~(num&0xFF))&0xFF) +1)).upper()

if not args.no_eeprom:
    print("Saving the data to eeprom hex file: "+args.eeprom_file)
    with open(args.eeprom_file, "w") as f:
        f.write(':10000000')
        f.write(pp_hash_hash.hexdigest().upper()[0:32])
        f.write( checksum(sum(bytearray(pp_hash_hash.digest()[0:16]))+16) )
        f.write('\n:10001000')
        f.write(pp_hash_hash.hexdigest().upper()[32:64])
        f.write( checksum(sum(bytearray(pp_hash_hash.digest()[16:32]))+32) )
        f.write('\n:10002000')
        f.write(binascii.hexlify(aes128_key_encr).upper())
        f.write( checksum(sum(bytearray(aes128_key_encr))+48) )
        f.write('\n:00000001FF\n')
        f.close()

