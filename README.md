# enstix (storage/serial/keyboard AVR stick)

This is a firmware for Stephan Bärwolf's
[AVRstick], an USB stick with
ATMEL's atxmega128a3u, which turns it into an USB stick pretending
to be a combined Mass Storage, CDC Serial and HID/Keyboard interface.

The current functionality is as follows:

 - When insterted into a computer, the Mass Storage identifies as a
   read-only FAT12-formatted USB drive (with one README.TXT file on it).
 - Connecting to the Serial interface with a terminal (e.g. putty,
   picocom, minicom or screen) allows for entering a password. With the
   correct password, it disconnects from USB and reconnects as another
   FAT12-formatted 64kB USB drive, again read-only. This one can be made
   writable by again connecting via Serial and issuing the proper
   command.
 - The Keyboard is not doing anything at the moment.

The point is that the image of the second (potentially writable) USB
drive is in fact stored encrypted directly in the flash of the
atxmega128a3, so the whole setup acts as an encrypted USB drive.

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

I'm assuming you have Stephan's AVRstick, and a PC (preferably running
linux or Mac OS X) with a working avr-gcc installation, git and python.
All instructions are to be executed in a terminal window.

1. Get the sources:

        git clone https://github.com/flabbergast/enstix.git
        cd enstix

2. Get the [LUFA] library: Download sources from its website (I used the
   140928 release), unpack and copy the `LUFA` folder (it's inside
   the main unpacked dir) into the `enstix` folder.

3. Create a "random" 64kB disk image (that will serve as the encrypted
   disk): `image.bin`. NB: all the python scripts in the
   `enstix/scripts` directory accepts `-h` or `--help` as parameters, which
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

## Encryption details

The encrypted disk image is encrypted with aes128-cbc-essiv (probably
not directly compatible with other existing programs using this scheme).
What is stored in xmega's EEPROM is the main AES128 key encrypted with
AES256 (the key for this is the SHA256 hash of the passphrase).
Another piece of data stored in EEPROM is the SHA256 hash of the SHA256
hash of the passphrase (for verifying if the entered passphrase is
"correct").

I would be grateful if you can let me know if there are some security
holes in this scheme.

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

- Implement passphrase changing from within the serial interface.
- Use xmega's hardware AES128 instead of a software implementation.
- Figure out and implement another way of entering the passphrase that
  wouldn't require Serial access.
- Use the AVRstick's button and the Keyboard interface to "one-button
  typing password", a-la [DIY USB password
  generator](http://codeandlife.com/2012/03/03/diy-usb-password-generator/)
- Use external microSD or dataflash storage instead of the poor xmega's
  flash.
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
