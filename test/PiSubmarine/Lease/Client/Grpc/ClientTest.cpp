#include <gtest/gtest.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>

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
    }

    TEST(ClientTest, RequiresTlsConfiguration)
    {
        LoggerFactoryStub loggerFactory;
        EXPECT_THROW((Client(loggerFactory, TlsConfig{})), std::invalid_argument);
    }
}
