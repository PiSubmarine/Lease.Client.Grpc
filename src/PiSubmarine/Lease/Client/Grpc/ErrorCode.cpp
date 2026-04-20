#include "PiSubmarine/Lease/Client/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    namespace
    {
        class ErrorCategory final : public std::error_category
        {
        public:
            [[nodiscard]] const char* name() const noexcept override
            {
                return "PiSubmarine.Lease.Client.Grpc";
            }

            [[nodiscard]] std::string message(const int condition) const override
            {
                switch (static_cast<ErrorCode>(condition))
                {
                case ErrorCode::InvalidTlsConfiguration:
                    return "gRPC lease client TLS configuration is invalid";
                case ErrorCode::RpcFailed:
                    return "gRPC lease client call failed";
                case ErrorCode::Unauthenticated:
                    return "gRPC lease client authentication failed";
                case ErrorCode::ProtocolViolation:
                    return "gRPC lease client received an invalid response";
                }

                return "unknown lease client grpc error";
            }
        };
    }

    std::error_code make_error_code(const ErrorCode errorCode) noexcept
    {
        static const ErrorCategory Category;
        return {static_cast<int>(errorCode), Category};
    }
}
