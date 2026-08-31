#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include "timerwheel.h"

// ============================================================
// TimeTask 测试
// ============================================================

// 构造函数测试
TEST(TimeTaskTest, Construct)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    EXPECT_EQ(tt.GetId(), 1);
}

TEST(TimeTaskTest, ConstructWithDifferentIds)
{
    auto noop = []() {};

    cpp_toolkit::TimeTask tt1(0, noop);
    cpp_toolkit::TimeTask tt2(999, noop);
    cpp_toolkit::TimeTask tt3(-1, noop);

    EXPECT_EQ(tt1.GetId(), 0);
    EXPECT_EQ(tt2.GetId(), 999);
    EXPECT_EQ(tt3.GetId(), -1);
}

// Handle 测试 - 未取消时执行任务
TEST(TimeTaskTest, HandleExecutesTask)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    EXPECT_EQ(counter, 0);
    tt.Handle();
    EXPECT_EQ(counter, 1);
}

TEST(TimeTaskTest, HandleMultipleTimes)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    tt.Handle();
    tt.Handle();
    tt.Handle();

    EXPECT_EQ(counter, 3);
}

TEST(TimeTaskTest, HandleExecutesCorrectTask)
{
    std::vector<int> execution_order;

    cpp_toolkit::TimeTask tt1(1, [&execution_order]() { execution_order.push_back(1); });
    cpp_toolkit::TimeTask tt2(2, [&execution_order]() { execution_order.push_back(2); });
    cpp_toolkit::TimeTask tt3(3, [&execution_order]() { execution_order.push_back(3); });

    tt2.Handle();
    tt1.Handle();
    tt3.Handle();

    std::vector<int> expected = {2, 1, 3};
    EXPECT_EQ(execution_order, expected);
}

// Cancel 测试
TEST(TimeTaskTest, CancelPreventsExecution)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    tt.Cancel();
    tt.Handle();

    EXPECT_EQ(counter, 0);
}

TEST(TimeTaskTest, CancelAfterHandleStillAllowsPrevious)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    tt.Handle();
    EXPECT_EQ(counter, 1);

    tt.Cancel();
    tt.Handle();
    EXPECT_EQ(counter, 1);  // 不应该增加
}

TEST(TimeTaskTest, HandleAfterCancelNoop)
{
    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    cpp_toolkit::TimeTask tt(1, task);

    tt.Cancel();
    tt.Handle();
    tt.Handle();
    tt.Handle();

    EXPECT_EQ(counter, 0);
}

// ============================================================
// SingleLayerTimerWheel 测试
// ============================================================

// 构造函数测试
TEST(SingleLayerTimerWheelTest, Construct)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);
    // 构造不应崩溃
    SUCCEED();
}

TEST(SingleLayerTimerWheelTest, ConstructWithDifferentLengths)
{
    cpp_toolkit::SingleLayerTimerWheel wheel1(1);
    cpp_toolkit::SingleLayerTimerWheel wheel2(60);
    cpp_toolkit::SingleLayerTimerWheel wheel3(365);

    // 都不应崩溃
    SUCCEED();
}

// AddTask 和 HasTask 测试
TEST(SingleLayerTimerWheelTest, AddTaskAndHasTask)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    wheel.AddTask(1, 5, []() {});

    EXPECT_TRUE(wheel.HasTask(1));
    EXPECT_FALSE(wheel.HasTask(2));
}

TEST(SingleLayerTimerWheelTest, AddMultipleTasks)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    wheel.AddTask(1, 1, []() {});
    wheel.AddTask(2, 2, []() {});
    wheel.AddTask(3, 3, []() {});

    EXPECT_TRUE(wheel.HasTask(1));
    EXPECT_TRUE(wheel.HasTask(2));
    EXPECT_TRUE(wheel.HasTask(3));
    EXPECT_FALSE(wheel.HasTask(4));
}

TEST(SingleLayerTimerWheelTest, AddTaskWithSameIdOverwrites)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    std::atomic<int> counter{0};
    wheel.AddTask(1, 1, [&counter]() { counter++; });

    // 添加相同 ID 的任务应该覆盖（根据实现，会添加新任务）
    wheel.AddTask(1, 2, [&counter]() { counter += 10; });

    EXPECT_TRUE(wheel.HasTask(1));
}

// RemoveTask 测试
TEST(SingleLayerTimerWheelTest, RemoveTask)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    wheel.AddTask(1, 5, []() {});
    EXPECT_TRUE(wheel.HasTask(1));

    wheel.RemoveTask(1);
    EXPECT_FALSE(wheel.HasTask(1));
}

TEST(SingleLayerTimerWheelTest, RemoveNonExistentTask)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    // 移除不存在的任务不应崩溃
    wheel.RemoveTask(999);
    SUCCEED();
}

TEST(SingleLayerTimerWheelTest, RemoveTaskTwice)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    wheel.AddTask(1, 5, []() {});
    wheel.RemoveTask(1);

    // 再次移除不应崩溃
    wheel.RemoveTask(1);
    EXPECT_FALSE(wheel.HasTask(1));
}

TEST(SingleLayerTimerWheelTest, RemoveAndAddSameId)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    std::atomic<int> counter{0};

    wheel.AddTask(1, 1, [&counter]() { counter++; });
    wheel.RemoveTask(1);

    EXPECT_FALSE(wheel.HasTask(1));

    // 重新添加相同 ID
    wheel.AddTask(1, 2, [&counter]() { counter += 10; });
    EXPECT_TRUE(wheel.HasTask(1));
}

// 多个任务的不同超时测试
TEST(SingleLayerTimerWheelTest, TasksWithDifferentTimeouts)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    wheel.AddTask(1, 1, []() {});
    wheel.AddTask(2, 5, []() {});
    wheel.AddTask(3, 9, []() {});

    EXPECT_TRUE(wheel.HasTask(1));
    EXPECT_TRUE(wheel.HasTask(2));
    EXPECT_TRUE(wheel.HasTask(3));
}

TEST(SingleLayerTimerWheelTest, TaskTimeoutWrapsAround)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(10);

    // timeout 超过 wheel 长度时应该取模
    wheel.AddTask(1, 15, []() {});  // 15 % 10 = 5
    wheel.AddTask(2, 25, []() {});  // 25 % 10 = 5

    EXPECT_TRUE(wheel.HasTask(1));
    EXPECT_TRUE(wheel.HasTask(2));
}

// ============================================================
// MultiLayerTimerWheel 测试
// ============================================================

// 构造函数测试
TEST(MultiLayerTimerWheelTest, Construct)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;
    // 构造不应崩溃
    SUCCEED();
}

// AddTask 测试
TEST(MultiLayerTimerWheelTest, AddTaskReturnsId)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    int id1 = wheel.AddTask([]() {}, 0, 0, 0, 1);
    int id2 = wheel.AddTask([]() {}, 0, 0, 0, 2);

    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_NE(id1, id2);
}

TEST(MultiLayerTimerWheelTest, AddTaskWithSecondsOnly)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 5);

    EXPECT_GE(id, 0);
}

TEST(MultiLayerTimerWheelTest, AddTaskWithMinutesAndSeconds)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 0, 0, 2, 30);

    EXPECT_GE(id, 0);
}

TEST(MultiLayerTimerWheelTest, AddTaskWithHoursMinutesSeconds)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 0, 1, 30, 0);

    EXPECT_GE(id, 0);
}

TEST(MultiLayerTimerWheelTest, AddTaskWithAllTimeUnits)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 1, 2, 3, 4);

    EXPECT_GE(id, 0);
}

TEST(MultiLayerTimerWheelTest, AddTaskTimeNormalization)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 测试时间进位: 90 seconds = 1 minute 30 seconds
    int id1 = wheel.AddTask([]() {}, 0, 0, 0, 90);
    EXPECT_GE(id1, 0);

    // 测试时间进位: 120 minutes = 2 hours 0 minutes
    int id2 = wheel.AddTask([]() {}, 0, 0, 120, 0);
    EXPECT_GE(id2, 0);

    // 测试时间进位: 25 hours = 1 day 1 hour
    int id3 = wheel.AddTask([]() {}, 0, 25, 0, 0);
    EXPECT_GE(id3, 0);

    // 测试跨层进位: 59 minutes + 120 seconds = 1 hour 1 minute
    int id4 = wheel.AddTask([]() {}, 0, 0, 59, 120);
    EXPECT_GE(id4, 0);

    // 超过 365 天容量, 应该被拒绝
    int id5 = wheel.AddTask([]() {}, 400, 0, 0, 0);
    EXPECT_EQ(id5, -1);
}

TEST(MultiLayerTimerWheelTest, AddTaskWithZeroTimeout)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 全零超时无法表达有效时长, 应该被拒绝
    int id = wheel.AddTask([]() {}, 0, 0, 0, 0);
    EXPECT_EQ(id, -1);
}

TEST(MultiLayerTimerWheelTest, AddMultipleTasks)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::vector<int> ids;
    for (int i = 0; i < 10; i++)
    {
        int id = wheel.AddTask([]() {}, 0, 0, 0, i + 1);
        ids.push_back(id);
    }

    // 所有 ID 应该唯一
    std::sort(ids.begin(), ids.end());
    auto last = std::unique(ids.begin(), ids.end());
    EXPECT_EQ(last, ids.end());
}

// RemoveTask 测试
TEST(MultiLayerTimerWheelTest, RemoveTask)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    int id = wheel.AddTask([]() {}, 0, 0, 0, 5);
    // 移除不应崩溃
    wheel.RemoveTask(id);
    SUCCEED();
}

TEST(MultiLayerTimerWheelTest, RemoveNonExistentTask)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 移除不存在的任务不应崩溃
    wheel.RemoveTask(999);
    SUCCEED();
}

TEST(MultiLayerTimerWheelTest, RemoveTaskTwice)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    int id = wheel.AddTask([]() {}, 0, 0, 0, 5);
    wheel.RemoveTask(id);

    // 再次移除不应崩溃
    wheel.RemoveTask(id);
    SUCCEED();
}

// DelayTask 测试
TEST(MultiLayerTimerWheelTest, DelayTask)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 5);

    // 延迟任务不应崩溃
    wheel.DelayTask(id);
    SUCCEED();
}

TEST(MultiLayerTimerWheelTest, DelayNonExistentTask)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 延迟不存在的任务不应崩溃
    wheel.DelayTask(999);
    SUCCEED();
}

// 综合测试
TEST(MultiLayerTimerWheelTest, AddRemoveAddSequence)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    auto task = [&counter]() { counter++; };

    int id1 = wheel.AddTask(task, 0, 0, 0, 1);
    wheel.RemoveTask(id1);

    int id2 = wheel.AddTask(task, 0, 0, 0, 2);
    EXPECT_NE(id1, id2);
}

TEST(MultiLayerTimerWheelTest, MultipleTaskTypes)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};
    std::atomic<int> counter3{0};

    int id1 = wheel.AddTask([&counter1]() { counter1++; }, 0, 0, 0, 1);
    int id2 = wheel.AddTask([&counter2]() { counter2++; }, 0, 0, 1, 0);
    int id3 = wheel.AddTask([&counter3]() { counter3++; }, 0, 1, 0, 0);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

// ============================================================
// 边界条件测试
// ============================================================

TEST(TimeTaskTest, TaskWithException)
{
    auto task = []() { throw std::runtime_error("test exception"); };
    cpp_toolkit::TimeTask tt(1, task);

    // Handle 应该传播异常
    EXPECT_THROW(tt.Handle(), std::runtime_error);
}

TEST(TimeTaskTest, TaskWithLargeId)
{
    auto noop = []() {};
    cpp_toolkit::TimeTask tt(1000000, noop);

    EXPECT_EQ(tt.GetId(), 1000000);
}

TEST(TimeTaskTest, TaskWithNegativeId)
{
    auto noop = []() {};
    cpp_toolkit::TimeTask tt(-100, noop);

    EXPECT_EQ(tt.GetId(), -100);
}

TEST(SingleLayerTimerWheelTest, LargeWheelLength)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(1000);

    wheel.AddTask(1, 500, []() {});
    EXPECT_TRUE(wheel.HasTask(1));
}

TEST(SingleLayerTimerWheelTest, SmallWheelLength)
{
    cpp_toolkit::SingleLayerTimerWheel wheel(1);

    // timeout 必须 > 0
    wheel.AddTask(1, 1, []() {});
    EXPECT_TRUE(wheel.HasTask(1));
    EXPECT_FALSE(wheel.HasTask(2));
}

TEST(MultiLayerTimerWheelTest, LargeTimeValues)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 容量上限: 364 days, 23 hours, 59 minutes, 59 seconds
    int id = wheel.AddTask([]() {}, 364, 23, 59, 59);
    EXPECT_GE(id, 0);
}

TEST(MultiLayerTimerWheelTest, OverflowTimeValues)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 超过 365 天容量的值应该被拒绝
    int id1 = wheel.AddTask([]() {}, 1000, 100, 100, 100);
    EXPECT_EQ(id1, -1);

    // 365 天整也超出 (0, 365天) 的开区间
    int id2 = wheel.AddTask([]() {}, 365, 0, 0, 0);
    EXPECT_EQ(id2, -1);
}

TEST(MultiLayerTimerWheelTest, TaskLongerThanOneMinute)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 65 秒 = 1 分 5 秒，归一化后应在分钟层
    int id1 = wheel.AddTask([]() {}, 0, 0, 1, 5);
    EXPECT_GE(id1, 0);

    // 125 秒 = 2 分 5 秒
    int id2 = wheel.AddTask([]() {}, 0, 0, 2, 5);
    EXPECT_GE(id2, 0);

    // 3661 秒 = 1 小时 1 分 1 秒
    int id3 = wheel.AddTask([]() {}, 0, 1, 1, 1);
    EXPECT_GE(id3, 0);

    // 90061 秒 = 1 天 1 小时 1 分 1 秒
    int id4 = wheel.AddTask([]() {}, 1, 1, 1, 1);
    EXPECT_GE(id4, 0);
}

TEST(MultiLayerTimerWheelTest, TaskTimeNormalizationBoundary)
{
    cpp_toolkit::MultiLayerTimerWheel wheel;

    // 刚好 60 秒 = 1 分 0 秒
    int id1 = wheel.AddTask([]() {}, 0, 0, 0, 60);
    EXPECT_GE(id1, 0);

    // 刚好 3600 秒 = 1 小时 0 分 0 秒
    int id2 = wheel.AddTask([]() {}, 0, 0, 60, 0);
    EXPECT_GE(id2, 0);

    // 刚好 86400 秒 = 1 天 0 小时 0 分 0 秒
    int id3 = wheel.AddTask([]() {}, 0, 24, 0, 0);
    EXPECT_GE(id3, 0);
}

// ============================================================
// 集成测试：测试定时任务是否真正执行
// 注意：
//   - 这些测试会等待真实时间，运行较慢
//   - timeout=N 的任务放在 tick index N，实际在第 N+1 次 tick 时执行
//   - 秒层每 1 秒 tick 一次，所以 timeout=1 的任务约 2 秒后执行
//   - 每个测试结束后析构 4 个 timer wheel 线程需要额外等待时间
// ============================================================

TEST(MultiLayerTimerWheelIntegrationTest, TaskExecutesAfterTimeout)
{
    // timeout=1 → 放在 index 1 → 第 2 次 tick 时执行 → 约 2 秒
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 1);

    wheel.Start();

    // 等待足够长以确保任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    EXPECT_GT(counter, 0);
}

TEST(MultiLayerTimerWheelIntegrationTest, RemovedTaskNotExecuted)
{
    // 移除的任务不应该被执行
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    int id = wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 1);

    wheel.RemoveTask(id);

    wheel.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    EXPECT_EQ(counter, 0);
}

TEST(MultiLayerTimerWheelIntegrationTest, MultipleTasksExecution)
{
    // 两个 timeout=1 的任务都应该在约 2 秒后执行
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};

    wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 1);
    wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 1);

    wheel.Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    EXPECT_GE(counter, 2);
}

TEST(MultiLayerTimerWheelIntegrationTest, TaskWithLongerTimeout)
{
    // timeout=2 → 放在 index 2 → 第 3 次 tick 时执行
    // 由于线程调度和 epoll 行为，精确时间不可预测，只验证最终执行
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    wheel.AddTask([&counter]() { counter++; }, 0, 0, 0, 2);

    wheel.Start();

    // 等待足够长时间，确保 timeout=2 的任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    EXPECT_GT(counter, 0);
}

// ============================================================
// 集成测试：超过一分钟的定时任务
// 注意：此测试需要等待约 2 分钟，运行较慢
// 运行方式: ./all_test --gtest_filter="*TaskLongerThanOneMinute*"
// ============================================================

TEST(MultiLayerTimerWheelIntegrationTest, TaskLongerThanOneMinute)
{
    // 65 秒 = 1 分 5 秒
    // 归一化: timeouts = {0, 0, 1, 5}
    // 首先添加到分钟层 (timeout=1)
    // 分钟层定时器每 60 秒触发一次，触发后降级到秒层 (timeout=5)
    // 秒层再等 5 秒后执行
    // 预计执行时间: ~65 秒
    cpp_toolkit::MultiLayerTimerWheel wheel;

    std::atomic<int> counter{0};
    wheel.AddTask([&counter]() { counter++; }, 0, 0, 1, 5);

    wheel.Start();

    // 等待足够长时间，确保任务执行 (65 秒 + 缓冲时间)
    std::this_thread::sleep_for(std::chrono::milliseconds(120000));

    EXPECT_GT(counter, 0);
}
