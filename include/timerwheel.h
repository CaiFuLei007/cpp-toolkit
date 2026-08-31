
#pragma once

/**
 * 实现时间轮定时器
 *
 * 多层时间轮定时器 , 实现 秒级定时器
 *
 * 线程模型 :
 *  - SingleLayerTimerWheel 内部有独立的 tick 线程 , 所有公开接口线程安全
 *  - 任务触发在该轮的 tick 线程中执行 , 回调应尽量简短 ,
 *    长耗时回调会阻塞对应层的 tick ( MultiLayerTimerWheel 的级联也依赖它 )
*/

#include "epoll.h"

#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

namespace cpp_toolkit
{

class TimeTask
{
    using Task = std::function<void()>;
private:
    int task_id_;
    Task task_;
    std::atomic<bool> is_cancel_;
public:
    TimeTask(int task_id , Task task);
    ~TimeTask() = default;

    int GetId() const
    {
        return task_id_;
    }

    void Cancel();
    void Handle();
};

class SingleLayerTimerWheel
{
    using STaskPtr = std::shared_ptr<TimeTask>;
private:
    int tick_;                                      // 已处理的 tick 数 ( 始终保持在 [0, size) )
    int timer_fd_;
    std::vector<std::vector<STaskPtr>> tasks_;      // 槽位 : 只追加 + 整槽清空
    std::unordered_map<int , STaskPtr> task_info_map_;  // task_id -> 任务

    mutable std::mutex mtx_;                        // 保护 tick_ / tasks_ / task_info_map_
    Epoll epoll_;
    std::jthread th_;
private:
    int CreateTimer();
    void HandleRead();

public:
    SingleLayerTimerWheel(int length);
    ~SingleLayerTimerWheel();

    // timeout 必须 > 0 ( 单位 : 本轮的 tick ); 允许超过轮长 , 触发时刻仍按 tick 精确对齐,
    // 但超过轮长的任务可能与其它任务落入同一槽位 , 不保证只触发一次 , 应避免
    void AddTask(int task_id , int timeout , std::function<void()> task);
    void RemoveTask(int task_id);

    bool HasTask(int task_id) const;

    void Start(int ftimeout , int stimeout);
    void Stop();
};

class MultiLayerTimerWheel
{
private:
    // mutex_ 声明在最前 , 保证最后析构 ; 析构函数中仍会先显式停掉各层轮线程
    std::mutex mutex_;                              // 保护 task_id_ / task_timeouts_ / task_funcs_
    int task_id_;
    std::deque<SingleLayerTimerWheel> timerwheels_;
    std::unordered_map<int , std::vector<int> > task_timeouts_;     // task_id -> 各层超时
    std::unordered_map<int , std::function<void()> > task_funcs_;   // task_id -> 用户任务
private:
    void HandleTask(int task_id, int level , std::function<void()> task);
    bool Schedule(int task_id , const std::vector<int>& timeouts , std::function<void()> task);

public:
    MultiLayerTimerWheel();
    ~MultiLayerTimerWheel();

    // 总时长必须在 (0, 365天) 区间内 , 否则返回 -1
    // 参数允许任意进位 ( 如 seconds=90 会归一化为 1分30秒 )
    int AddTask(std::function<void()> task , int days , int hours , int minutes , int seconds);
    bool RemoveTask(int task_id);
    // 重新以注册时的完整时长倒计时 , task_id 保持不变 ; 任务不存在时返回 false
    bool DelayTask(int task_id);

    void Start();
};

} // namespace cpp_toolkit
