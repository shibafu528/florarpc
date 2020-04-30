#ifndef FLORARPC_ROOT_CERTIFICATES_H
#define FLORARPC_ROOT_CERTIFICATES_H

#include <grpc/grpc_security.h>

namespace Platform {
    grpc_ssl_roots_override_result grpc_root_certificates_override_callback(char **pem_root_certs);
}

#endif  // FLORARPC_ROOT_CERTIFICATES_H