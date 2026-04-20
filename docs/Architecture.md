# Lease.Client.Grpc

`PiSubmarine.Lease.Client.Grpc` implements `Lease.Api::ILeaseIssuer` over a
secure gRPC channel.

## Security

This client uses mutual TLS:

- server certificate is verified against the configured CA
- client certificate and private key authenticate the caller
- unauthenticated RPC failures are reported back through `Result`

Because it implements `Lease.Api::ILeaseIssuer`, callers stay unaware of gRPC
and TLS details.
