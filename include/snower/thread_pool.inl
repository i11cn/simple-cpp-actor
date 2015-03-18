#ifndef __SNOWER_THREAD_POOL_INL__
#define __SNOWER_THREAD_POOL_INL__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace snower
{

class Priority {};
class Schedule {};
class Sequence {};

class AutoManage {};
class Fixed {};
class Infinity {};

namespace native
{

template<typename Func, typename Queue = std::vector<Func>>
class queue_wrapper_base
{
public:
    queue_wrapper_base(uint32_t threshold, uint32_t queue_limit)
    : m_threshold(threshold)
    , m_queue_limit(queue_limit)
    , m_idle_sec(30)
    {
    }
    void set_thread_idle_seconds(uint32_t seconds)
    {
        m_idle_sec = seconds;
    }
    size_t size(void) const
    {
        return m_queue.size();
    }
    bool empty(void) const
    {
        return m_queue.empty();
    }
    virtual void clear(void)
    {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_queue.clear();
        }
        m_signal.notify_all();
    }
    virtual void add(size_t key, Func&& func) = 0;
    virtual Func get(void) = 0;

    bool out_of_threshold(void) const
    {
        return (m_threshold > 0) ? size() >= m_threshold : false;
    }
    bool out_of_limit(void) const
    {
        return (m_queue_limit > 0) ? size() >= m_queue_limit : false;
    }

protected:
    void on_add_func(void)
    {
        m_signal.notify_one();
    }

protected:
    uint32_t m_threshold;
    uint32_t m_queue_limit;
    uint32_t m_idle_sec;

    Queue m_queue;
    std::mutex m_mutex;
    std::condition_variable_any m_signal;
};

template<typename Trait, typename Func = std::function<void ()>>
class queue_wrapper : public queue_wrapper_base<Func>
{
};

template<typename Func>
class queue_wrapper<Schedule, Func> : public queue_wrapper_base<Func, std::multimap<std::chrono::steady_clock::time_point, Func>>
{
private:
    using queue_type = std::multimap<std::chrono::steady_clock::time_point, Func>;
    using parent_type = queue_wrapper_base<Func, queue_type>;

public:
    enum { DEFAULT = 0 };

    queue_wrapper(uint32_t threshold, uint32_t queue_limit)
    : parent_type(0, 0)
    {
    }
    virtual void add(size_t key, Func&& func)
    {
        using namespace std;
        using namespace std::chrono;

        steady_clock::time_point tp = steady_clock::now() + microseconds(key);
        {
            std::lock_guard<std::mutex> locker(parent_type::m_mutex);
            parent_type::m_queue.emplace(tp, std::move(func));
        }
        parent_type::on_add_func();
    }
    virtual Func get(void)
    {
        using namespace std;
        using namespace std::chrono;

        steady_clock::time_point now = steady_clock::now();
        auto get_first = [&]() -> Func {
            auto iter = parent_type::m_queue.begin();
            if((iter != parent_type::m_queue.end()) && (iter->first < steady_clock::now()))
            {
                Func ret = iter->second;
                parent_type::m_queue.erase(iter);
                return move(ret);
            }
            else
            {
                return move(Func());
            }
        };

        lock_guard<mutex> locker(parent_type::m_mutex);
        if(!parent_type::m_queue.empty())
        {
            auto iter = parent_type::m_queue.begin();
            if(iter->first > now)
            {
                parent_type::m_signal.wait_for(parent_type::m_mutex, (iter->first - now));
            }
            return get_first();
        }
        else
        {
            parent_type::m_signal.wait_for(parent_type::m_mutex, seconds(parent_type::m_idle_sec));
            return get_first();
        }
    }
};

template<typename Func>
class queue_wrapper<Priority, Func> : public queue_wrapper_base<Func, std::multimap<size_t, Func>>
{
private:
    using queue_type = std::multimap<size_t, Func>;
    using parent_type = queue_wrapper_base<Func, queue_type>;

public:
    enum { DEFAULT = 5 };
    enum {
        PRIORITY_LOWEST = 1
        , PRIORITY_HIGHEST = 10
    };

    queue_wrapper(uint32_t threshold, uint32_t queue_limit)
    : parent_type(threshold, queue_limit)
    {
    }
    virtual void add(size_t key, Func&& func)
    {
        if(key > PRIORITY_HIGHEST)
        {
            key = PRIORITY_HIGHEST;
        }
        else if(key < PRIORITY_LOWEST)
        {
            key = PRIORITY_LOWEST;
        }
        {
            std::lock_guard<std::mutex> locker(parent_type::m_mutex);
            parent_type::m_queue.emplace(key, std::move(func));
        }
        parent_type::on_add_func();
    }
    virtual Func get(void)
    {
        using namespace std;
        using namespace std::chrono;

        auto get_first = [&]() -> Func {
            auto iter = parent_type::m_queue.begin();
            if(iter != parent_type::m_queue.end())
            {
                Func ret = iter->second;
                parent_type::m_queue.erase(iter);
                return move(ret);
            }
            else
            {
                return move(Func());
            }
        };
        lock_guard<mutex> locker(parent_type::m_mutex);
        if(!parent_type::m_queue.empty())
        {
            return get_first();
        }
        else
        {
            parent_type::m_signal.wait_for(parent_type::m_mutex, seconds(parent_type::m_idle_sec));
            return get_first();
        }
    }
};

template<typename Func>
class queue_wrapper<Sequence, Func> : public queue_wrapper_base<Func, std::deque<Func>>
{
private:
    using queue_type = std::deque<Func>;
    using parent_type = queue_wrapper_base<Func, queue_type>;

public:
    enum { DEFAULT = 0 };

    queue_wrapper(uint32_t threshold, uint32_t queue_limit)
    : parent_type(threshold, queue_limit)
    {
    }
    virtual void add(size_t key, Func&& func)
    {
        {
            std::lock_guard<std::mutex> locker(parent_type::m_mutex);
            parent_type::m_queue.emplace_back(std::move(func));
        }
        parent_type::on_add_func();
    }
    virtual Func get(void)
    {
        using namespace std;
        using namespace std::chrono;

        auto get_first = [&]() -> Func {
            if(parent_type::m_queue.size() > 0)
            {
                Func ret = parent_type::m_queue.front();
                parent_type::m_queue.pop_front();
                return move(ret);
            }
            else
            {
                return move(Func());
            }
        };
        lock_guard<mutex> locker(parent_type::m_mutex);
        if(!parent_type::m_queue.empty())
        {
            return get_first();
        }
        else
        {
            parent_type::m_signal.wait_for(parent_type::m_mutex, seconds(parent_type::m_idle_sec));
            return get_first();
        }
    }
};

class thread_manage_policy_base
{
public:
    thread_manage_policy_base(uint32_t min, uint32_t max)
    : m_min(min)
    , m_max(max)
    , m_current(0)
    , m_idle_sec(30)
    {
    }
    void set_thread_idle_seconds(uint32_t seconds)
    {
        m_idle_sec = seconds;
    }
    bool must_create_thread(void)
    {
        return m_current < m_min;
    }
    bool apply_for_create_thread(void)
    {
        return m_current < m_max;
    }
    bool apply_for_destory_thread(const std::chrono::steady_clock::time_point& last_active)
    {
        using namespace std::chrono;

        if(m_current > m_max)
        {
            return true;
        }
        else if(m_current <= m_min)
        {
            return false;
        }
        else
        {
            uint32_t span = duration_cast<duration<uint32_t>>(steady_clock::now() - last_active).count();
            return (span >= m_idle_sec);
        }
    }
    void on_thread_start(void)
    {
        m_current++;
    }
    void on_thread_exit(void)
    {
        m_current--;
    }

private:
    uint32_t m_min;
    uint32_t m_max;
    std::atomic<uint32_t> m_current;
    uint32_t m_idle_sec;
};

template<typename T>
class thread_manage_policy : public thread_manage_policy_base
{
};

template<>
class thread_manage_policy<AutoManage> : public thread_manage_policy_base
{
public:
    thread_manage_policy(uint32_t cpus)
    : thread_manage_policy_base(2, cpus + 1)
    {
    }
};

template<>
class thread_manage_policy<Fixed> : public thread_manage_policy_base
{
public:
    thread_manage_policy(uint32_t num)
    : thread_manage_policy_base(num, num)
    {
    }
};

template<>
class thread_manage_policy<Infinity> : public thread_manage_policy_base
{
public:
    thread_manage_policy(uint32_t init)
    : thread_manage_policy_base(init, init + 100)
    {
    }
};

} // namespace native

template<typename QueueType = Schedule, typename Policy = AutoManage>
class thread_pool
{
private:
    using func_type = std::function<void ()>;
    using queue_type = native::queue_wrapper<QueueType, func_type>;
    using policy_base_type = native::thread_manage_policy_base;
    using policy_type = native::thread_manage_policy<Policy>;

public:
    thread_pool(uint32_t queue_limit = 1000)
    : m_queue(std::min((uint32_t)50, queue_limit / 10), queue_limit)
    , m_policy(4)
    , m_running(false)
    , m_idle_sec(30)
    {
        m_policy.set_thread_idle_seconds(m_idle_sec);
        m_queue.set_thread_idle_seconds(m_idle_sec);
        start();
    }
    ~thread_pool(void)
    {
        stop();
        join();
    }
    void start(void)
    {
        using namespace std;
        using namespace std::chrono;

        if(!m_running)
        {
            m_running = true;
            while(m_policy.must_create_thread())
            {
                lock_guard<mutex> locker(m_threads_mutex);
                m_threads.push_back(thread(&thread_pool::worker_thread, this));
            }
            m_last_check = steady_clock::now();
        }
    }
    void stop(void)
    {
        m_running = false;
        m_queue.clear();
    }
    void join(void)
    {
        std::lock_guard<std::mutex> locker(m_threads_mutex);
        for(std::thread& t : m_threads)
        {
            if(t.joinable())
            {
                t.join();
            }
        }
        m_threads.clear();
        m_exits.clear();
    }
    void set_thread_idle_seconds(uint32_t seconds)
    {
        if(m_idle_sec != seconds)
        {
            m_idle_sec = seconds;
            m_queue.set_thread_idle_seconds(m_idle_sec);
            m_policy.set_thread_idle_seconds(m_idle_sec);
        }
    }
    bool add_job(const func_type& func, int arg = queue_type::DEFAULT)
    {
        using namespace std;
        using namespace std::chrono;
        if(!m_running || m_queue.out_of_limit())
        {
            return false;
        }
        m_queue.add(arg, func_type(func));
        if(m_policy.must_create_thread() || (m_queue.out_of_threshold() && m_policy.apply_for_create_thread()))
        {
            lock_guard<mutex> locker(m_threads_mutex);
            m_threads.push_back(thread(&thread_pool::worker_thread, this));
        }
        uint32_t span = duration_cast<duration<uint32_t>>(steady_clock::now() - m_last_check).count();
        if(span >= 10)
        {
            thread(&thread_pool::check_exits, this).detach();
        }
        return true;
    }
    template<typename... Types>
    bool add_job(const std::function<void (Types...)>& func, Types... args, int arg = queue_type::DEFAULT)
    {
        return add_job(std::bind(func, args...), arg);
    }

private:
    void worker_thread(void)
    {
        using namespace std;
        using namespace std::chrono;

        bool work = true;
        m_policy.on_thread_start();
        steady_clock::time_point last_active = steady_clock::now();
        while(work && m_running)
        {
            func_type func = m_queue.get();
            if(func)
            {
                func();
                last_active = steady_clock::now();
            }
            else if(m_policy.apply_for_destory_thread(last_active))
            {
                work = false;
            }
        }
        m_policy.on_thread_exit();
        {
            thread::id tid = this_thread::get_id();
            lock_guard<mutex> locker(m_exits_mutex);
            m_exits.push_back(tid);
        }
    }
    void check_exits(void)
    {
        using namespace std;
        using namespace std::chrono;

        vector<thread::id> exits;
        {
            lock_guard<mutex> locker(m_exits_mutex);
            m_exits.swap(exits);
        }
        for(thread::id& id : exits)
        {
            lock_guard<mutex> locker(m_threads_mutex);
            size_t total = m_threads.size();
            for(size_t i = 0; i < total; i++)
            {
                if(m_threads[i].get_id() == id)
                {
                    if(i < (total - 1))
                    {
                        m_threads[i].swap(m_threads[total - 1]);
                    }
                    if(m_threads[total - 1].joinable())
                    {
                        m_threads[total - 1].join();
                    }
                    m_threads.pop_back();
                    break;
                }
            }
        }
        m_last_check = steady_clock::now();
    }

private:
    queue_type m_queue;
    policy_type m_policy;
    std::atomic<bool> m_running;

    std::vector<std::thread> m_threads;
    std::mutex m_threads_mutex;
    std::vector<std::thread::id> m_exits;
    std::mutex m_exits_mutex;
    std::chrono::steady_clock::time_point m_last_check;

    uint32_t m_idle_sec;
};

} // namespace snower

#endif // __SNOWER_THREAD_POOL_INL__

