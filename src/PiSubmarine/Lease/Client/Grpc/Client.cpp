#include "PiSubmarine/Lease/Client/Grpc/Client.h"

#include <chrono>
#include <stdexcept>
#include <string_view>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

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

        [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(Logging::Api::IFactory& loggerFactory)
        {
            auto logger = loggerFactory.CreateLogger("Lease.Client.Grpc");
            if (!logger)
            {
                throw std::invalid_argument("Lease.Client.Grpc requires a logger factory that returns a logger");
            }

            return logger;
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

    Client::Client(Logging::Api::IFactory& loggerFactory, ::PiSubmarine::Grpc::Client::Channel& channel)
        : m_Logger(CreateLogger(loggerFactory))
        , m_Channel(channel)
    {
        m_Stub = ::pisubmarine::lease::grpc::api::LeaseService::NewStub(m_Channel.Get());
        SPDLOG_LOGGER_INFO(m_Logger, "Initialized gRPC lease client using injected shared channel");
    }

    Error::Api::Result<Api::Lease> Client::AcquireLease(const Api::LeaseRequest& request)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_Channel.GetRpcTimeout());

        ::pisubmarine::lease::grpc::api::AcquireLeaseRequest protoRequest;
        protoRequest.set_resource(request.Resource.Value);
        ::pisubmarine::lease::grpc::api::LeaseResult response;

        SPDLOG_LOGGER_INFO(m_Logger, "Sending AcquireLease request for resource '{}'", request.Resource.Value);
        return ReadLeaseResult(m_Stub->AcquireLease(&context, protoRequest, &response), response);
    }

    Error::Api::Result<Api::Lease> Client::RenewLease(const Api::LeaseId& leaseId)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_Channel.GetRpcTimeout());

        ::pisubmarine::lease::grpc::api::RenewLeaseRequest protoRequest;
        protoRequest.set_lease_id(leaseId.Value);
        ::pisubmarine::lease::grpc::api::LeaseResult response;

        SPDLOG_LOGGER_INFO(m_Logger, "Sending RenewLease request for lease '{}'", leaseId.Value);
        return ReadLeaseResult(m_Stub->RenewLease(&context, protoRequest, &response), response);
    }

    Error::Api::Result<void> Client::ReleaseLease(const Api::LeaseId& leaseId)
    {
        ::grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + m_Channel.GetRpcTimeout());

        ::pisubmarine::lease::grpc::api::ReleaseLeaseRequest protoRequest;
        protoRequest.set_lease_id(leaseId.Value);
        ::pisubmarine::lease::grpc::api::VoidResult response;

        SPDLOG_LOGGER_INFO(m_Logger, "Sending ReleaseLease request for lease '{}'", leaseId.Value);
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
                SPDLOG_LOGGER_WARN(m_Logger, "Lease RPC failed due to authentication error: {}", status.error_message());
                return std::unexpected(MakeCommunicationError(ErrorCode::Unauthenticated));
            }

            SPDLOG_LOGGER_WARN(m_Logger, "Lease RPC failed: {}", status.error_message());
            return std::unexpected(MakeCommunicationError(ErrorCode::RpcFailed));
        }

        switch (response.value_case())
        {
        case ::pisubmarine::lease::grpc::api::LeaseResult::kLease:
            SPDLOG_LOGGER_INFO(
                m_Logger,
                "Lease RPC returned lease '{}' for resource '{}'",
                response.lease().id(),
                response.lease().resource());
            return FromProtoLease(response.lease());
        case ::pisubmarine::lease::grpc::api::LeaseResult::kError:
            SPDLOG_LOGGER_WARN(m_Logger, "Lease RPC returned domain error code {}", response.error().lease_error_code());
            return std::unexpected(FromProtoError(response.error()));
        case ::pisubmarine::lease::grpc::api::LeaseResult::VALUE_NOT_SET:
            SPDLOG_LOGGER_ERROR(m_Logger, "Lease RPC violated protocol: response payload is empty");
            return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
        }

        SPDLOG_LOGGER_ERROR(m_Logger, "Lease RPC violated protocol: unknown response variant");
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
                SPDLOG_LOGGER_WARN(m_Logger, "Void lease RPC failed due to authentication error: {}", status.error_message());
                return std::unexpected(MakeCommunicationError(ErrorCode::Unauthenticated));
            }

            SPDLOG_LOGGER_WARN(m_Logger, "Void lease RPC failed: {}", status.error_message());
            return std::unexpected(MakeCommunicationError(ErrorCode::RpcFailed));
        }

        switch (response.value_case())
        {
        case ::pisubmarine::lease::grpc::api::VoidResult::kSuccess:
            SPDLOG_LOGGER_INFO(m_Logger, "Void lease RPC succeeded");
            return {};
        case ::pisubmarine::lease::grpc::api::VoidResult::kError:
            SPDLOG_LOGGER_WARN(m_Logger, "Void lease RPC returned domain error code {}", response.error().lease_error_code());
            return std::unexpected(FromProtoError(response.error()));
        case ::pisubmarine::lease::grpc::api::VoidResult::VALUE_NOT_SET:
            SPDLOG_LOGGER_ERROR(m_Logger, "Void lease RPC violated protocol: response payload is empty");
            return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
        }

        SPDLOG_LOGGER_ERROR(m_Logger, "Void lease RPC violated protocol: unknown response variant");
        return std::unexpected(MakeContractError(ErrorCode::ProtocolViolation));
    }
}
