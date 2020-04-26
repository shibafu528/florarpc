#include "Server.h"

Server::Server() : Server(QUuid::createUuid()) {}

Server::Server(QUuid id) : id(id) {}
