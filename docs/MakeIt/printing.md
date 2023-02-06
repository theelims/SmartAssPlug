# Print Your Plug

## Recommended Filaments

For the body of the SmartAss Plug ASA and ABS are recommended as filament. Purchase a quality brand as the print quality can vary greatly between brands. If possible buy a blend where the raw materials that go into the filament are FDA approved for food contact. If you have never printed either ASA or ABS go with ASA. Not only does it emit considerable less nasty fumes, but also does it generally posses more favorable print properties. As the parts are rather small ASA can be printed on open frame printers like an Ender or Prusa without too much worries.

Be sure to test the vapour smoothing properties of your specific ABS / ASA blend. Some blends polish barely due to additives.

!!! warning "It is strongly recommended to use ASA or ABS"

    Inserting 3D printed parts into the body is not without dangers. The mechanical properties and the force to break the plugs neck haven been optimized with ASA. It takes considerable force - unlikely to encounter in a real world application - to break the plug in half. Also the fact that 3D printed parts are typically not water tight and have a rough surface makes them less then ideal for inserting them into the body. Both issues can be somewhat mitigated by the ability to vapour smooth ASA and ABS prints. Also both materials can be purchased in FDA approved food safe qualities.

For the cap a transparent filament is needed. Generally speaking this boils down to PET, PETG or PCTG as the filament choice. The cap is used to diffuse two RGB leds.

## Print Settings

### Plug Body

These print settings are recommended for optimal results:

- Nozzle: 0.4 mm
- Perimeters: 4
- Layer height: 0.1 mm
- Top / Bottom Solid Layers: 10
- Infill: 40% Rectilinear
- Smooth build surface

All parts print without supports enabled. Except for _SmartAssPlug_S_Plug.stl_ and _SmartAssPlug_M_Plug.stl_ which need support inside the electronics compartment. For these enable supports from the build plate only.

!!! tip "Suffers from poor surface finish where the main body slopes towards the neck?"

    Due to the thin layer height of the top solid layers they can sink in on the shallow sloped top surface. If you run into this issue, then use a height modifier in the slicer for these few millimeters and modify the infill to 100%.

If you experience issues with the print not sticking to your build surface

### Transparent Cap

Since this part must diffuse the RGB leds it must be printed with optical properties in mind. Best results I got with these settings:

- Nozzle: 0.4 mm
- Perimeters: 30
- Layer height: 0.1 mm
- Top / Bottom Solid Layers: 1
- Infill: 100% Rectilinear

The heigh count of perimeters lines makes the slicer print the part in concentric lines and had prettier results then concentric infill. Basically we want a solid part.
