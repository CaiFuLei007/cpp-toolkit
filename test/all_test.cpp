#include <gtest/gtest.h>
// #include "test_unique_fd.hpp"
#include "test_socket.hpp"

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
