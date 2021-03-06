# AmigaPS2Mouse

Very much work-in-progress PS/2 Amiga Mouse driver.

Uses an Arduino to translate PS/2 mouse protocol to the Amiga mouse port.
The middle mouse button us used to communicate extra information - the scroll wheel and the status of the middle button.


NOTES
=====

The Middle mouse button is emulated, it does not use the real middle mouse button pin.
This is because this pin is used for communication between the Amiga and Arduino for 
scroll wheel etc.


THANKS
======

Kris Adcock for his PS/2-to-Quadrature-mouse project.
http://danceswithferrets.org/geekblog/?m=201702
PS2_to_Amiga.ino is based heavily on his PS2_to_Quadrature_Mouse.ino
I used an interrupt based PS/2 library instead if Kris's.

PS2Utils Arduino library by Roger Tawa (Apache License 2.0)
https://github.com/rogerta/PS2Utils
Designed for a PS/2 keyboard but works perfectly with a mouse.  Interrupt driven.

digitalWriteFast by Nickson Yap

Members of English Amiga Board for help
http://eab.abime.net/showthread.php?p=1178490



ERROR CODES
===========
Errors will be shown by flashing the onboard LED:

* 2 = Error resetting PS/2 mouse
* 3 = Error changing sample rate (non-intellimouse?)
* 4 = Error reading ID (non-intellimouse?)
* 5 = Unable to begin PS2 protocol
* 6 = Unable to enable Data Reporting



OUTSTANDING ISSUES
==================
Scroll wheel does not work if the left or right mouse button is held down.
This is not a problem for the right mouse but would be nice for the left.
Do other mouse adapters support this?


