# Lease.Client.Grpc

`PiSubmarine.Lease.Client.Grpc` implements `Lease.Api::ILeaseIssuer` over a
secure gRPC channel.

It targets a remote gRPC service endpoint and can be pointed at a shared gRPC
host that serves multiple PiSubmarine domains.

## Security

This client uses the shared `Grpc.Client` channel for mutual TLS:

- server certificate is verified against the configured CA
- client certificate and private key authenticate the caller
- unauthenticated RPC failures are reported back through `Result`

Because it implements `Lease.Api::ILeaseIssuer`, callers stay unaware of gRPC,
channel reuse, and TLS details.
