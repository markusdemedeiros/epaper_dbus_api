import dbus

bus = dbus.SystemBus()
screen_obj = bus.get_object("io.markusde.epaper", "/io/markusde/epaper")
screen = dbus.Interface(screen_obj, "io.markusde.epaper")

screen.setup()
screen.flush()
