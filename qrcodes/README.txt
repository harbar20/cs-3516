INSTRUCTIONS FOR SERVER
To make the server, run "make server." This will use the provided Makefile to compile our server.c.

To run the server, run "./server." This will use the defaults as follows:
- Port: 2012
- Rate limit: 3 requests per user per 60 seconds
- Maximum number of concurrent users: 3
- Time out connections: 80 seconds

To change these values, add the flags as follows:
- Port: -p [port between 2000-3000]
- Rate limit: -r [maximum requests per user] [maximum time frame]
- Maximum number of concurrent users: -m [number of users]
- Time out connections: -t [number of seconds]
You can add the flags in any order when running the server.

For example:
$ make server
$ ./server -p 2012 -r 3 60 -m 3 -t 80

------------------------------------------------------------------------------------------------------------
INSTRUCTIONS FOR CLIENT
To make the client, run "make client." This will use the provided Makefile to compile our client.c.

To run the client, run "./client [IP address of server] [file containing QR code] [port of server]"

All these options are required. For example:
$ make client
$ ./client 127.0.0.1 test.png 2012

------------------------------------------------------------------------------------------------------------
KNOWN BUGS
- Sometimes, the server will error out with clients exceeding the max users, and sometimes, it will work just fine. Other times, however, it will work as if there is no max user limit.
- There is no functional rate limit
- A client connecting to a server one at a time works 90% of the time. We have noticed that it receives nothing from the server often when first tested right after a VM is turned on/booted up. 
