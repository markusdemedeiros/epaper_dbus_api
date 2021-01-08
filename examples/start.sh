#!/bin/sh

# Example of launching the server and a clock program

(trap "kill 0" 2; ../epd & sleep 1 & python3 startup.py && python3 ./hackernews.py & python3 ./clock.py)

