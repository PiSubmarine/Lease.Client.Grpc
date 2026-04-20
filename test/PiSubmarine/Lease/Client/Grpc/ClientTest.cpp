#include <gtest/gtest.h>

#include "PiSubmarine/Lease/Client/Grpc/Client.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    TEST(ClientTest, RequiresTlsConfiguration)
    {
        EXPECT_THROW((Client(TlsConfig{})), std::invalid_argument);
    }
}
