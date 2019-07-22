# Status Bar

![bar_left](images/bottom_bar.png "Example Output")

A multi-threaded status bar for [dwm](https://dwm.suckless.org/).

Currently, it displays this *system* information:
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

... and this *user* information:
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
