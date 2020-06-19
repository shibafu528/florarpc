#include "Request.h"

Request::Request(const Method &method, QObject *parent) : QObject(parent), method(method) {}
