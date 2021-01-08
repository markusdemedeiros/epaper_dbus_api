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


def draw_xkcd_comic():
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
            time.sleep(30)
    except (dbus.exceptions.DBusException):
        pass


main()
