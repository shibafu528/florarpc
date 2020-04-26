#ifndef FLORARPC_SERVER_H
#define FLORARPC_SERVER_H

#include <QString>

class Server {
public:
    QString name;
    QString address;
    bool useTLS;
};

#endif  // FLORARPC_SERVER_H
