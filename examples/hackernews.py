import signal
import sys
import dbus
import time

import requests, bs4
import unicodedata

bus = dbus.SystemBus()
screen_obj = bus.get_object("io.markusde.epaper", "/io/markusde/epaper")
screen = dbus.Interface(screen_obj, "io.markusde.epaper")


def signal_handler(sig, frame):
    print("closing program")
    sys.exit(0)

HN_OX = 20
HN_OY = 20
HN_LX = 18
HN_LY = 18
draw_hn_container = [(8,HN_OX,HN_OY-1,0,0,0,0,1,1,"/home/pi/doc/projects/epaper/pic/hn.bmp"),(7,HN_OX+HN_LX+3,HN_OY,0,0,0,1,0,3,"Hacker News")]

NUMPOSTS = 10

def draw_hn_posts():
    res = requests.get('https://news.ycombinator.com/')

    try:
        res.raise_for_status()
    except Exception as ex:
        print("[hn] Problem accessing website")

    hn = bs4.BeautifulSoup(res.text, 'html.parser')
    elems = hn.select('.storylink')
    
    ret = []

    for i in range(min(NUMPOSTS, len(elems))):
        # Attempt to normalize to ascii
        data = unicodedata.normalize('NFKD', elems[i].getText()).encode('ascii', 'ignore').decode('ascii', 'ignore')

        disp = (data[:40] + '...') if len(data) > 43 else data
        print(disp)
        ret.append((7,20,40 + 13*i,0,0,0,1,0,1,disp))

    return ret


def main():
    try:
        signal.signal(signal.SIGINT, signal_handler)
        while(1):
            screen.apply(draw_hn_container + draw_hn_posts())
            time.sleep(60*10)                   # Refresh information every 10 minutes
    except (dbus.exceptions.DBusException):
        pass


main()
