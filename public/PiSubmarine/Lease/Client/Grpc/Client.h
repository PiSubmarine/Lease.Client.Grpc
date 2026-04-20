#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Grpc/Api/LeaseService.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    struct TlsConfig
    {
        std::string Target;
        std::string CertificateAuthority;
        std::string ClientCertificateChain;
        std::string ClientPrivateKey;
        std::string ServerAuthorityOverride;
        std::chrono::milliseconds RpcTimeout{5000};
    };

    class Client final : public Api::ILeaseIssuer
    {
    public:
        explicit Client(TlsConfig tlsConfig);

        [[nodiscard]] Error::Api::Result<Api::Lease> AcquireLease(const Api::LeaseRequest& request) override;
        [[nodiscard]] Error::Api::Result<Api::Lease> RenewLease(const Api::LeaseId& leaseId) override;
        [[nodiscard]] Error::Api::Result<void> ReleaseLease(const Api::LeaseId& leaseId) override;

    private:
        [[nodiscard]] Error::Api::Result<Api::Lease> ReadLeaseResult(
            const ::grpc::Status& status,
            const ::pisubmarine::lease::grpc::api::LeaseResult& response) const;
        [[nodiscard]] Error::Api::Result<void> ReadVoidResult(
            const ::grpc::Status& status,
            const ::pisubmarine::lease::grpc::api::VoidResult& response) const;

        TlsConfig m_TlsConfig;
        std::shared_ptr<::grpc::Channel> m_Channel;
        std::unique_ptr<::pisubmarine::lease::grpc::api::LeaseService::Stub> m_Stub;
    };
}
