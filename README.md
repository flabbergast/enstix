# enstix (storage/serial/keyboard AVR stick)

This is a firmware for Stephan Bärwolf's
[AVRstick], an USB stick with
ATMEL's atxmega128a3u, which turns it into an USB stick pretending
to be a combined Mass Storage, CDC Serial and HID/Keyboard interface.

The whole shebang is about making the AVR stick into an encrypted USB
storage (64kB in size).

The contents of the disk are stored in the AVR stick's flash, encrypted
with AES128. "Unlocking" the drive is done by connecting to the stick
via CDC Serial interface and entering the passphrase (see the Usage
instructions for more details).

To use the Serial on Windows machines, an `.inf` file is provided (not
tested). Note that it doesn't install any driver, it just lets Windows
know that it's a standard CDC Serial interface. (Like for Arduino.)

## Warnings

Using atxmega128a3u's flash as a writable USB drive is a **bad idea**,
since the flash on ATMEL's chips is *not meant to be used like this* and
has a very limited lifespan compared to usual solid-state storage (on
the order of 1000 writes?).

This is more like a *demo firmware*, which can be quite useful for
storing passwords and similar data which is small (it needs to fit into
64kB!) and don't change often. This is also why extra steps are needed
to make the storage writable.

## VID/PID

Every USB device has a Vendor/Product identifying signature. This is set
in software here (`Descriptors.c`). The current code uses a pair which
belongs to the [LUFA] library. DO NOT USE IT for any other than
development purposes only! See [LUFA's VID/PID
page](http://www.fourwalledcubicle.com/files/LUFA/Doc/120730/html/_page__v_i_d_p_i_d.html).

## Credits

- The whole USB interface utilizes Dean Camera's excellent [LUFA]
  library, and builds on several LUFA demos supplied with the library.
- Writing to atxmega128a3u's flash is made possible by Stephan Bärwolf's
  custom bootloader and his `apipage` example/library (see the
  [AVRstick]'s webpage).
- Software AES implementation for AVR comes from
  [AVR-crypto-lib].

## Compiling / installation

### Prerequisites

I'm assuming you have Stephan's [AVRstick] {you can buy it on
[Tindie](https://www.tindie.com/products/matrixstorm/avr-stick-prototype/)},
and a PC (preferably running linux or Mac OS X) with a working
[avr-gcc](http://www.nongnu.org/avr-libc/) installation,
[git](http://git-scm.com/) and [python](https://www.python.org/) 2.7
(with the [pycrypto](https://www.dlitz.net/software/pycrypto/) package
installed). {Haven't tried python 3 yet.} All instructions are to be
executed in a terminal window.

1. Get the sources:

        git clone https://github.com/flabbergast/enstix.git
        cd enstix

2. Get the [LUFA] library: Download sources from its website (I used the
   140928 release), unpack and copy the `LUFA` folder (it's inside
   the main unpacked dir) into the `enstix` folder.

3. Create a "random" 64kB disk image (that will serve as the encrypted
   disk): `image.bin`. NB1: The first 5 sectors (2560 bytes) are the
   same on every image. NB2: all the python scripts in the `enstix/scripts`
   directory accepts `-h` or `--help` as parameters, which
   will then give you a list of options and a short explanation.

        ./scripts/create-random-64k-image.py

4. Encrypt `image.bin`. This step requires you to enter a passphrase
   (that will be used throughout), by default creates a new random
   AES128 key and encrypts the disk image with it. The resulting files
   are `image.bin.out` and `eeprom_contents.c` (where the encrypted
   AES key and passphrase hash^2 are recorded).

        ./scripts/crypto.py

5. Compile the firmware (this requires the `eeprom_contents.c` source
   file, that's why we first created the disk image).

        make

6. Attach the disk image to the compiled firmware.

        ./scripts/attach-img-to-bin.py

7. Convert the generated `.eep` file with EEPROM contents into the
   binary format used by Stephan's bootloader.

        avr-objcopy -I ihex -O binary enstix.eep EEPROM.BIN

8. Upload the firmware and EEPROM contents onto the AVR stick: Insert
   the AVR stick with the PROG button pressed (so that it enters the
   bootloader). It will emulate an USB drive. Mount it somewhere (might
   be done automatically by your system).

        cp FIRMWARE.BIN /wherever/the/stick/is/mounted/FIRMWARE.BIN
        cp EEPROM.BIN /wherever/the/stick/is/mounted/EEPROM.BIN

9. Reset or reinsert the AVR stick and enjoy.

Note: if you just want to update the firmware, and are happy with the
current password and the disk image currently burned on the xmega's
FLASH, it's enough to run `make` to compile the firmware, and copy the
resulting `enstix.bin` to `FIRMWARE.BIN` file onto the AVR stick in
bootloader mode. Note that it doesn't matter what passwords
`eeprom_contents.c` file contains, as long as you don't update the
EEPROM contents on the AVR stick (i.e. you don't write to `EEPROM.BIN`
file on the AVR stick in bootloader mode).

## Usage

When the AVR stick is first inserted, it presents itself as a USB
composite device, Mass Storage combined with CDC Serial and Keyboard.

Keyboard is there, but its functionality is not used at the moment.

Mass Storage appears as a 64kB SCSI USB drive, with one read-only
FAT12-formatted partition (ENSTIX), with one README.TXT file on it.
Nothing too interesting. (You can tweak the contents of the README file
in the sources, `Config/AppConfig.h`.)

To switch to "encrypted mode", connect to the Serial interface using
one of serial terminal programs, e.g.
[minicom](https://alioth.debian.org/projects/minicom),
[picocom](https://code.google.com/p/picocom/),
[puTTY](http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html) or
even [screen](http://www.gnu.org/software/screen/). The baud rate is not
important, any should work. For example, on linux:

        picocom -b 115200 /dev/ttyUSB0

or

        screen /dev/ttyACM0 115200

You should of course use the apropriate `ttyXXXX`, you can find it by
looking at the output of `dmesg`.

OK, so now you're talking to AVR stick via a serial terminal. Any key
other that `i`, `p`, `r`, `c` will display a short help. The individual
keys do the following:

- `i` will print some info.
- `p` will ask you for your passphrase. If entered correctly, the stick
  switches to the "encrypted mode".
- with `r` you can switch from "read-only" to "writable" and back. This
  only works in the "encrypted mode".
- with `c` you can change your passphrase.

Note that any change to the disk state (initial -> encrypted mode, or RO
to RW or back) will cause the whole stick to disconnect from USB and
then connect back after 1 second delay.

In the "encrypted mode", the Mass Storage works as a common SCSI disk
drive, 64kB in size. The only thing that the stick is doing is that
whenever the computer wants to read a sector from that "drive", the
stick will take the appopriate portion of the flash, decrypt it and
serve the result to the computer. Likewise for writing (receive the
sector contents, encrypt and save to flash). So you can partition and
format the 64kB to your liking. All the partition/filesystem business is
now done on the computer side. If you followed the set-up instructions
above, then the default "contents" of the drive will be a drive with one
FAT12-formatted partition (ENSTIXEN).

Once more, the xmega's flash has a *very* limited lifespan, so I
recommend enabling the RW mode only when absolutely necessary (so when
you actually need to write some data to it).

## Encryption details

The encrypted disk image is encrypted with aes128-cbc-essiv (probably
not directly compatible with other existing programs using this scheme).
So there is one main AES128 key. This one is used to encrypt each sector
(512 bytes) with CBC, where the IV is derived from the key and the
sector number as described by the ESSIV scheme.

What is stored in xmega's EEPROM is the main AES128 key encrypted with
AES (the key for this is the first 16 bytes of the SHA256 hash of the
passphrase). Another piece of data stored in EEPROM is the SHA256 hash
of the SHA256 hash of the passphrase (for verifying if the entered
passphrase is "correct").

Of course, the (encrypted) disk drive image can be easily extracted from
the stick by putting it into the bootloader mode and inspecting the
contents of the `FIRMWARE.BIN` file. Likewise, the passphrase-encrypted
AES key, as well as HASH^2(passphrase) can be read from `EEPROM.BIN`
file.

Finally, while the stick is in operation and in the "encrypted mode",
the main AES key is stored in chip's SRAM (working memory), as well as
probably in the "AES hardware module" of the chip. So it can be probably
extracted with, say, a JTAG debugger hooked up to the chip. This is the
same problem as with pretty much all the "encrypted storage" schemes
that I know - while the encryption is in operation, the key can be
extracted from the device's working memory. However, as the passphrase
is only needed for getting a hold of the main AES key, it is deleted
from the working memory after the main AES key is decrypted.

I would be grateful if you can let me know if there are some security
holes in this scheme.

## Upgrading firmware / changelog

### Upgrading from older than 1.2

The upgrade to 1.2 breaks previously used disk images, as it changes the
way the main key is encrypted (and stored in EEPROM). No special upgrade
instructions are provided (ask me if you need them).

## Remarks

In principle this firmware should compile for other USB breakouts
with ATMEL's USB capable chips (e.g. atmega32u4), but some
hacking/gutting the current code would be required.  The limitations
that come into play are:

- The number of USB endpoint that the chip can handle. This completely
  rules out atmega32u2, since that one can only do 4 endpoints and
  MassStorage+Serial requires 2+3 = 5 endpoints.
- The current model that utilizes the chip's FLASH for storage requires
  that the firmware needs to be able to write to FLASH. This pretty much
  means Stephan's bootloader+apipage example on his [AVRstick].
  Of course you can change the storage backend to use an external
  microSD or dataflash or something like that (this is on the roadmap).

## Hacking / tweaking the code

Some constants (e.g. `README.TXT` contents) can be tweaked in
`Config/AppConfig.h` file.

For more serious hacking, you should probably start with `enstix.c`:
this contains the main program logic. I tried to isolate the USB support
code into `LufaLayer.c` (plus the other files), see `LufaLayer.h` for
the list of functions that it provides.

## Roadmap / TODO

- Figure out and implement another way of entering the passphrase that
  wouldn't require Serial access.
- Use the AVRstick's button and the Keyboard interface to "one-button
  typing password", a-la [DIY USB password
  generator](http://codeandlife.com/2012/03/03/diy-usb-password-generator/)
- Use external microSD or dataflash storage instead of the poor little
  xmega's flash.
- Design and make a hardware "shield" with a microSD socket to be
  soldered to the AVRstick, turning it into a "hardware encrypted
  storage" stick.

## License

My code is (c) flabbergast. GPL v3 license (see LICENSE file). Portions
of the code come from LUFA demos, this is licensed by LUFA's license.

See also the libraries' [LUFA
license (MIT)](http://www.fourwalledcubicle.com/files/LUFA/Doc/120730/html/_page__license_info.html) and
[AVR-crypto-lib license
(GPLv2)](https://git.cryptolib.org/?p=avr-crypto-lib.git;a=blob;f=LICENSE;h=92851102051bbf74b2794f4b8a9b7e04374932ee;hb=HEAD)


[AVRstick]: http://matrixstorm.com/avr/avrstick/
[LUFA]: http://www.fourwalledcubicle.com/LUFA.php
[AVR-crypto-lib]: https://git.cryptolib.org/avr-crypto-lib.git
