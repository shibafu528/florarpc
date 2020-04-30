#if !__has_feature(objc_arc)
#error "ARC is off"
#endif

#include "platform/RootCertificates.h"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

grpc_ssl_roots_override_result Platform::grpc_root_certificates_override_callback(char **pem_root_certs) {
    @autoreleasepool {
        CFArrayRef certs = NULL;
        OSStatus status = SecTrustCopyAnchorCertificates(&certs);
        if (status != noErr) {
            return GRPC_SSL_ROOTS_OVERRIDE_FAIL;
        }

        NSMutableData *pemData = [NSMutableData data];
        CFIndex certsLength = CFArrayGetCount(certs);
        for (CFIndex i = 0; i < certsLength; i++) {
            CFTypeRef cert = CFArrayGetValueAtIndex(certs, i);
            if (cert == NULL) {
                continue;
            }

            CFDataRef data;
            status = SecItemExport(cert, kSecFormatPEMSequence, kSecItemPemArmour, NULL, &data);
            if (status != noErr) {
                continue;
            }

            [pemData appendData:(__bridge_transfer NSData *)data];
        }
        CFRelease(certs);

        NSUInteger pemBufferLength = [pemData length] + 1;
        *pem_root_certs = static_cast<char *>(malloc(pemBufferLength));
        if (*pem_root_certs == nullptr) {
            return GRPC_SSL_ROOTS_OVERRIDE_FAIL;
        }
        [[[NSString alloc] initWithData:pemData encoding:NSUTF8StringEncoding] getCString:*pem_root_certs
                                                                                maxLength:pemBufferLength
                                                                                 encoding:NSASCIIStringEncoding];
    }

    return GRPC_SSL_ROOTS_OVERRIDE_OK;
}
