# OMNIXIE
Nixie driver board to enable easy inclusion of a Nixie tube into projects. One common driver board to support a wide range of Nixie INdicator Tubes. I love modularity!


Initial sketches, ideas, original project intent was just a nixie clock. But I hated the feeling of desigingn a nixie clock with the nixie driver circuitry as single use for just a clock, when really you should be able to drive a nixie tube anywehere you'd like for displaying numbers. So that's where this inspiration came from

ALSO cnlohr's cnixxie. Particularly his MCU of choice ch32v003 and the accompanying library ch32v003fun and rv003usb inspired the digitally controlled flyback he uses in his design. Really in hopes of learning some skills he demonstrated in his youtube vide.

OMNIXIE is omni beause all of its OMNIXIE_HATs share a common hardware interface to the nixie driver board. Since nixie tubes wildly vary in their footprint, its nice to share a common hardware connection between them all thats what makes it omni. 

The real guts is the ddriver board.
Should be minimal in size such that it doesnt add any more difficult in creating some case for the nixie tube.
Should be relatively efficient like 90% efficiency on the flyback onverter
Should be programmable 
  There should be a software package that comes with the hardware that could easily spin up different functionality

The default program should allow the user of the board to immediatly get the devices to display a synchronized clock.
Via some qwiic connectors. Might have to be a custom i2c interface.


A flyback seemed to be the most suitable approach because of the smallest amount of parts needed for it.


WORK IN PROGRESS.
