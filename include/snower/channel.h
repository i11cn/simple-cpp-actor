#ifndef __SNOWER_MAILBOX_H__
#define __SNOWER_MAILBOX_H__

#include <deque>
#include <tuple>
#include <snower/ring.h>

namespace snower
{

namespace impl
{

template<typename Type, int Length, bool Inf>
class _channel
{
};

template<typename Type, int Length>
class _channel<Type, Length, true>
{
public:
    _channel(void) {};
    ~_channel(void) {};

    size_t size(void) const { return m_queue.size(); }
    bool empty(void) const { return m_queue.empty(); }
    void clear(void) { m_queue.clear(); }

    bool try_push(const Type& item)
    {
        using namespace std;
        {
            lock_guard<mutex> locker(m_mutex);
            m_queue.push_back(item);
        }
        m_pop_signal.notify_one();
        return true;
    }
    bool try_push(Type&& item)
    {
        using namespace std;
        {
            lock_guard<mutex> locker(m_mutex);
            m_queue.push_back(move(item));
        }
        m_pop_signal.notify_one();
        return true;
    }
    bool push(const Type& item) { return try_push(item); }
    bool push(Type&& item) { return try_push(move(item)); }
    template<typename... Types>
    bool push(Types... args)
    {
        using namespace std;
        {
            lock_guard<mutex> locker(m_mutex);
            m_queue.emplace_back(args...);
        }
        m_pop_signal.notify_one();
        return true;
    }
    template<typename Rep, typename Period>
    bool push(const Type& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_push(item);
    }
    template<typename Rep, typename Period>
    bool push(Type&& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        return try_push(move(item));
    }
    std::tuple<bool, Type> try_pop(void)
    {
        return do_pop([](){});
    }
    std::tuple<bool, Type> pop(void)
    {
        return do_pop([&](){ m_pop_signal.wait(m_mutex, [=](){ return !m_queue.empty(); }); });
    }
    template<typename Rep, typename Period>
    std::tuple<bool, Type> pop(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return do_pop([&](){ m_pop_signal.wait_for(m_mutex, rel_time, [=](){ return !m_queue.empty(); }); });
    }

private:
    std::tuple<bool, Type> do_pop(const std::function<void ()>& func)
    {
        using namespace std;
        lock_guard<mutex> locker(m_mutex);
        func();
        if(!m_queue.empty())
        {
            Type obj = m_queue.front();
            m_queue.pop_front();
            return move(tuple<bool, Type>(true, move(obj)));
        }
        return move(tuple<bool, Type>(false, Type()));
    }

private:
    std::deque<Type> m_queue;
    std::mutex m_mutex;
    std::condition_variable_any m_pop_signal;
};

template<typename Type, int Length>
class _channel<Type, Length, false>
{
public:
    _channel(void) {};
    ~_channel(void) {};

    size_t size(void) const { return m_queue.size(); }
    bool empty(void) const { return m_queue.empty(); }
    void clear(void) { m_queue.clear(); }

    bool try_push(const Type& item) { return m_queue.try_push(item); }
    bool try_push(Type&& item) { return m_queue.try_push(move(item)); }
    bool push(const Type& item) { return m_queue.push(item); }
    bool push(Type&& item) { return m_queue.push(move(item)); }
    template<typename... Types>
    bool push(Types... args) { return m_queue.push(args...); }
    template<typename Rep, typename Period>
    bool push(const Type& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        return m_queue.push(item, rel_time);
    }
    template<typename Rep, typename Period>
    bool push(const Type&& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        return m_queue.push(move(item), rel_time);
    }

    std::tuple<bool, Type> try_pop(void)
    {
        return m_queue.try_pop();
    }
    std::tuple<bool, Type> pop(void)
    {
        return m_queue.pop();
    }
    template<typename Rep, typename Period>
    std::tuple<bool, Type> pop(const std::chrono::duration<Rep, Period>& rel_time)
    {
        return m_queue.pop(rel_time);
    }

private:
    ring<Type, Length> m_queue;
};

} // namespace impl

template<typename Type, int Length>
using channel = impl::_channel<Type, Length, Length <= 0>;

} // namespace snower

#endif // __SNOWER_MAILBOX_H__

