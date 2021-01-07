# Epaper DBUS API
A DBus API which exposes drawing features of the Waveshare 7.5 inch epaper screen to other programs. 

## Disclaimer
I am very new to using DBus, asynchronous C programming, and a myriad of other topics used in this project. Keep in mind that this is primarilty a project to learn these tools if you attempt to use this code. I value any and all feedback!


## Configuration
- Permissions:
	This program needs to be run as root in order to access the GPIO pins. Additionally, dbus needs to allow this program system bus access. This will differ based on your specific system, but on my system it was sufficient to add a file `/etc/dbus-1/system.d/epaper-dbus.conf`:
```
<!DOCTYPE busconfig PUBLIC
          "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
          "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <!-- Allow anyone to own epaper-->
  <policy context="default">
    <allow own="io.markusde.epaper"/>
	<allow send_destination="io.markusde.epaper"/>
  </policy>
</busconfig>
```

## Usage
To start up the server, run `./epd` as root. A server can revieve the following commands (sent on the system bus to `io.markusde.epaper` with object path `/io/markusde/epaper`:
| Method | Description |
| ------ | ----------- |
| setup  | Establishes connection to the screen | 
| close  | Closes connection to the screen |
| flush	 | Resets buffered image to white |
| apply  | Applies a list of drawing commands to the buffered image. See below. |
| push   | Draws the buffered image to the epaper display |
| clear  | Clears the epaper display to white |

The server is multithreaded so none of these commands block the DBus interface. The server enqueues commands to be excecuted in the order it recieves them. This is particularly important since, on an epaper display, the `push` and `clear` arguments take several seconds to complete. 

A typical use case would have one program periodically sending `apply` commands for commonly drawn elements and `push`ing the results to the screen, and several other programs which update less frequently sending their `apply` commands to the screen and only `push`ing if an update is urgent. 

Each of these commands will return `(1,)` if it was revieced successfully. API Commands are also outlined in the `introspect` dbus interface. 

### Apply arguments
The apply method takes one argument which has type `[(qqqqqyyyys)]`. Each struct represents a drawing command, and the entire array will be excecuted atomically- this prevents a `push` when an element is partially drawn. 

A drawing command is a struct consisting of the fields
```
(cmd, x0, y0, x1, y1, color_f, color_b, dot_pixel, aux, data)
```
which are interpreted as defined in this table
| cmd | description |
| --- | ----------- |
| 0x0 | clears screen to colour `color_f` |
| 0x1 | clears screen in the rectangle (`x0`, `y0`, `x1`, `y1`) to `color_f` |
| 0x2 | colours pixel (`x0`, `y0`) to `color_f` |
| 0x3 | draws a point at (`x0`, `y0`) with color `color_f`, dot type (width) `dot_pixel`, and dot style `aux` |
| 0x4 | draws a line from (`x0`, `y0`) to (`x1`, `y1`) with color `color_f`, line type (width) `dot_pixel`, and line style `aux` |
| 0x5 | draws the rectangle (`x0`, `y0`, `x1`, `y1`) with color `color_f`, line type (width) `dot_pixel`, and fill `aux` |
| 0x6 | draws the circle centred at (`x0`, `y0`) with radius `x1`, color `color_f`, line type (width) `dot_pixel`, and fill `aux` |
| 0x7 | draws the string `dat` at position (`x0`, `y0`), with foreground color `color_f`, background color `col_b`, and font `aux` |
| 0x8 | draws the bitmap file `dat` at position (`x0`, `y0`) |

and are applied to the image buffer. Malformed commands fail silently. 

To enable support for more fonts or colours (for example, a shaded or hatchmarked fill), you can edit the lookup tables in `src/screendriver.c`. This will change how arguments are interpreted when they are drawn to the screen. 


## Attribution
- Queue Library: [StsQueue](https://github.com/petercrona/StsQueue)

- Waveshare libraries: The code that this project is based around is [written by Waveshare](https://github.com/waveshare/e-Paper), and there is also a plethora of information on their [wiki](https://www.waveshare.com/wiki/7.5inch_e-Paper_HAT). 

- GPIO Library: [BCM2835](http://www.airspayce.com/mikem/bcm2835/)

- DBus Library: This project uses `libdbus-1-dev`, as found in the regular raspberry pi repositories. The tool `gdbus` is used in some of the example scripts and is very helpful for manually communuicating with the server (`dbus-send` does not allow sending nested containers, so won't be able to send 
