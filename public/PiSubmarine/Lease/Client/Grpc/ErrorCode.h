#pragma once

#include <system_error>

namespace PiSubmarine::Lease::Client::Grpc
{
    enum class ErrorCode
    {
        InvalidTlsConfiguration = 1,
        RpcFailed,
        Unauthenticated,
        ProtocolViolation
    };

    [[nodiscard]] std::error_code make_error_code(ErrorCode errorCode) noexcept;
}

namespace std
{
    template<>
    struct is_error_code_enum<PiSubmarine::Lease::Client::Grpc::ErrorCode> : true_type
    {
    };
}
