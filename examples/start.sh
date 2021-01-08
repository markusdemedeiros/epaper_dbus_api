#!/bin/sh

# Example of launching the server and a clock program

(trap "kill 0" 2; ../epd & sleep 1 && python ./clock.py)

