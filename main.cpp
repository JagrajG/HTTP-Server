#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// create the socket
int s = socket(AF_INET, SOCK_STREAM, 0);
