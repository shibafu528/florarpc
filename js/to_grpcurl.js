/*
 * Copy to gRPCurl
 */

const args = [];

// -d
args.push('-d');
args.push(`'${JSON.stringify(request.body)}'`);

// -H
for (const key in request.metadata) {
    args.push('-H');
    args.push(`"${key}: ${request.metadata[key]}"`);
}

// -import-path
for (const path of imports) {
    args.push('-import-path');
    args.push(`"${path}"`);
}
if (imports.length === 0) {
    const idx = request.protoFile.lastIndexOf('/');
    const dir = request.protoFile.substring(0, idx);
    args.push('-import-path');
    args.push(`"${dir}"`);
}

// -proto
args.push('-proto');
args.push(`"${request.protoFile}"`);

// server
if (server.useTLS) {
    if (server.certificate.rootCerts) {
        args.push('-cacert');
        args.push(`${server.certificate.rootCerts}`);
    }
    if (server.certificate.privateKey) {
        args.push('-key');
        args.push(`${server.certificate.privateKey}`);
    }
    if (server.certificate.certChain) {
        args.push('-cert');
        args.push(`${server.certificate.certChain}`);
    }
} else {
    args.push('-plaintext');
}
args.push(server.address);

const path = request.path.startsWith('/') ? request.path.substring(1) : request.path;
args.push(path);

`grpcurl ${args.join(' ')}`;
