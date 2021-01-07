# Load DBus session bus address
DBUS_USER=your-user
if [ -d /home/$DBUS_USER/.dbus/session-bus ]
then
	dbus_file=$(ls -t1 /home/$DBUS_USER/.dbus/session-bus | head -n1)
	export DBUS_SESSION_BUS_ADDRESS=$(cat /home/$DBUS_USER/.dbus/session-bus/$dbus_file | \
	grep "DBUS_SESSION_BUS_ADDRESS=" | \
	 sed 's/[A-Z_]*=//')
fi
