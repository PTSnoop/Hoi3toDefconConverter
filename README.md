# Hoi3 to Defcon Converter

Played your [Paradox megacampaign](https://forum.paradoxplaza.com/forum/index.php?threads/paradox-game-converters.743404/) all the way through Crusader Kings 2, Europa Universalis IV, Victoria 2 and Hearts Of Iron 3? Disappointed that your thousand-year magnum opus has now ground to a halt in 1945?

There is now an answer!

The Hoi3 to Defcon converter takes save files from the World War 2 strategy game [Hearts Of Iron 3](http://store.steampowered.com/app/25890/), and converts them into mods for the nuclear war strategy game [DEFCON](http://store.steampowered.com/app/1520/).

To use the converter, edit configuration.txt to add the correct paths and options (explained therein), then run Hoi3ToDefcon.exe .

---

# Features

* You can set up Allies / Axis / Comintern wars, choose the sides yourself, or just let the converter automatically pick the six largest countries to do battle.
* Supports vanilla Hoi3, Vic2toHoi3-converter games, or modded Hoi3 mods.
* Should continue to illustrate why nuclear war is a really really bad idea.

---

# Limitations

* Should work with pretty much all Hoi3 mods, including ones that change the map - but only one mod at a time. And I've only tested it on mods from the [V2 to HoI3 Converter](https://forum.paradoxplaza.com/forum/index.php?threads/v2-to-hoi3-converter-mod.582798/), so your mileage may vary.
* All players start off with equal populations and units, regardless of industrial capacity or land covered. There's no way of changing this without delving into the DEFCON source code itself.
* DEFCON has no save games, so there's no way we'll be able to create a converter further up the Paradox timeline from DEFCON to Stellaris. I suppose you'll have to go Hoi4->Stellaris, and leave DEFCON as a non-canon what-could-have-happened option.
* Probably doesn't support Hearts Of Iron 4. Yet.

Paradox-save-format parser and logging code taken from the other [Paradox Game Converters](https://github.com/Idhrendur/paradoxGameConverters), used under MIT licence.

# Screenshots

![Alternate History](http://imgur.com/aIm08qO.png)

If 1941 had nukes: Axis v Allies v Comintern.

![Border Gore](http://imgur.com/VGRVcYS.png)

Can cope with all sorts of crazy borders.

![Unexpected Finland](http://imgur.com/o6gr9Gd.png)

Unlikely superpowers: Kola, Kimak, and the Kingdom of America.
