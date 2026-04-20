#include "PiSubmarine/Lease/Client/Grpc/Client.h"

#include <stdexcept>
#include <string_view>

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Lease/Api/ErrorCode.h"
#include "PiSubmarine/Lease/Client/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    namespace
    {
        [[nodiscard]] Error::Api::Error MakeCommunicationError(const ErrorCode code)
        {
            return Error::Api::MakeError(Error::Api::ErrorCondition::CommunicationError, make_error_code(code));
        }

        [[nodiscard]] Error::Api::Error MakeContractError(const ErrorCode code)
        {
            return Error::Api::MakeError(Error::Api::ErrorCondition::ContractError, make_error_code(code));
        }

        [[nodiscard]] Error::Api::Error FromProtoError(const ::pisubmarine::lease::grpc::api::ErrorPayload& error)
        {
            std::error_code cause{};
            if (error.lease_error_code() != 0)
            {
                cause = Api::make_error_code(static_cast<Api::ErrorCode>(error.lease_error_code()));
            }

            return Error::Api::MakeError(
                static_cast<Error::Api::ErrorCondition>(error.error_condition()),
                cause);
        }

        [[nodiscard]] Api::Lease FromProtoLease(const ::pisubmarine::lease::grpc::api::LeasePayload& lease)
        {
            return Api::Lease{
                .Id = Api::LeaseId{.Value = lease.id()},
                .Resource = Api::ResourceId{.Value = lease.resource()},
                .Duration = std::chrono::milliseconds(lease.duration_ms())};
        }
    }

    Client::Client(TlsConfig tlsConfig)
        : m_TlsConfig(std::move(tlsConfig))
    {
        if (m_TlsConfig.Target.empty() ||
            m_TlsConfig.CertificateAuthority.empty() ||
            m_TlsConfig.ClientCertificateChain.empty() ||
            m_TlsConfig.ClientPrivateKey.empty())
        {
            throw std::invalid_argument("Lease.Client.Grpc requires complete mutual TLS configuration");
        }

        ::grpc::SslCredentialsOptions sslOptions;
        sslOptions.pem_root_certs = m_TlsConfig.CertificateAuthority;
        sslOptions.pem_cert_chain = m_TlsConfig.ClientCertificateChain;
        sslOptions.pem_private_key = m_TlsConfig.ClientPrivateKey;

        ::grpc::ChannelArguments channelArguments;
        if (!m_TlsConfig.ServerAuthorityOverride.empty())
        {
            channelArguments.SetSslTargetNameOverride(m_TlsConfig.ServerAuthorityOverride);
        }

        m_Channel = ::grpc::CreateCustomChannel(
            m_TlsConfig.Target,
            ::grpc::SslCredentials(sslOptions),
            channelArguments);
        m_Stub = ::pisubmarine::lease::grpc::api::LeaseService::NewStub(m_Channel);
    }

    Error::Api::Result<Api::Lease> Client::AcquireLease(const Api::LeaseRequest& request)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_TlsConfig.RpcTimeout);

        ::pisubmarine::lease::grpc::api::AcquireLeaseRequest protoRequest;
        protoRequest.set_resource(request.Resource.Value);
        ::pisubmarine::lease::grpc::api::LeaseResult response;

        return ReadLeaseResult(m_Stub->AcquireLease(&context, protoRequest, &response), response);
    }

    Error::Api::Result<Api::Lease> Client::RenewLease(const Api::LeaseId& leaseId)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_TlsConfig.RpcTimeout);

        ::pisubmarine::lease::grpc::api::RenewLeaseRequest protoRequest;
        protoRequest.set_lease_id(leaseId.Value);
        ::pisubmarine::lease::grpc::api::LeaseResult response;

        return ReadLeaseResult(m_Stub->RenewLease(&context, protoRequest, &response), response);
    }

    Error::Api::Result<void> Client::ReleaseLease(const Api::LeaseId& leaseId)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_TlsConfig.RpcTimeout);

        ::pisubmarine::lease::grpc::api::ReleaseLeaseRequest protoRequest;
        protoRequest.set_lease_id(leaseId.Value);
        ::pisubmarine::lease::grpc::api::VoidResult response;

        return ReadVoidResult(m_Stub->ReleaseLease(&context, protoRequest, &response), response);
    }

    Error::Api::Result<Api::Lease> Client::ReadLeaseResult(
        const ::grpc::Status& status,
        const ::pisubmarine::lease::grpc::api::LeaseResult& response) const
    {
        if (!status.ok())
        {
            if (status.error_code() == ::grpc::StatusCode::UNAUTHENTICATED ||
                status.error_code() == ::grpc::StatusCode::PERMISSION_DENIED)
            {
                return std::unexpected(MakeCommunicationError(ErrorCode::Unauthenticated));
            }

            return std::unexpected(MakeCommunicationError(ErrorCode::RpcFailed));
        }

        switch (response.value_case())
        {
        case ::pisubmarine::lease::grpc::api::LeaseResult::kLease:
            return FromProtoLease(response.lease());
        case ::pisubmarine::lease::grpc::api::LeaseResult::kError:
            return std::unexpected(FromProtoError(response.error()));
        case ::pisubmarine::lease::grpc::api::LeaseResult::VALUE_NOT_SET:
            return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
        }

        return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
    }

    Error::Api::Result<void> Client::ReadVoidResult(
        const ::grpc::Status& status,
        const ::pisubmarine::lease::grpc::api::VoidResult& response) const
    {
        if (!status.ok())
        {
            if (status.error_code() == ::grpc::StatusCode::UNAUTHENTICATED ||
                status.error_code() == ::grpc::StatusCode::PERMISSION_DENIED)
            {
                return std::unexpected(MakeCommunicationError(ErrorCode::Unauthenticated));
            }

            return std::unexpected(MakeCommunicationError(ErrorCode::RpcFailed));
        }

        switch (response.value_case())
        {
        case ::pisubmarine::lease::grpc::api::VoidResult::kSuccess:
            return {};
        case ::pisubmarine::lease::grpc::api::VoidResult::kError:
            return std::unexpected(FromProtoError(response.error()));
        case ::pisubmarine::lease::grpc::api::VoidResult::VALUE_NOT_SET:
            return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
        }

        return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
    }
}
