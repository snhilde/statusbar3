# Status Bar

![bottom_bar](images/bottom_bar.png "Example Output")

A multi-threaded status bar for [dwm](https://dwm.suckless.org/).

Currently, it displays this __system__ information:
* Network usage (download and upload)
* Disk usage
* Memory usage
* CPU usage (total and load)
* CPU temp
* Fan speed
* Battery status
* System volume
* Wifi status
* Time

... and this __user__ information:
* TODO list updates
* Weather (current and forecasted)


# Table of Contents #
* [Prerequisites](#prerequisites)
* [Recommendations](#recommendations)
* [Installation](#installation)
* [Usage](#usage)
* [Getting Started](#getting-started)
	* [Disk routine](#disk-routine)
	* [Time routine](#time-routine)
	* [TODO routine](#todo-routine)
	* [Weather routine](#weather-routine)
	* [Colors](#colors)
* [Contributing](#contributing)
* [Author](#author)
* [License](#license)
* [Acknowledgments](#acknowledgments)


## Prerequisites ##
Status Bar requires 3 libraries for compilation:
* pthread
* m
* X11

These should already be installed, and you shouldn't need to do anything about them.

There are additional requirements for individual routines. If your machine does not have a library installed that
a certain routine needs, then that routine will be skipped during compilation. This is all handled by
[Autotools](https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html),
particularly [Autoconf and configure.ac](https://www.gnu.org/software/autoconf/autoconf.html).

These are the routine-specific libraries:
* Volume: libasound2
* Weather: [libcurl](https://curl.haxx.se/libcurl/)

Beyond that, if a routine is not being built like you expected, check the output for `./configure` (saved in config.log)
and the header checks in configure.ac to see what is missing.


## Recommendations ##
Status Bar works best with Andrew Milkovich's [dualstatus patch](https://dwm.suckless.org/patches/dualstatus/), which
allows for displaying more information. If you choose not to use this patch in your setup, you should remove the
semicolon separator that divides the top and bottom bars by removing `DELIMITER` from your chosen routines in config.h.

All output is colored with [Clément Sipieter's](https://github.com/sipi) [status2d patch](https://dwm.suckless.org/patches/status2d/).
If you do not use this patch, you should set `color_text` to `SB_FALSE` in config.h.


## Installation ##
```
./configure
make
make install
```


## Usage ##
If the binary is installed somewhere in your PATH, you need only run `statusbar`.

To start the program when X11 starts up, add this line to your .xsession (or .xinitrc, whatever you use)
before the call for dwm:
```
/path/to/statusbar 2> /home/user/statusbar.log &
```
This will start the program, log the errors to statusbar.log, and not block the queue during startup.


## Getting Started ##
You can choose which routines you want displayed by editing src/config.h. The routines will be displayed in order
from left to right on the status bar, meaning that the topmost routine will appear first on the left. If you are
using the [dualstatus patch](#recommendations), then the special routine value of DELIMITER will print a semicolon
at your chosen breakpoint, allowing for dual status bars.

Let's take the battery routine as an example:
```
{ BATTERY , 30, "#FFFFFF", "#BB4F2E", "#A1273E" },
```
This will run the battery routine every 30 seconds, using the selected [colors](#colors) depending on the conditions.

If you do not want to run a routine or have it printed to the status bar, simply remove its line from the array.

A few routines take some customized values, all of which can be found in src/config.h. These are the values you change:

### Disk routine ###
The first value is the path to the mounted filesystem for which you want to display disk usage.
The second value is the name you want it displayed as.
```
	{ "/"    , "root" }
	{ "/home", "home" }
```

### Time routine ###
This is how you want the time/date to be displayed. For conversion specifications, see `strftime(3)`.
```
time_format = "%b %d - %I:%M"
```

### TODO routine ###
If you have a TODO list you want displayed on the status bar, you can specify the path here.
__Note:__ This is the path relative to your home directory.
```
todo_path = ".TODO"
```

### Weather routine ###
This is your zip code for displaying the current temperature and tomorrow's low/high.
```
zip_code = "90210"
```

## Colors ##
Additionally, if you're using the status2d patch, you can tailor the colors of each, from config.h.
For example:
```
{ CPU_USAGE, 1, "#FFFFFF", "#BB4F2E", "#A1273E" },
```
This will print the CPU usage each second using the color white (#FFFFFF) for normal conditions,
orange (#BB4F2E) for warning conditions, and red (#A1273E) for error conditions.

All values must be a pound (#) sign followed by six hex digits, in the style of RRGGBB.

If `color_text` is set to SB_FALSE, then these color codes will be ignored.


## Contributing ##
Send a pull request or a message. Additional functionality is welcome, as are suggestions to make the program leaner,
faster, and better performing.


## Author ##
Hilde N.


## License ##
This project is licensed under the MIT License. Do whatever you want with it.
See the [LICENSE](LICENSE) file for details


## Acknowledgments ##
* [Dave Gamble](https://github.com/DaveGamble) for creating [cJSON](https://github.com/DaveGamble/cJSON)
* The [curl team](https://www.haxx.se/curl.html)
* Everyone who contributed to the open-source projects and provided me with lots of wisdom, thank you.
