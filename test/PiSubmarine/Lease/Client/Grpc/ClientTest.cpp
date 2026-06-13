#include <gtest/gtest.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>

#include "PiSubmarine/Grpc/Client/Channel.h"
#include "PiSubmarine/Lease/Client/Grpc/Client.h"
#include "PiSubmarine/Logging/Api/IFactory.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    namespace
    {
        class LoggerFactoryStub final : public Logging::Api::IFactory
        {
        public:
            [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(std::string_view name) override
            {
                return std::make_shared<spdlog::logger>(
                    std::string(name),
                    std::make_shared<spdlog::sinks::null_sink_mt>());
            }
        };

        class NullLoggerFactory final : public Logging::Api::IFactory
        {
        public:
            [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger(std::string_view) override
            {
                return {};
            }
        };

        [[nodiscard]] ::PiSubmarine::Grpc::Client::TlsConfig MakeTlsConfig()
        {
            return ::PiSubmarine::Grpc::Client::TlsConfig{
                .Target = "127.0.0.1:50051",
                .CertificateAuthority = "ca",
                .ClientCertificateChain = "cert",
                .ClientPrivateKey = "key"};
        }
    }

    TEST(ClientTest, RequiresLoggerFactoryToReturnLogger)
    {
        LoggerFactoryStub channelLoggerFactory;
        auto channel = ::PiSubmarine::Grpc::Client::Channel(channelLoggerFactory, MakeTlsConfig());
        NullLoggerFactory loggerFactory;

        EXPECT_THROW((Client(loggerFactory, channel)), std::invalid_argument);
    }

    TEST(ClientTest, ConstructsWithSharedChannel)
    {
        LoggerFactoryStub channelLoggerFactory;
        auto channel = ::PiSubmarine::Grpc::Client::Channel(channelLoggerFactory, MakeTlsConfig());
        LoggerFactoryStub loggerFactory;

        EXPECT_NO_THROW((Client(loggerFactory, channel)));
    }
}
