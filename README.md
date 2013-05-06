Distributed Systems Spring 2013
TEAM: 2 urbansafari
MEMBERS:
rconrad 0 Ryan Conrad, rconrad54@gmail.com
jggiardina 1 Jeffrey Giardina, jggiardina@gmail.com
wvang 2 William Ang, wikalw@gmail.com

urbansafari
===========

For testing:
run client/doexp to run a robot client connection and random move experiment.
USAGE: ./doexp <experimentname> <port> [robotcount]
This runs [robotcount] client robots on the host curzon at port: <port> and writes the output of each robot to client/logs/experimentname... Each robot client runs a random list of 200 moves.

run client/doexpconn to run a robot client connection experiment. (No moves)
USAGE: ./doexp <experimentname> <port> [robotcount]
This connects [robotcount] client robots on the host curzon at port: <port> and wr
ites the output of each robot to client/logs/experimentname...

To stress test connection without spawning a new process for each client edit client/client.c with the following:
line 578: uncomment //#define JA_HACK
line 638: change magic number in for loop to # of clients to connect
This simply spawns <magic number> threads of connections to the server, but each thread is not playable and does not run beyond the stress test.


urbansafari
