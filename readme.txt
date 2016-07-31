This is a guide on how to use myserver.

Initially you need to run "run.sh"
Execute ./myserver portno
For eg ./myserver 9000
This sets up a TCP server at port 9000 and UDP at 9001 (always consecutive)
Therefore care must be taken while setting up 2nd server that it doesn't use an already port as above.

use connect IP to tell the IP of the client on the other side.
use chport portno to tell the TCP port used by the other side.

Repeat above 2 commands for both the servers

use myip IP to tell the other side about the IP you are using.
udpport udportno to tell the other side about the UDP port no your server is using.

Repeat above 2 commands for both sides.

for eg ./myserver 9000                                            ./myserver 9002
connect 10.42.3.90												   connect 10.42.4.90										
chport 9002                                                        chport 9000
myip 10.42.4.90													    myip 10.42.3.90
udpport 9001                                                        udpport 9003

**************************** START USING NORMAL COMMANDS ****************************************

Contributers:
  1. ishaan1996
  2. AMaini503
