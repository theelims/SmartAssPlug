# Source all Parts

As you can't buy the plug off the shelf making it is up to you. This might be daunting at first, but if you already own a 3D printer you figured out the hard part already. But even if you don't own a printer yourself there are numerous service providers out happily accepting your money. Same goes for the electronics. You can practice SMD assembly and order the board from your preferred PCB house. Or you can throw money at the problem and get ready made and assembled boards from several sources in China.

## Tools needed

- 3D printer
- Soldering Iron + Solder (SmartAss Plug S & M only)
- Sandpaper, various grids starting from 120 and finer
- Scotch-Brite red and black
- Scissors
- Screwdriver Philipps #1
- Cleaned pickles jar, big enough for the intended plug size
- Acetone
- Paper Towel
- Acetone-proof brush
- IPA and lint-free cloth
- fine tipped tweezers

## Bill of Material Plug

| Component                | Size               | Qty | Source                                                                                        | Notes                                  |
| :----------------------- | :----------------- | :-- | :-------------------------------------------------------------------------------------------- | :------------------------------------- |
| Vibration Motor FF-030   | FF-030             | 1   | [AliExpress](https://www.aliexpress.com/item/1005003722404804.html)                           | Only for Plug Size S                   |
| Vibration Motor Mini 130 | 130                | 1   | [AliExpress](https://www.aliexpress.com/item/1005002947312785.html)                           | Only for Plug Size M                   |
| LiPo Battery 620mAh      | LP852040           | 1   | [Amazon](https://www.amazon.de/gp/product/B095VVWTSH/)                                        | LiPo 20mm x 40mm, thickness up to 10mm |
| Silicone Tube            | ID 2mm OD 3mm      | 1m  | [Amazon](https://www.amazon.de/gp/product/B075ZPR794/)                                        | Must be thin walled silicone tube      |
| Inner Spring             | 0.2mm x 45mm x 2mm | 1   | [AliExpress](https://www.aliexpress.com/item/1005001304880787.html)                           |
| Outer Spring             | 0.4mm x 10mm x 4mm | 1   | [AliExpress](https://www.aliexpress.com/item/1005001304880787.html)                           |
| Philipps Pan Head Screws | 2mm x 6mm          | 2   | [Amazon](https://www.amazon.de/gp/product/B00GMQDSRI)                                         |
| Silicone Wire            | 28AWG or 26AWG     | 1m  | [Amazon](https://www.amazon.com/DAOKI-Silicone-Stranded-Electrical-Assortment/dp/B08PBLTYD8/) | Only for Plugs with vibration motor    |

## Electronics

!!! info "Buying ready-made boards"

    As of now, you need to source the PCB yourself and take care of the assembly. But I hope that someone will make this board available to buy in the near future (already in negotiation).

The [hardware folder](https://github.com/theelims/SmartAssPlug/tree/main/hardware/PCB) contains the PCB design files exported from Fusion360 as Eagle 9.X compatible format. It opens in eagle, however it throws some error messages because font's are not supported. With the files you should be able to create the required Gerbers for your board house of choice. Double check the silk screen layers, as the font might be off.
