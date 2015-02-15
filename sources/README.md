# enstix sources

## Compiling / installation

### Compiling / flashing

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

3. **(only for flash-backed storage)** Create a "random" 64kB disk image
   (that will serve as the encrypted disk): `image.bin`.

   NB1: The first 5 sectors (2560 bytes) are the
   same on every image.

   NB2: all the python scripts in the `enstix/scripts`
   directory accept `-h` or `--help` as parameters, which
   will then give you a list of options and a short explanation.

        ./scripts/create-random-64k-image.py

4. **(only for flash-backed storage)** Encrypt `image.bin`. This step
   requires you to enter a passphrase (that will be used throughout), by
   default creates a new random
   AES128 key and encrypts the disk image with it. The resulting files
   are `image.bin.out` and `eeprom_contents.c` (where the encrypted
   AES key and passphrase hash^2 are recorded).

        ./scripts/crypto.py

5. **(only for microSD-backed storage)** Create a random AES key, encrypt
   it with a passphrase and write the data to (and/or create) the
   `eeprom_contents.c` source file.

        ./scripts/generate-pass.py

6. Edit the firmware configuration. Have a look at (the beggining of)
   `Config/AppConfig.h` file. The main thing is to choose whether the
   storage should be on a microSD (default) or in the flash.
   Another spot to have a look is at `sd_raw/sd_raw_config.h`, where the
   hardware connection to microSD (SPI) is set up. Finally, some bits
   (LEDs, Buttons) are set up in `Board/*`. Note that if LUFA supports
   your board directly, you can just edit `makefile` and supply
   the appropriate value for the `BOARD` variable (e.g. `OLIMEX32U4`).

   The defaults are for [AVRstick], with microSD and LED set up as on
   [this](http://174763.calepin.co/uSD-shield-1.html) shield.

7. Compile the firmware (this requires the `eeprom_contents.c` source
   file, that's why we first created the this file (possibly along with
   a disk image)).

        make

8. **(only for flash-backed storage)** Attach the disk image to the
   compiled firmware.

        ./scripts/attach-img-to-bin.py

9. Convert the generated `.eep` file with EEPROM contents into the
   binary format used by Stephan's bootloader.

        avr-objcopy -I ihex -O binary enstix.eep EEPROM.BIN

10. Upload the firmware and EEPROM contents onto the AVR stick: Insert
   the AVR stick with the PROG button pressed (so that it enters the
   bootloader). It will emulate an USB drive. Mount it somewhere (might
   be done automatically by your system).

        cp FIRMWARE.BIN /wherever/the/stick/is/mounted/FIRMWARE.BIN
        cp EEPROM.BIN /wherever/the/stick/is/mounted/EEPROM.BIN

11. Reset or reinsert the AVR stick and enjoy.

Note: if you just want to update the firmware, and are happy with the
current password and the disk image currently burned on the xmega's
FLASH, it's enough to run `make` to compile the firmware, and copy the
resulting `enstix.bin` to `FIRMWARE.BIN` file onto the AVR stick in
bootloader mode. Note that it doesn't matter what passwords
`eeprom_contents.c` file contains, as long as you don't update the
EEPROM contents on the AVR stick (i.e. you don't write to `EEPROM.BIN`
file on the AVR stick in bootloader mode).

## Hacking / tweaking the code

Some constants (e.g. `README.TXT` contents) can be tweaked in
`Config/AppConfig.h` file.

For more serious hacking, you should probably start with `enstix.c`:
this contains the main program logic. I tried to isolate the USB support
code into `LufaLayer.c` (plus the other files), see `LufaLayer.h` for
the list of functions that it provides.

## License

My code is (c) flabbergast. GPL v3 license (see LICENSE file). Portions
of the code come from LUFA demos, this is licensed by LUFA's license.



[avrstick]: http://matrixstorm.com/avr/avrstick/
[lufa]: http://www.fourwalledcubicle.com/lufa.php
[avr-crypto-lib]: https://git.cryptolib.org/avr-crypto-lib.git
[arduino leonardo]: http://arduino.cc/en/main/arduinoboardleonardo
[teensy]: https://www.pjrc.com/store/teensy.html
[x-a4u-stick]: http://174763.calepin.co/x-a4u-stick-2.html
