# Epaper DBUS API
A DBus API which exposes drawing features of the Waveshare 7.5 inch epaper screen to other programs. 


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

- Libraries

## Usage
To start up the server, run `./epd` as root. 

