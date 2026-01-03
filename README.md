# SouthPoleLevitator

Janky Jingle Crew is back this Christmas as well! We are continuing with the theme of magnetic propulsion from last year, but this time for levitation!

% FINAL IRL PICTURE ?

<img src="./Media/assembly_render.png" width="800px"/>


## User Guide

The card needs to be powered with a 5V power supply over USB-C, capable of at least 500 mA.

To levitate the tree:

1. Place the tree away from the driver board
2. Place the driver board on a flat surface, and plug in the USB-C cable. Note that the board needs to be level when it gets power for the correct calibration. If the tree is too close, this will also affect the calibration.
3. Hold the tree a few centimeters over and lower it slowly, trying to keep it centered and pointed straight up.
4. The rest is a balance between keeping the tree centered, and holding it with a loose enough grip. The tree needs to have the freedom to find the center itself, while also being prevented from sticking to either of the four base magnets.

## Working principle

### Driving LEDs wirelessly

The LEDs on the tree are powered by electro-magnetic noise from the switching of the coils on the driver board. There are four PCB coils in the base of the tree, each powering its quarter. The induced AC voltage in each of these coils is amplified with a resonant LC-tank tuned to the switching frequency. The resonance circuit consists of the inductance in the PCB coil and a separate MLCC C0G capacitor on the tree quarter. This amplified waveform is then rectified with a single schottky diode, finally driving the LEDs.

<img style="display: block; margin: auto;" src="./Media/resonant_circuit.png" width="700px"/>



## Design

### Tree
The tree consists of 5 PCBs, four of which constitute the tree itself, and one being the base plate with the pickup coils. A 3D-printed star keeps all of the tree-PCBs connected at the top! Tip: blow on one side of the tree when it is levitating to make it spin!

<img style="display: block; margin: auto;" src="./Media/spinning_tree.gif" width="300px"/>



## Manufacturing


### Driver board

<img style="display: block; margin: auto;" src="./Media/driver_exploded.png" width="400px"/>


<img style="display: block; margin: auto;" src="./Media/driver_animation.gif" width="400px"/>


### Tree

To begin with, the star is 3D-printed. You can either use the half-star variant (recommended) or the whole-star variant. For the half-star, print two of them laying flat and with support activated. The halves can then be glued together with superglue. Alternatively, the whole-star variant can be printed standing up depending on how well your printer handles overhang and/or surface finishes over supports.

<p align="center">
<img src="./Media/stars_3d_print.png" style=";" width="300px"/>
</p>

To keep the magnet centered, and the parts of the tree perpendicular to the base, a 3D-printed jig was used. This is not necessary, but drastically reduces the time to assemble multiple trees. If you are only making one, don't bother printing out the jig.

Both the magnet and the star can be glued with e.g. superglue or epoxy. Note that the magnet orientation matters!

The four "quarters" of the tree are soldered onto the base PCB with two solder joints each. These joints are the PCB coil A & B. We recommend gluing the star before soldering!

<img style="display: block; margin: auto;" src="./Media/tree_assembly.gif" width="700px"/>

## FAQ
