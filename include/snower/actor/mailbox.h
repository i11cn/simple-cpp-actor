#ifndef __SNOWER_ACTOR_MAILBOX_H__
#define __SNOWER_ACTOR_MAILBOX_H__

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <snower/channel.h>
#include <snower/actor/actor_address.h>

#include <snower/logger.h>
#include <snower/singleton.h>

namespace snower
{
namespace actor
{

template<typename Item>
class mailbox : public std::enable_shared_from_this<mailbox<Item>>
{
private:
    mailbox(void)
    : m_in_pool(false)
    {
    }
    ~mailbox(void) { }

public:
    template<typename T>
    mailbox(T) = delete;
    template<typename T>
    mailbox& operator = (T) = delete;

    template<typename... Types>
    bool push(const std::shared_ptr<class actor>& act, const actor_address& sender, Types&&... args)
    {
        return false;
    }
    bool push(const Item& func)
    {
        singletons<logger>::get_instance("mailbox").INFO("邮箱中加入一个函数", " - const&");
        return m_mailbox.try_push(func);
    }
    bool push(Item&& func)
    {
        singletons<logger>::get_instance("mailbox").INFO("邮箱中加入一个函数", " - &&");
        return m_mailbox.try_push(std::move(func));
    }
    void clear(void)
    {
        m_mailbox.clear();
    }
    Item pop(void)
    {
        using namespace std;
        function<void()> ret;
        tie(ignore, ret) = m_mailbox.try_pop();
        return move(ret);
    }
    bool add_to_pool (void)
    {
        std::lock_guard<std::mutex> locker(m_check_mutex);
        if(!m_in_pool.load() && m_mailbox.size() > 0)
        {
            m_in_pool.store(true);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    void thread_pool_enter(void)
    {
        std::lock_guard<std::mutex> locker(m_check_mutex);
        m_in_pool.store(true);
    }
    void thread_pool_leave(void)
    {
        std::lock_guard<std::mutex> locker(m_check_mutex);
        m_in_pool.store(false);
    }

private:
    channel<Item, 0> m_mailbox;
    std::atomic<bool> m_in_pool;
    std::mutex m_check_mutex;
    friend class actor_system;
};

} // namespace actor
} // namespace snower

#endif // __SNOWER_ACTOR_MAILBOX_H__

