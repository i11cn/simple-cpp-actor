#include <snower/actor/caller.h>
#include <snower/actor/actor_system.h>

#include <snower/logger.h>
#include <snower/singleton.h>

namespace snower
{
namespace actor
{

template<typename... Types>
void actor::handle(void(*func)(Types...))
{
    m_handlers.emplace(caller::caller_hash<Types...>(), caller(std::function<void(Types...)>(func)));
}

template<typename Class, typename... Types>
void actor::handle(void(Class::* func)(Types...), Class* c)
{
    auto f = bind_args(func, c, typename gen_seqs<sizeof...(Types)>::seqs_type());
    m_handlers.emplace(caller::caller_hash<Types...>(), caller(std::function<void(Types...)>(f)));
}

template<typename... Types>
void actor::unhandle(void)
{
    m_handlers.erase(caller::caller_hash<Types...>());
}

template<typename... Types>
void actor::reply(Types... args)
{
    singleton<actor_system>::get_instance().send_as(get_self(), get_sender(), std::forward<Types>(args)...);
}

template<typename... Types>
void actor::send(const actor_address& receiver, Types... args)
{
    singleton<actor_system>::get_instance().send_as(get_self(), receiver, std::forward<Types>(args)...);
}

template<typename... Types>
void actor::forward(const actor_address& next, Types... args)
{
    singleton<actor_system>::get_instance().send_as(get_sender(), next, std::forward<Types>(args)...);
}

template<typename... Types>
bool actor::call(Types&&... args)
{
    logger& l = singletons<logger>::get_instance("actor");
    l.INFO("正在准备调用");
    auto iter = m_handlers.find(caller::caller_hash<Types...>());
    if(iter == m_handlers.end())
    {
        l.INFO("类型的hash值是：", caller::caller_hash<Types...>());
        l.INFO("m_handlers中有 ", m_handlers.size(), " 个handle");
        {
            l.INFO("他们的Hash分别是");
            for(auto& i : m_handlers)
            {
                l.INFO(i.first);
            }
        }
        l.WARN("没有找到对应的handle");
        return false;
    }
    else
    {
        l.INFO("找到了对应的handle");
        iter->second(std::forward<Types>(args)...);
        return true;
    }
}

template<typename... Types>
bool actor::call(std::tuple<Types...>* t)
{
    auto iter = m_handlers.find(caller::caller_hash<Types...>());
    if(iter == m_handlers.end())
    {
        return false;
    }
    else
    {
        iter->second(*t);
        return true;
    }
}

template<typename... Types>
std::function<void()> actor::bind(Types... args)
{
    auto iter = m_handlers.find(caller::caller_hash<Types...>());
    if(iter == m_handlers.end())
    {
        return std::function<void()>();
    }
    else
    {
        return [&]() { iter->second(std::forward<Types>(args)...); };
    }
}

} // namespace actor
} // namespace snower

