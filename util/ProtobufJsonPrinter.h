#ifndef FLORARPC_PROTOBUFJSONPRINTER_H
#define FLORARPC_PROTOBUFJSONPRINTER_H

#include <string>
#include <google/protobuf/descriptor.h>

namespace ProtobufJsonPrinter {
    std::string makeRequestSkeleton(const google::protobuf::Descriptor *descriptor);
}

#endif //FLORARPC_PROTOBUFJSONPRINTER_H
