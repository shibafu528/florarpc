#include "Certificate.h"

Certificate::Certificate() : Certificate(QUuid::createUuid()) {}

Certificate::Certificate(QUuid id) : id(id) {}
