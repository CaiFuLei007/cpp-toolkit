
#include "timerwheel.h"

#include <sys/timerfd.h>
#include <cerrno>
#include <iostream>
#include <unistd.h>

namespace cfl
{

// =============== TimeTask ===============

TimeTask::TimeTask(int task_id, int timeout, Task task)
: task_id_(task_id), timeout_(timeout), task_(std::move(task)), is_cancel_(false)
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

SingleLayerTimerWheel::~SingleLayerTimerWheel()
{
    // 先停止线程，再关闭文件描述符
    if(th_.joinable())
    {
        th_.request_stop();
        th_.join();
    }
    if(timer_fd_ >= 0)
        close(timer_fd_);
}


void SingleLayerTimerWheel::HandleRead()
{
    // 处理 timerfd 的读事件
    int64_t expirations;
    int ret = read(timer_fd_, &expirations, sizeof(expirations));
    if(ret < 0)
    {
        // 非阻塞模式下，没有数据可读时返回 EAGAIN 是正常的
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << ("TIMERFD READ FAIL");
        return ;
    }
    for(int i = tick_ ; i <= tick_ + expirations ; ++i)
    {
        int index = i % tasks_.size();
        for(auto& task_ptr : tasks_[index])
        {
            if(task_ptr)
            {
                task_ptr->Handle();
                task_info_map_.erase(task_ptr->GetId());
            }
        }
        tasks_[index].clear();
    }
    tick_ += expirations;
}


int SingleLayerTimerWheel::CreateTimer()
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if(fd < 0)
    {
        std::cerr << ("TIMERFD CREATE FAIL");
        return -1;
    }
    return fd;
}

SingleLayerTimerWheel::SingleLayerTimerWheel(int length)
: tick_(0), timer_fd_(CreateTimer()), tasks_(length), task_info_map_()
{}

void SingleLayerTimerWheel::AddTask(int task_id , int timeout,std::function<void()> task)
{
    STaskPtr task_ptr = std::make_shared<TimeTask>(task_id, timeout, std::move(task));
    int index = (timeout + tick_) % tasks_.size();
    tasks_[index].emplace_back(std::move(task_ptr));
    task_info_map_[task_id] = std::make_pair(index , tasks_[index].size() - 1);
}

void SingleLayerTimerWheel::RemoveTask(int task_id)
{
    auto it = task_info_map_.find(task_id);
    if (it != task_info_map_.end())
    {
        auto [index, pos] = it->second;
        tasks_[index][pos] = nullptr;
        task_info_map_.erase(it);
    }
}

std::shared_ptr<TimeTask> SingleLayerTimerWheel::GetTask(int task_id)
{
    auto it = task_info_map_.find(task_id);
    if (it != task_info_map_.end())
    {
        auto [index, pos] = it->second;
        if (pos < tasks_[index].size())
        {
            return tasks_[index][pos];
        }
    }
    return nullptr;
}

bool SingleLayerTimerWheel::HasTask(int task_id) const
{
    return task_info_map_.find(task_id) != task_info_map_.end();
}

void SingleLayerTimerWheel::Start(int ftimeout , int stimeout)
{
    // 设置定时器
    itimerspec new_value;
    new_value.it_value.tv_sec = ftimeout;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = stimeout;
    new_value.it_interval.tv_nsec = 0;
    int ret = timerfd_settime(timer_fd_, 0, &new_value, NULL);
    if (ret) {
        std::cerr << ("TIMER SETTIME FAIL");
        return;
    }

    th_ = std::jthread([this](std::stop_token stoken)
    {
        epoll_.AddFd(timer_fd_, EPOLLIN);
        while (!stoken.stop_requested())
        {
            std::vector<struct epoll_event> events(1);
            epoll_.Wait(events , 3000);
            HandleRead();
        }
    });
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

MultiLayerTimerWheel::~MultiLayerTimerWheel() = default;

void MultiLayerTimerWheel::HandleTask(int task_id, int level, std::function<void()> task)
{
    std::function<void()> task_to_run;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(task_timeouts_.find(task_id) == task_timeouts_.end())
        return;  // 任务已被移除
        
        // 如果是最后一层就处理定时任务
        // 如果不是最后一层, 将任务交给下一层
        if(level == timerwheels_.size() - 1)
        {
            task_to_run = task;
            task_timeouts_.erase(task_id);
        }
        else
        {
            int timeout = task_timeouts_[task_id][level + 1];
            timerwheels_[level + 1].AddTask(task_id , timeout, [task_id , level , task , this](){
                HandleTask(task_id , level + 1 , task);
            });
        }
    }
    if(task_to_run)
        task_to_run();
    
}

int MultiLayerTimerWheel::AddTask(std::function<void()> task, int days, int hours, int minutes, int seconds)
{
    // 先对数据进行预处理
    // 将 days , hours , minutes , seconds 在合法范围内

    std::vector<int> timeouts = {
        (days + hours/24) % 365 , 
        (hours + minutes/60) % 24 , 
        (minutes + seconds/60) % 60 , 
        seconds % 60
    };

    int task_id = task_id_++;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        task_timeouts_.emplace(task_id, timeouts);
    }

    for(int i = 0 ; i < timeouts.size() ; i++)
    {
        if(timeouts[i] > 0)
        {
            timerwheels_[i].AddTask(task_id, timeouts[i], [i, task, task_id, this](){
                HandleTask(task_id, i , task);
            });
            break;
        }
    }
    return task_id;
}

void MultiLayerTimerWheel::RemoveTask(int task_id)
{
    for (auto& wheel : timerwheels_)
    {
        if(wheel.HasTask(task_id))
        {
            wheel.RemoveTask(task_id);
            std::lock_guard<std::mutex> lock(mutex_);
            task_timeouts_.erase(task_id);
        }
    }
}

void MultiLayerTimerWheel::DelayTask(int task_id)
{
    for (auto& wheel : timerwheels_)
    {
        if(wheel.HasTask(task_id))
        {
            auto task_ptr = wheel.GetTask(task_id);
            std::vector<int> timeouts;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                timeouts = task_timeouts_[task_id];
            }
            std::function<void()> task = task_ptr->GetTask();
            wheel.RemoveTask(task_id);

            AddTask(task, timeouts[0], timeouts[1], timeouts[2], timeouts[3]);
        }
    }
}

void MultiLayerTimerWheel::Start()
{
    timerwheels_[0].Start(86400  , 86400);
    timerwheels_[1].Start(3600 , 3600);
    timerwheels_[2].Start(60 , 60);
    timerwheels_[3].Start(1 , 1);
}

} // namespace cfl