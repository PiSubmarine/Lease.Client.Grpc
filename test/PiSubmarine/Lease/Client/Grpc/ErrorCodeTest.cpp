#include <gtest/gtest.h>

#include "PiSubmarine/Lease/Client/Grpc/ErrorCode.h"

namespace PiSubmarine::Lease::Client::Grpc
{
    TEST(ErrorCodeTest, ReportsReadableMessage)
    {
        EXPECT_EQ(make_error_code(ErrorCode::Unauthenticated).message(),
                  "gRPC lease client authentication failed");
    }
}
