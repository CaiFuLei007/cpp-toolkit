
#include "timerwheel.h"

#include <sys/timerfd.h>
#include <cstdint>
#include <cerrno>
#include <iostream>
#include <unistd.h>

namespace cpp_toolkit
{

// =============== TimeTask ===============

TimeTask::TimeTask(int task_id, Task task)
: task_id_(task_id), task_(std::move(task)), is_cancel_(false)
{}

void TimeTask::Cancel()
{
    is_cancel_ = true;
}

void TimeTask::Handle()
{
    if (!is_cancel_)
    {
        task_();
    }
}

// ================== SingleLayerTimerWheel ==================

int SingleLayerTimerWheel::CreateTimer()
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(fd < 0)
    {
        std::cerr << "TIMERFD CREATE FAIL" << std::endl;
    }
    return fd;
}

SingleLayerTimerWheel::SingleLayerTimerWheel(int length)
: tick_(0), timer_fd_(CreateTimer()), tasks_(length), task_info_map_()
{}

SingleLayerTimerWheel::~SingleLayerTimerWheel()
{
    // 先停止线程，再关闭文件描述符
    Stop();
    if(timer_fd_ >= 0)
        close(timer_fd_);
}

void SingleLayerTimerWheel::AddTask(int task_id , int timeout , std::function<void()> task)
{
    if(tasks_.empty() || timeout <= 0)
        return;

    STaskPtr task_ptr = std::make_shared<TimeTask>(task_id, std::move(task));
    std::lock_guard<std::mutex> lock(mtx_);

    // 相同 id 重复添加 : 取消旧任务, 保证一个 id 同时只有一个有效任务
    auto it = task_info_map_.find(task_id);
    if(it != task_info_map_.end())
        it->second->Cancel();

    int index = static_cast<int>((tick_ + timeout) % tasks_.size());
    tasks_[index].emplace_back(task_ptr);
    task_info_map_[task_id] = std::move(task_ptr);
}

void SingleLayerTimerWheel::RemoveTask(int task_id)
{
    // 通过 Cancel 标记取消, 槽位中残留的已取消任务在触发时会被 Handle() 跳过,
    // 无需维护槽内下标, 也避免了与触发线程之间的复杂竞争
    STaskPtr task_ptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = task_info_map_.find(task_id);
        if(it == task_info_map_.end())
            return;
        task_ptr = std::move(it->second);
        task_info_map_.erase(it);
    }
    task_ptr->Cancel();
}

bool SingleLayerTimerWheel::HasTask(int task_id) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return task_info_map_.find(task_id) != task_info_map_.end();
}

void SingleLayerTimerWheel::HandleRead()
{
    // 处理 timerfd 的读事件
    uint64_t expirations = 0;
    ssize_t ret = read(timer_fd_, &expirations, sizeof(expirations));
    if(ret != static_cast<ssize_t>(sizeof(expirations)))
    {
        // 非阻塞模式下，没有数据可读时返回 EAGAIN 是正常的
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "TIMERFD READ FAIL" << std::endl;
        return;
    }
    if(tasks_.empty())
        return;

    // 加锁收集到期任务, 在锁外执行用户回调,
    // 避免回调中再次操作时间轮导致死锁
    std::vector<STaskPtr> fired;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        // 每次过期处理下一个槽位, 处理区间为 (tick_, tick_ + expirations]
        for(uint64_t i = 0 ; i < expirations ; ++i)
        {
            int index = static_cast<int>((tick_ + 1 + i) % tasks_.size());
            for(auto& task_ptr : tasks_[index])
            {
                if(task_ptr)
                {
                    task_info_map_.erase(task_ptr->GetId());
                    fired.push_back(std::move(task_ptr));
                }
            }
            tasks_[index].clear();
        }
        // tick_ 始终保持在 [0, size), 避免溢出
        tick_ = static_cast<int>((tick_ + expirations) % tasks_.size());
    }
    for(auto& task_ptr : fired)
        task_ptr->Handle();
}


void SingleLayerTimerWheel::Start(int ftimeout , int stimeout)
{
    if(th_.joinable())
        return; // 已经启动

    // 设置定时器
    itimerspec new_value;
    new_value.it_value.tv_sec = ftimeout;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = stimeout;
    new_value.it_interval.tv_nsec = 0;
    int ret = timerfd_settime(timer_fd_, 0, &new_value, NULL);
    if (ret) {
        std::cerr << "TIMER SETTIME FAIL" << std::endl;
        return;
    }

    th_ = std::jthread([this](std::stop_token stoken)
    {
        if(!epoll_.AddFd(timer_fd_, EPOLLIN))
        {
            std::cerr << "EPOLL ADD TIMERFD FAIL" << std::endl;
            return;
        }
        while (!stoken.stop_requested())
        {
            std::vector<struct epoll_event> events(1);
            int n = epoll_.Wait(events , 3000);
            if(n > 0)
                HandleRead();
        }
    });
}

void SingleLayerTimerWheel::Stop()
{
    if(th_.joinable())
    {
        th_.request_stop();
        th_.join();
    }
}


// ============= MultiLayerTimerWheel =============

MultiLayerTimerWheel::MultiLayerTimerWheel()
:task_id_(0)
{
    // 初始化多层时间轮
    // 第一层表示天 : 365 天
    // 第二层表示小时 : 24 小时
    // 第三层表示分钟 : 60 分钟
    // 第四层表示秒 : 60s
    timerwheels_.emplace_back(365);
    timerwheels_.emplace_back(24);
    timerwheels_.emplace_back(60);
    timerwheels_.emplace_back(60);
}

MultiLayerTimerWheel::~MultiLayerTimerWheel()
{
    // 必须先停止各层轮线程, 否则线程可能在成员 ( 如 mutex_ ) 销毁后仍然访问它们
    for (auto& wheel : timerwheels_)
        wheel.Stop();
}

void MultiLayerTimerWheel::HandleTask(int task_id, int level, std::function<void()> task)
{
    std::function<void()> task_to_run;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = task_timeouts_.find(task_id);
        if(it == task_timeouts_.end())
            return;  // 任务已被移除

        // 跳过超时为 0 的层, 逐层向下级联; 到最后一层则执行任务
        const std::vector<int>& timeouts = it->second;
        int next = level + 1;
        while(next < static_cast<int>(timeouts.size()) && timeouts[next] == 0)
            ++next;

        if(next == static_cast<int>(timeouts.size()))
        {
            task_to_run = task;
            task_timeouts_.erase(it);
            task_funcs_.erase(task_id);
        }
        else
        {
            timerwheels_[next].AddTask(task_id , timeouts[next], [task_id , next , task , this](){
                HandleTask(task_id , next , task);
            });
        }
    }
    if(task_to_run)
        task_to_run();
}

bool MultiLayerTimerWheel::Schedule(int task_id , const std::vector<int>& timeouts , std::function<void()> task)
{
    // 从高粒度层到低粒度层, 挂载到第一个超时非零的层
    for(int i = 0 ; i < static_cast<int>(timeouts.size()) ; i++)
    {
        if(timeouts[i] > 0)
        {
            timerwheels_[i].AddTask(task_id, timeouts[i], [task_id, i, task, this](){
                HandleTask(task_id, i , task);
            });
            return true;
        }
    }
    return false;
}

int MultiLayerTimerWheel::AddTask(std::function<void()> task, int days, int hours, int minutes, int seconds)
{
    // 先统一换算成秒, 再反解到各层, 保证跨层进位正确
    long long total = ((static_cast<long long>(days) * 24 + hours) * 60 + minutes) * 60 + seconds;

    // 时间轮最大容量为第一层的 365 天, 非正数或超出容量都视为非法
    if(total <= 0 || total >= static_cast<long long>(365) * 86400)
        return -1;

    std::vector<int> timeouts = {
        static_cast<int>(total / 86400),
        static_cast<int>(total / 3600 % 24),
        static_cast<int>(total / 60 % 60),
        static_cast<int>(total % 60)
    };

    int task_id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_id = task_id_++;
        task_timeouts_.emplace(task_id, timeouts);
        task_funcs_.emplace(task_id, task);
    }
    Schedule(task_id, timeouts, std::move(task));
    return task_id;
}

bool MultiLayerTimerWheel::RemoveTask(int task_id)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 先注销, 正在级联中的任务会在下一次 HandleTask 检查时被中止
        if(task_timeouts_.erase(task_id) == 0)
            return false;
        task_funcs_.erase(task_id);
    }
    // 再逐层取消已挂载的任务
    for (auto& wheel : timerwheels_)
        wheel.RemoveTask(task_id);
    return true;
}

bool MultiLayerTimerWheel::DelayTask(int task_id)
{
    // 取出任务并注销, 再用原来的 task_id 和完整时长重新倒计时;
    // task_id 保持不变, 调用方之后仍可用它 RemoveTask / DelayTask
    std::function<void()> task;
    std::vector<int> timeouts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto to = task_timeouts_.find(task_id);
        auto fn = task_funcs_.find(task_id);
        if(to == task_timeouts_.end() || fn == task_funcs_.end())
            return false;
        timeouts = std::move(to->second);
        task = std::move(fn->second);
        task_timeouts_.erase(to);
        task_funcs_.erase(fn);
    }
    for (auto& wheel : timerwheels_)
        wheel.RemoveTask(task_id);

    Schedule(task_id, timeouts, std::move(task));
    return true;
}

void MultiLayerTimerWheel::Start()
{
    timerwheels_[0].Start(86400  , 86400);
    timerwheels_[1].Start(3600 , 3600);
    timerwheels_[2].Start(60 , 60);
    timerwheels_[3].Start(1 , 1);
}

} // namespace cpp_toolkit
