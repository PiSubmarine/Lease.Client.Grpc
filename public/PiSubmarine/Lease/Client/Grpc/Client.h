#pragma once

#include <memory>

#include "PiSubmarine/Grpc/Client/Channel.h"
#include "PiSubmarine/Lease/Api/ILeaseIssuer.h"
#include "PiSubmarine/Lease/Grpc/Api/LeaseService.h"
#include "PiSubmarine/Logging/Api/IFactory.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    class Client final : public Api::ILeaseIssuer
    {
    public:
        Client(Logging::Api::IFactory& loggerFactory, ::PiSubmarine::Grpc::Client::Channel& channel);

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

        std::shared_ptr<spdlog::logger> m_Logger;
        ::PiSubmarine::Grpc::Client::Channel& m_Channel;
        std::unique_ptr<::pisubmarine::lease::grpc::api::LeaseService::Stub> m_Stub;
    };
}
