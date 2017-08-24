# Firmware replacement for Chinese STC based processor DIY Clock kits

[![Join the chat at https://gitter.im/stc-clock/Lobby](https://badges.gitter.im/stc-clock/Lobby.svg)](https://gitter.im/stc-clock/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

This is a replacement program for the STC 8051 core uP based DIY digital clock kits available from numerous Chinese sources. The specific clock used to develop this firmware was purchased from [Banggood](http://www.banggood.com/DIY-4-Digit-LED-Electronic-Clock-Kit-Temperature-Light-Control-Version-p-972289.html) but was constructed with a code base that should be (easily?) modified with future forks to support most of the four digit clocks that are based on the STC controllers.

03-April-17 This firmware now supports the "talking clock" sold by Banggood and others that uses the NY3P065 sound chip in both 24 and 12 hour modes. Also supported are the 0.8 inch single LED clocks with both LDR dimming and temperature measurement. Any port/pin combination can be mapped in the header files as well as a single board design supporting both common anode and common cathode displays. I will be adding photographs and detailed modification documents as time and interest permit.

![Image of Banggood id 972289](http://img.banggood.com/thumb/large/2014/xiemeijuan/03/SKU203096/A7.jpg)

## Getting Started
You'll need [SDCC](http://sdcc.sf.net) to build and [STC-ISP](http://www.stcmcudata.com/STCISP/stc-isp-15xx-v6.86D.zip) or [STCGAL](https://github.com/grigorig/stcgal) to set the clock speed, processor hardware options and to flash the firmware.

In addition to the above tools, you will need one serial port with TTL outputs for programming the STC processor. The simplest solution is usually an inexpensive USB to serial TTL converter. These can be based on the FT232 chipset, PL2303 or CH340. They all work equally well, it's just a matter of what you find at the ready when you need it.

### Clock Features
* Time display (12 and 24 hour modes).
* Alarm (using the rather loud internal buzzer).
* Programmable hourly chime. On/Off selectable.
* Date and Day display. On/Off selectable.
* Ambient Temperature display. F/C selectable. On/Off selectable.
* Display Auto-Dim with programmable limits. The auto-dim can be disabled using the limits.
* Temperature calibration
* Auto-increment when setting times/alarms/etc. No need to repeatedly press the same key.

The features above are implemented with a multi-level menu with best-as-can-be-managed text prompts using the limited 7 segment displays. This menu helps (IMO) with the "where-the-hell-am-I" in the rather tortured, linear menu of the original software. This does come with a cost, namely flash space so please note that the initial commit of this code requires 4.25k of memory. This should not be an issue for the Banggood clock mentioned since it arrived with an 8k chip, the STC15W408AS. If you want to use this in a similar model that has only 4K of available flash, some features will need to be removed. Since this is a work in progress, a request with at least one feature to delete should get you a branch that will need no more than 4K of flash.

I have found that consolidating the units selection (hour format, date format and temperature) into one group as an US/EU selection rather than three individual pieces works well to get code size under 4k. In hindsight, this might be a better method than keeping them as individual selections going forward. Another option is to remove the display of the day of the week feature which I consider rather useless option.

## Things that could be implemented
* Use of the relay outline on the PC board.
* Year display. Really?
* Alarm snooze.
* ?

## To-do list
* Single page menu flow diagram
* Documentation (code modification guide)

```
Menu simple navigation
S1 (Top switch) is menu cycle
S2 (Bottom) is the select or change

Example: To change Alarm time. Bracketed text indicate display text.
S1 [Set] S2 [CL] S1 [AL] S2 [08:00]
The hours group is now flashing. Press S2 to change, S1 to change to minutes then S2 to change minutes.
When complete, press S1. Menu advances to [CH], press S1 three times, display blanks and returns to clock.
```

### Programming with STC-ISP
Connect to the STC processor using the four pin header which hopefully you installed prior to soldering the four LED display in place. If your clock is already built and your desoldering skills and/or tools are not up to the task, you can solder the necessary header(s) using wire between the header(s) and the top of the board.

 ----------------------------
| P1 header | Serial adapter |
|-----------|----------------|
| 5V        | 5V             |
| GND       | GND            |
| P3.0      | TXD            |
| P3.1      | RXD            |
 ----------------------------

Once you're connected, you have your choice of either STC-ISP, which is a Windows application (runs fine on most VM's) or your can use [STCGAL](https://github.com/grigorig/stcgal) if you're comfy with Python. Once your tool is running, you'll need to load the .HEX output file into the processor. The STC-ISP tool is pretty straight forward, the process is:

* Select the correct processor type
* Select com port
* Open code file (main.hex)
* Select clock speed of 22.1184 MHz on the Hardware Option tab
* Click the Download/Program button
* Interrupt the +5 supply to the STC processor and restore

The last step above can be as easy as removing the connector to P3 and restoring. Or, if you're going to do additional development, a NC momentary push button in the +5 line and a diode and resistor in the RX/TX lines to prevent the serial adaptor signals from keeping the processor alive are required. The schematic for these connections is detailed in the STC technical document. Links below.

### Making changes
It is a simple matter to rebuild with the provided Makefile for SDCC. The Makefile originated with zerog2k's STC DIY-Clock project and I extended it with the additional file structure I created. In doing so, I found that there were several file inter dependencies that required a fair number of "make clean" followed by "make" commands so I added the ".phony" rule to just recompile everything per session since the compile and link times were insignificant. Better to wait three seconds for a complete rebuild than to waste twenty minutes on trying to figure out why the changes didn't appear in the code.

If you want to make any significant code changes, you'll probably wish you had some debugging capability. If you have the STC15W408AS part in your clock, you're in luck as this processor has a second timer and UART. The code already has initialization in place for this UART  if turned on in the global.h header file. SDCC supports a small footprint printf (printf_tiny) that only requires about 400 additional bytes of flash. The timer 2 default configuration is for 115200 baud, 8/1/N. On the Banggood board, the RX and TX have separate pins on a two pin connector, P3. Connection to a serial device is:

 ----------------------------
| P3 header | Serial adapter |
|-----------|----------------|
| P3.6      | TXD            |
| P3.7      | RXD            |
 ----------------------------

In order to pick up the GND connection, you'll need to parallel the main serial port on P1. The connections for P1 are detailed in the previous section.

## Program Assumptions
The cpu clock is set to run at 22.1184MHz. Originally, things started out at 11.0592MHz but after investigating the timing of other clocks and displays, it was discovered that the white and blue LED's appear to be very, very bright. After some experimentation with timing and dimming schemes, I decided to go with a 50us clock tick and in order to not waste all the cpu time servicing the timer ISR, I doubled the clock rate. The STC parts can go to 35MHz and while I tested them at these speeds, the USB serial upload was not 100% reliable, so I compromised at 22.1184MHz. Interestingly, the current consumption of these parts is very low and the total current at 22MHz was only a few milliamps, made even more insignificant by the LED current. At this nice, fast 50us clock tick, I set the overall maximum tick count to 64 so dividing that among the four digits allows for a 0.39% digit minimum on time. It was quite surprising the overall brightness at these incredible small on times but this was necessary to get them down to a good dimming level for night time viewing.

Why 22.1184MHz and not 16 or 20MHz? Simple, standard baud rates. If you plan on using a serial port for debugging, keep the clock frequency set to a value that can be divided evenly down to standard bit rates. 22.1184 / 115200 = 192 which is the baud rate used for the serial debugging port as released. 18.432MHz is the other common value for this use.

## Code notes and structures
The C code is formatted with spaces only, no tab characters used (except in the Makefile). Tabs are set to 4 so stops occur at columns 5,9,13,17... Indent and brace style is K&R with functions receiving the opening brace on a new line rather than in-line with the argument parens.

## Authors
* **R Sloyer** - *Initial work* - [aFewBits](https://github.com/aFewBits)

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Disclaimer
This code is provided as-is, with NO guarantees or liabilities as per the License. Use at your own risk.

Since the original firmware loaded on the STC processor cannot be copied, there is no duplicating what you originally received with your clock. If you want to retain this code for any reason, you must purchase another STC chip of the same type (or same base type with more flash if desired or needed). These are readily available on Aliexpress in small quantities as well as eBay although the prices are usually somewhat higher. Just be prepared to wait for them to arrive, especially from Aliexpress.

## Acknowledgments
* [zerog2k](https://github.com/zerog2k) for his original STC DIY Clock work
* STC and zerog2k for the ADC code used in the LDR and temperature measurement code
* Jack Ganssle et al at The Embedded Muse for the [initial switch debouncing idea](http://www.ganssle.com/debouncing.htm)
* Maxim Integrated for their DS1302 app note and [8051 example code](https://www.maximintegrated.com/en/app-notes/index.mvp/id/3449)
* All the contributors to the SDCC tool set

### References
Main STC website(in Chinese - Google translate is your friend!), STCMCU.COM redirects here:
http://www.gxwmcu.com

STC15 series English datasheet:
http://www.stcmcu.com/datasheet/stc/STC-AD-PDF/STC15-English.pdf

Older STC15F204EA English datasheet:
http://gxwmcu.com/datasheet/stc/STC-AD-PDF/STC15F204EA-series-english.pdf

Other STC documents (English)
http://stcmicro.com/sjsc.html

SDCC User Guide (PDF):
http://sdcc.sourceforge.net/doc/sdccman.pdf

Maxim DS1302 datasheet:
http://datasheets.maximintegrated.com/en/ds/DS1302.pdf

Maxim DS1302 application note 3449 with example code for 8051 interfacing:
https://www.maximintegrated.com/en/app-notes/index.mvp/id/3449

[![Join the chat at https://gitter.im/stc-clock/Lobby](https://badges.gitter.im/stc-clock/Lobby.svg)](https://gitter.im/stc-clock/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
