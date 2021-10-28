# Relay-Server

These two programs allow you to turn a relay on/off from a remote location using two raspberry pi's 
(tested on a PI-2 but any flavor should work fine). The control signals are sent over a TCP/IP
network, so it is necessary for the server to be network accessable. The IP address must be known
to the client computer, and is currently a #define statement in the source code. If both computers
are on the same network then the server does not need to be publically accessable.

The program relayServer.c runs on the PI with the relay attached.
The program relayClient.c runs on the PI with the switch to turn on/off the remote relay.

Starting out:
The PI with the relay runs the program relayServer. When a command is sent over the network from
the client computer, the command is decoded, the relay is energized, and a response is sent back to
the client computer. When the client sees the response, it lights up a LED giving feedback to the user.
Over a reasonably fast connection the response is almost immediate.
When the switch on the client computer is turned off, a command is sent to the server and the relay is 
de-energized. A response is sent back to the client, and the LED is turned off.

While the client is connected to the server, the server will send a keep-alive packet to the client.
If the client fails to respond the server will close the connection. This happens if the connection
is broken by hardware failure or if the client computer stops working.

Currently there is provision for one relay/switch, but is should be a copy/paste change to add as many
relay/switch/LED's as needed. As written, the programs use about 0.2% of the CPU's resources.

On the server end, the top of the relay coil connects to VCC (+5v, +12V, whatever). The bottom of the 
relay coil connects to the drain of a 2N7000 fet. The source goes to ground, and the gate goes to the
output pin of the raspberry pi (currently defined as hardware pin 16 (GPIO 23).

On the client end, one end of the switch goes to ground. The other end of the switch goes to hardware 
pin 18 (GPIO 24). There needs to be a pull-up resistor on the switch (the internal pull-up resistors
are not reliably controlled due to changes in the rasbian image changing). One end goes to +3.3v, obtained
in the PI's hardware pin 1 of the PI's 40 pin expansion port. The other end of the resistor connects to 
hardware pin 18. This puts the hardware at 3.3 volts until the switch is closed, at which point the pin goes
to ground. Make sure you use the right pin or your PI can be quickly destroyed.

Also on the client PI connect hardware pin 35 to a 680 ohm resistor. The other end of the resistor
connects to the positive lead of a low-voltage LED (red/yellow). The PI doesn't have enough voltage to 
directly drive a higher voltage (blue, green, white) LED. The negative lead of the LED goes to ground.

Start the server program first (relayServer).

Next, start the client program (relayClient). When the switch on the client side is ON, the relay will 
energize and the LED on the client will light. When the switch on the client side is turned off, the relay
will turn off and the LED on the client will also turn off.

Building the programs:

The hardware on the PI's are controlled with the wiringPi library. If it is not already installed run:

sudo apt-get install wiringpi

On the server end (relayServer.c) the TCP/IP port is defined as 9000. Change this if needed. If you do, don't
forget to make the same change in the relayClient.c file.  Then build the server program:

cc -o relayServer relayServer.c -lwiringPi

On the client computer, set the SERVER_ADDR to the IP address of the server computer. This is 
defined just after the comments. If you changed the port number of the server, also change the 
port number here.

Now build the client:

cc -o relayClient relayClient.c -lwiringPi

Start the client (after making sure both computers on connected to a network.
The server can be left running, and the client turn on/off as need be.
