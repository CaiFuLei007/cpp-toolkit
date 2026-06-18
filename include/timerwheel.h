
#pragma once

/**
 * 实现时间轮定时器
 * 
 * 多层时间轮定时器 , 实现 秒级定时器
*/

#include "epoll.h"

#include <vector>
#include <deque>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace cfl
{

class TimeTask
{
    using Task = std::function<void()>;
private:
    int task_id_;
    int timeout_;

    Task task_;
    std::atomic<bool> is_cancel_;
public:
    TimeTask(int task_id , int timeout , Task task);
    ~TimeTask() = default;

    int TimeOut()
    {
        return timeout_;
    }

    Task GetTask()
    {
        return task_;
    }

    int GetId()
    {
        return task_id_;
    }
    
    void Cancel();
    void Handle();
};

class SingleLayerTimerWheel
{
    using STaskPtr = std::shared_ptr<TimeTask>;
    using WTaskPtr = std::weak_ptr<TimeTask>;
private:
    int tick_;
    int timer_fd_;
    std::vector<std::vector<STaskPtr>> tasks_;
    std::unordered_map<int , std::pair<int,int> > task_info_map_;   

    Epoll epoll_;
    std::jthread th_;
private:
    int CreateTimer();
    void HandleRead(); 

public:
    SingleLayerTimerWheel(int length);
    ~SingleLayerTimerWheel();


    void AddTask(int task_id , int timeouts,std::function<void()> task);
    void RemoveTask(int task_id);

    std::shared_ptr<TimeTask> GetTask(int task_id);
    bool HasTask(int task_id) const;

    void Start(int ftimeout , int stimeout);
};

class MultiLayerTimerWheel
{
private:
    int task_id_;
    std::deque<SingleLayerTimerWheel> timerwheels_;
    std::unordered_map<int , std::vector<int> > task_timeouts_;

    std::mutex mutex_;
private:
    void HandleTask(int task_id, int level , std::function<void()> task);

public:
    MultiLayerTimerWheel();
    ~MultiLayerTimerWheel();
    int AddTask(std::function<void()> task , int days , int hours , int minutes , int seconds);
    void RemoveTask(int task_id);
    void DelayTask(int task_id);

    void Start();
};

} // namespace cfl