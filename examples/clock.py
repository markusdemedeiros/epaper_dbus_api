import signal
import sys
import dbus
import time
import math

bus = dbus.SystemBus()
screen_obj = bus.get_object("io.markusde.epaper", "/io/markusde/epaper")
screen = dbus.Interface(screen_obj, "io.markusde.epaper")

# Example clock body
CL_X0=600   # Clock centre x
CL_Y0=240   # Clock centre y
CL_R0=150   # Clock outline radius
CL_LW=5     # Clock outline linewidth
HH_R1=70    # Hour hand length
HH_LW=3     # Hour hand linewidth
MH_R1=120   # Minute hand length
MH_LW=2     # Minute hand linewidth

def clock_body():
    def cx(r, a):
        return int(round(r*(math.cos(2 * math.pi * a - math.pi/2))))

    def cy(r, a):
        return int(round(r*(math.sin(2 * math.pi * a - math.pi/2))))

    hour, minute = map(int, time.strftime("%I %M").split())
    min_rnd = 5 * round(minute/5)       # Minute hand will be rounded to nearest 5 minutes

    hr_a = (hour * 60 + minute) / (12 * 60.0)
    mn_a = min_rnd / 60.0

    return [(6,CL_X0,CL_Y0,CL_R0,0,1,0,0,1,""),
        (6,CL_X0,CL_Y0,CL_R0-CL_LW,0,0,0,0,1,""),
        (4,CL_X0,CL_Y0,CL_X0+cx(HH_R1,hr_a),CL_Y0+cy(HH_R1,hr_a),1,0,HH_LW,0,""),
        (4,CL_X0,CL_Y0,CL_X0+cx(MH_R1,mn_a),CL_Y0+cy(MH_R1,mn_a),1,0,MH_LW,0,""),
        (6,CL_X0,CL_Y0,2+CL_LW,0,0,0,0,1,""),
        (6,CL_X0,CL_Y0,CL_LW,0,1,0,0,1,"")]



def signal_handler(sig, frame):
    print("closing program and server")
    screen.serv_close()
    sys.exit(0)



def main():
    try:
        signal.signal(signal.SIGINT, signal_handler)
        time.sleep(5)  # Allow short setup time for other programs to enqueue initial commands
        while(1):
            screen.apply(clock_body())
            screen.push()
            time.sleep(60 * 5)
    except (dbus.exceptions.DBusException):
        pass


main()
