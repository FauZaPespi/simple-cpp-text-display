# simple cpp text display

Build it:
```bash
g++ -o text_display text_display.cpp -lX11 -lXext -lXrender -lXft -lXinerama -lXcursor -lXcomposite -lXdamage -lXtst -lXxf86vm
```
Usage Examples:

Display "Hello World" in red in the top-right corner:
```bash

./text_display --text "Hello World"
```

Display "Warning!" in yellow at the center with large margins:
```bash
./text_display --text "Warning!" --position center --color FFFF00 --marginx 50 --marginy 50
```
Display "Notice" in green in the bottom-left corner for 10 seconds:
```bash
./text_display --text "Notice" --position bottom-left --color 00FF00 --time 10
```
Display "ERROR" in red with a black background in the top-right corner:
```bash
./text_display --text "ERROR" --transparent false --color FF0000
```
Help:
```bash
./text_display --help
```
