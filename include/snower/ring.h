#ifndef __SNOWER_RING_H__
#define __SNOWER_RING_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace snower
{

template<int Length>
struct length_plus_one
{
    enum { LENGTH = Length + 1 };
};

template<typename Item, int Length>
class ring
{
public:
    ring(void)
    : m_head(0)
    , m_tail(0)
    {
    }
    ~ring(void) {}

    bool full(void) const
    {
        int delta = m_tail - m_head;
        return (delta == -1) || (delta == Length);
    }
    bool empty(void) const { return m_head == m_tail; }
    size_t size(void) const
    {
        int delta = m_tail - m_head;
        return (delta < 0) ? (size_t)(delta + 1 + Length) : (size_t)delta;
    }
    void clear(void) { m_head = 0; m_tail = 0; }

    bool try_push(const Item& item)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        if(!full())
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_head = 0; }
            m_pop_signal.notify_one();
        }
    }
    bool try_push(Item&& item)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    template<typename... Types>
    bool try_push(Types... args)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(args...);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    bool push(const Item& item)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        m_push_signal.wait(m_push_mutex, [=](){ return !full(); });
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    bool push(Item&& item)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        m_push_signal.wait(m_push_mutex, [=](){ return !full(); });
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    template<typename... Types>
    bool push(Types... args)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        m_push_signal.wait(m_push_mutex, [=](){ return !full(); });
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(args...);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    template<typename Rep, typename Period>
    bool push(const Item& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        m_push_signal.wait_for(m_push_mutex, rel_time, [=](){ return !full(); });
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }
    template<typename Rep, typename Period>
    bool push(Item&& item, const std::chrono::duration<Rep, Period>& rel_time)
    {
        using namespace std;
        lock_guard<mutex> locker(m_push_mutex);
        m_push_signal.wait_for(m_push_mutex, rel_time, [=](){ return !full(); });
        bool ret = !full();
        if(ret)
        {
            Item* ptr = &m_items[m_tail];
            ptr->~Item();
            new(ptr) Item(item);
            if(++m_tail > Length) { m_tail = 0; }
            m_pop_signal.notify_one();
        }
        return ret;
    }

    std::tuple<bool, Item> try_pop(void)
    {
        using namespace std;
        lock_guard<mutex> locker(m_pop_mutex);
        if(!empty())
        {
            tuple<bool, Item> ret = make_tuple(true, move(m_items[m_head]));
            if(++m_head > Length) { m_head = 0; }
            m_push_signal.notify_one();
            return move(ret);
        }
        else
        {
            return make_tuple(false, Item());
        }
    }
    std::tuple<bool, Item> pop(void)
    {
        using namespace std;
        lock_guard<mutex> locker(m_pop_mutex);
        m_pop_signal.wait(m_pop_mutex, [=](){ return !empty(); });
        if(!empty())
        {
            tuple<bool, Item> ret = make_tuple(true, move(m_items[m_head]));
            if(++m_head > Length) { m_head = 0; }
            m_push_signal.notify_one();
            return move(ret);
        }
        else
        {
            return make_tuple(false, Item());
        }
    }
    template<typename Rep, typename Period>
    std::tuple<bool, Item> pop(const std::chrono::duration<Rep, Period>& rel_time)
    {
        using namespace std;
        lock_guard<mutex> locker(m_pop_mutex);
        m_pop_signal.wait_for(m_pop_mutex, rel_time, [=](){ return !empty(); });
        if(!empty())
        {
            tuple<bool, Item> ret = make_tuple(true, move(m_items[m_head]));
            if(++m_head > Length) { m_head = 0; }
            m_push_signal.notify_one();
            return move(ret);
        }
        else
        {
            return make_tuple(false, Item());
        }
    }

private:
    Item m_items[length_plus_one<Length>::LENGTH];
    std::atomic<int> m_head;
    std::atomic<int> m_tail;
    std::mutex m_push_mutex;
    std::condition_variable_any m_push_signal;
    std::mutex m_pop_mutex;
    std::condition_variable_any m_pop_signal;
};

} // namespace snower

#endif // __SNOWER_RING_H__

