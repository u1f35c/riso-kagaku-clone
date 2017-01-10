I wrote this for my friend [Jon](https://jmtd.net/), who was after a simple notification device for his NAS but had difficulty finding anything appropriately cheap. Some discussion on IRC led to discovering the Linux kernel has support for a variety of USB HID LED devices. In particular the Riso Kagaku Webmail Notifier presents a simple 8 colour RGB device (red/green/blue can each be on or off). This seemed like the sort of thing that could easily be implemented on an Atmel ATtiny, so going from various pieces of information gleaned from Google and the Linux driver I came up with this code. It's only been tested under Linux; I'd be curious to know if it works with the Windows drivers too.

It's based on [V-USB](https://www.obdev.at/products/vusb/). I've reused the Riso Kagaku USB IDs so the Linux driver picks it up without needing any patching (assuming CONFIG_HID_LED is set, or CONFIG_USB_LED for older kernels). This is probably wrong of me.

`apt install avr-libc avrdude` should install the appropriate build requirements on Debian (`gcc-avr` will be automatically pulled in) assuming you already have `build-essential` installed for `make`.

`make` will then build you a main.hex which you can program to your device using
`avrdude`. With my [Bus Pirate](http://dangerousprototypes.com/docs/Bus_Pirate) I use:

    avrdude -p attiny85 -c buspirate -P /dev/ttyUSB0 -U flash:w:main.hex:i

Or if you're using [Micronucleus](https://github.com/micronucleus/micronucleus):

    micronucleus main.hex

I originally tested the code out on a tiny45 board I had lying around, but subsequently have reconfigured for a [Digispark](http://digistump.com/products/1) clone, which is based on a tiny85 and comes with Micronucleus pre-installed, removing the need for a separate programmer. USB D- is assumed to be connected to Port B pin 3, USB D+ to Port B pin 4, green is on Port B pin 0, red is on Port B pin 1, and blue on Port B pin 2. The USB definitions can be configured in usbconfig.h, the LED pins are defined at the top of main.c.

Given that V-USB is GPLv2+ or commercial all of my code is released as GPLv3+, available at [https://the.earth.li/gitweb/?p=riso-kagaku-clone.git;a=summary](https://the.earth.li/gitweb/?p=riso-kagaku-clone.git;a=summary) or on GitHub for easy whatever at [https://github.com/u1f35c/riso-kagaku-clone](https://github.com/u1f35c/riso-kagaku-clone)
